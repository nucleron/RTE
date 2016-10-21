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
#include <plc_clock.h>
#include <plc_diag.h>
#include <plc_hw.h>

typedef struct
{

    uint32_t hpre;
    uint32_t adcpre;
    uint32_t ppre1;
    uint32_t ppre2;

    uint32_t flash_ws;

    uint32_t pll_m;
    uint32_t pll_src;
    uint32_t pll_xtpre;

    uint32_t ahb_freq;
    uint32_t apb1_freq;
    uint32_t apb2_freq;
} clk_cfg;

/* 64MHz configs */
static const clk_cfg cfg_hse_8Mhz =
{
    .hpre   = RCC_CFGR_HPRE_SYSCLK_NODIV,
    .adcpre = RCC_CFGR_ADCPRE_PCLK2_DIV8,
    .ppre1  = RCC_CFGR_PPRE1_HCLK_DIV2,
    .ppre2  = RCC_CFGR_PPRE2_HCLK_NODIV,

    .flash_ws = FLASH_ACR_LATENCY_2WS,

    .pll_m     = RCC_CFGR_PLLMUL_PLL_CLK_MUL8,
    .pll_src   = RCC_CFGR_PLLSRC_HSE_CLK,
    .pll_xtpre = RCC_CFGR_PLLXTPRE_HSE_CLK,

    .ahb_freq  = 64000000ul,
    .apb1_freq = 32000000ul,
    .apb2_freq = 64000000ul
};

static const clk_cfg cfg_hse_16Mhz =
{
    .hpre   = RCC_CFGR_HPRE_SYSCLK_NODIV,
    .adcpre = RCC_CFGR_ADCPRE_PCLK2_DIV8,
    .ppre1  = RCC_CFGR_PPRE1_HCLK_DIV2,
    .ppre2  = RCC_CFGR_PPRE2_HCLK_NODIV,

    .flash_ws = FLASH_ACR_LATENCY_2WS,

    .pll_m     = RCC_CFGR_PLLMUL_PLL_CLK_MUL8,
    .pll_src   = RCC_CFGR_PLLSRC_HSE_CLK,
    .pll_xtpre = RCC_CFGR_PLLXTPRE_HSE_CLK_DIV2,

    .ahb_freq  = 64000000ul,
    .apb1_freq = 32000000ul,
    .apb2_freq = 64000000ul
};

static const clk_cfg cfg_hsi =
{
    .hpre   = RCC_CFGR_HPRE_SYSCLK_NODIV,
    .adcpre = RCC_CFGR_ADCPRE_PCLK2_DIV8,
    .ppre1  = RCC_CFGR_PPRE1_HCLK_DIV2,
    .ppre2  = RCC_CFGR_PPRE2_HCLK_NODIV,

    .flash_ws = FLASH_ACR_LATENCY_2WS,

    .pll_m     = RCC_CFGR_PLLMUL_PLL_CLK_MUL16,
    .pll_src   = RCC_CFGR_PLLSRC_HSI_CLK_DIV2,

    .ahb_freq  = 64000000ul,
    .apb1_freq = 32000000ul,
    .apb2_freq = 64000000ul
};

static void pll_setup( const clk_cfg *clock )
{
    /*
     * Set prescalers for AHB, ADC, ABP1, ABP2.
     * Do this before touching the PLL (TODO: why?).
     */
    rcc_set_hpre  (clock->hpre  );
    rcc_set_adcpre(clock->adcpre);
    rcc_set_ppre1 (clock->ppre1 );
    rcc_set_ppre2 (clock->ppre2 );
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
    rcc_set_pll_multiplication_factor(clock->pll_m);

    /* Select PLL source. */
    rcc_set_pll_source(clock->pll_src);

    if (RCC_CFGR_PLLSRC_HSE_CLK==clock->pll_src)
    {
        /*Divide external frequency if needed*/
        rcc_set_pllxtpre(clock->pll_xtpre);
    }

    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);

    /* Select PLL as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_PLLCLK);

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
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);

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
            pll_setup( &PLC_HSE_CONFIG );
            pll_is_dirty = false;

            return;
        }
    }
    /* Fallback to HSI. */
    pll_is_dirty = true;
    pll_setup( &cfg_hsi );
    pll_is_dirty = false;
    /* This is an error, but we can do some work... */
    plc_diag_status |= PLC_DIAG_ERR_HSE;
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
            pll_setup( &cfg_hsi );
            /* This is an error, but we can do some work... */
            plc_diag_status |= PLC_DIAG_ERR_HSE;
        }
    }
}
