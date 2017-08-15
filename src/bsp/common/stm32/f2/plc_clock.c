/*
 * This file is based on the libopencm3 project.
 *
 * Copyright (C) 2016 Nucleron R&D LLC (main@nucleron.ru)
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <plc_config.h>
#include <plc_diag.h>
#include <plc_clock.h>
#include <plc_hw.h>

#define PLL_HSE 0x0
#define PLL_HSI 0x1

volatile uint8_t plc_clock_hse_failure = 0;

typedef struct
{
    uint32_t hpre;
    uint32_t ppre1;
    uint32_t ppre2;

    uint32_t flash_ws;

    uint32_t pll_m;
    uint32_t pll_n;
    uint32_t pll_p;
    uint32_t pll_q;

    uint32_t pll_src;

    uint32_t ahb_freq;
    uint32_t apb1_freq;
    uint32_t apb2_freq;
} clk_cfg;

/* 120MHz configs */
static const clk_cfg cfg_hse_8Mhz =
{
    .hpre   = RCC_CFGR_HPRE_DIV_NONE,
    .ppre1  = RCC_CFGR_PPRE_DIV_4,
    .ppre2  = RCC_CFGR_PPRE_DIV_2,

    .flash_ws = FLASH_ACR_ICE | FLASH_ACR_DCE | FLASH_ACR_LATENCY_2WS,

    .pll_m     = 8,
    .pll_n     = 240,
    .pll_p     = 2,
    .pll_q     = 5,
    .pll_src   = PLL_HSE,

    .ahb_freq  = 120000000ul,
    .apb1_freq =  30000000ul,
    .apb2_freq =  60000000ul
};

static const clk_cfg cfg_hse_16Mhz =
{
    .hpre   = RCC_CFGR_HPRE_DIV_NONE,
    .ppre1  = RCC_CFGR_PPRE_DIV_4,
    .ppre2  = RCC_CFGR_PPRE_DIV_2,

    .flash_ws = FLASH_ACR_ICE | FLASH_ACR_DCE | FLASH_ACR_LATENCY_2WS,

    .pll_m     = 16,
    .pll_n     = 240,
    .pll_p     = 2,
    .pll_q     = 5,
    .pll_src   = PLL_HSE,

    .ahb_freq  = 120000000ul,
    .apb1_freq =  30000000ul,
    .apb2_freq =  60000000ul
};

static const clk_cfg cfg_hsi =
{
    .hpre   = RCC_CFGR_HPRE_DIV_NONE,
    .ppre1  = RCC_CFGR_PPRE_DIV_4,
    .ppre2  = RCC_CFGR_PPRE_DIV_2,

    .flash_ws = FLASH_ACR_ICE | FLASH_ACR_DCE | FLASH_ACR_LATENCY_2WS,

    .pll_m     = 16,
    .pll_n     = 240,
    .pll_p     = 2,
    .pll_q     = 5,
    .pll_src   = PLL_HSI,

    .ahb_freq  = 120000000ul,
    .apb1_freq =  30000000ul,
    .apb2_freq =  60000000ul
};

static void pll_setup(const clk_cfg *clock)
{
    /*
     * Set prescalers for AHB, ABP1, ABP2.
     * Do this before touching the PLL (TODO: why?).
     */
    rcc_set_hpre  (clock->hpre );
    rcc_set_ppre1 (clock->ppre1);
    rcc_set_ppre2 (clock->ppre2);
    /*
     * Sysclk runs with 72MHz -> 2 waitstates.
     * 0WS from 0-24MHz
     * 1WS from 24-48MHz
     * 2WS from 48-72MHz
     */
    flash_set_ws(clock->flash_ws);
    /*
     * Set the PLL multiplication factor to 9.
     * fsrc * multiplier
     */
    if (clock->pll_src==PLL_HSE)
    {
        rcc_set_main_pll_hse(clock->pll_m, clock->pll_n, clock->pll_p, clock->pll_q);
    }
    else
    {
        rcc_set_main_pll_hsi(clock->pll_m, clock->pll_n, clock->pll_p, clock->pll_q);
    }

    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);

    /* Select PLL as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SWS_PLL);

    /* Wait for PLL clock to be selected. */
    rcc_wait_for_sysclk_status(RCC_PLL);

    /* Set the peripheral clock frequencies used */
    rcc_ahb_frequency  = clock->ahb_freq;
    rcc_apb1_frequency = clock->apb1_freq;
    rcc_apb2_frequency = clock->apb2_freq;
}

static bool pll_is_dirty = false;

void plc_clock_setup(void)
{
    uint32_t i;
    /* Enable internal high-speed oscillator. */
    rcc_osc_on(RCC_HSI);
    rcc_wait_for_osc_ready(RCC_HSI);

    /* Select HSI as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SWS_HSI);

    /* Enable clock security system, as HSE depends on external wiring. */
    rcc_css_enable();

    /* Try to setup HSE oscilator. */
    rcc_osc_on(RCC_HSE);
    for (i = 0; i < 1000000; i++)
    {
        if (RCC_CR & RCC_CR_HSERDY)
        {
            /* Sucess. */
            pll_is_dirty = true;
            pll_setup(&PLC_HSE_CONFIG);
            pll_is_dirty = false;

            return;
        }
    }
    /* Fallback to HSI. */
    pll_is_dirty = true;
    pll_setup(&cfg_hsi);
    pll_is_dirty = false;
    /* This is an error, but we can do some work... */
    plc_clock_hse_failure = 1;
    //plc_diag_status |= PLC_DIAG_ERR_HSE;
}

void nmi_handler(void)
{
    if (RCC_CIR & RCC_CIR_CSSF)
    {
        /* Clear CSS interrupt! */
        RCC_CIR |= RCC_CIR_CSSC;
        if (pll_is_dirty)
        {
            /* We can't call rcc_pll_setup now, just reset the system. */
            scb_reset_system();
        }
        else
        {
            /* We are already on HSI, so we need only PLL setup. */
            pll_setup(&cfg_hsi);
            /* This is an error, but we can do some work... */
            plc_clock_hse_failure = 1;
            //plc_diag_status |= PLC_DIAG_ERR_HSE;
        }
    }
}
