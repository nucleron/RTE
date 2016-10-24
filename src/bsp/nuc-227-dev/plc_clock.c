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

#include <plc_clock.h>
#include <plc_hw.h>

#include <plc_config.h>

static bool pll_is_dirty = 0;

static void rcc_pll_setup( uint32_t source, const struct rcc_clock_scale *clock )
{

    /* Enable/disable high performance mode */
    if (!clock->power_save)
    {
        pwr_set_vos_scale(PWR_SCALE1);
    }
    else
    {
        pwr_set_vos_scale(PWR_SCALE2);
    }

    /*
     * Set prescalers for AHB, ADC, ABP1, ABP2.
     * Do this before touching the PLL (TODO: why?).
     */
    rcc_set_hpre(clock->hpre);
    rcc_set_ppre1(clock->ppre1);
    rcc_set_ppre2(clock->ppre2);

    if( RCC_CFGR_SW_HSE == source )
    {
        rcc_set_main_pll_hse(clock->pllm, clock->plln, clock->pllp, clock->pllq);
    }
    else
    {
        rcc_set_main_pll_hsi(clock->pllm, clock->plln, clock->pllp, clock->pllq);
    }

    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);

    /* Configure flash settings. */
    flash_set_ws(clock->flash_config);

    /* Select PLL as SYSCLK source. */
    rcc_set_sysclk_source( RCC_CFGR_SW_PLL );

    /* Wait for PLL clock to be selected. */
    rcc_wait_for_sysclk_status(RCC_PLL);

    /* Set the peripheral clock frequencies used. */
    rcc_apb1_frequency = clock->apb1_frequency;
    rcc_apb2_frequency = clock->apb2_frequency;
}

void plc_clock_setup(void)
{
    uint32_t i;
    /* Enable internal high-speed oscillator. */
    rcc_osc_on(RCC_HSI);
    rcc_wait_for_osc_ready(RCC_HSI);

    /* Select HSI as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SW_HSI);

    /* Enable clock security system, as HSE depends on external wiring. */
    rcc_css_enable();

    /* Try to setup HSE oscilator. */
    rcc_osc_on(RCC_HSE);
    for( i = 0; i < 1000000; i++ )
    {
        if( RCC_CR & RCC_CR_HSERDY )
        {
            /* Sucess. */
            pll_is_dirty = true;
            rcc_pll_setup( RCC_CFGR_SW_HSE, &PLC_HSE_CONFIG[RCC_CLOCK_3V3_168MHZ] );
            pll_is_dirty = false;

            return;
        }
    }
    /* Fallback to HSI. */
    pll_is_dirty = true;
    rcc_pll_setup( RCC_CFGR_SW_HSI, &rcc_hse_16mhz_3v3[RCC_CLOCK_3V3_168MHZ] );
    pll_is_dirty = false;
    /* This is an error, but we can do some work... */
    plc_diag_status |= PLC_HW_ERR_HSE;
}

void nmi_handler(void)
{
    if( RCC_CIR & RCC_CIR_CSSF )
    {
        /* Clear CSS interrupt! */
        RCC_CIR |= RCC_CIR_CSSC;
        if( pll_is_dirty )
        {
            /* We can't call rcc_pll_setup now, just reset the system. */
            scb_reset_system();
        }
        else
        {
            /* We are already on HSI, so we need only PLL setup. */
            rcc_pll_setup( RCC_HSI, &rcc_hse_16mhz_3v3[RCC_CLOCK_3V3_168MHZ] );
            /* This is an error, but we can do some work... */
            plc_diag_status |= PLC_HW_ERR_HSE;
        }
    }
}
