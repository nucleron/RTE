/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>


#include <plc_config.h>
#include <plc_wait_tmr.h>
#include <plc_iom.h>

void plc_wait_tmr_init(void)
{
    //Wait timer config, basic timers TIM6 and TIM7 may be used
    rcc_periph_clock_enable( PLC_WAIT_TMR_PERIPH );

    timer_reset            ( PLC_WAIT_TMR );
    timer_set_prescaler    ( PLC_WAIT_TMR, ((2*rcc_apb1_frequency)/1000000ul - 1)); //1MHz
    timer_disable_preload  ( PLC_WAIT_TMR );
    timer_continuous_mode  ( PLC_WAIT_TMR );
    timer_set_period       ( PLC_WAIT_TMR, 100 ); //10KHz

    timer_enable_counter   ( PLC_WAIT_TMR );
    timer_enable_irq       ( PLC_WAIT_TMR, TIM_DIER_UIE);

    nvic_enable_irq( PLC_WAIT_TMR_VECTOR );
}

volatile uint32_t plc_sys_timer = 0;
volatile uint32_t plc_wait_cnt  = 0;

extern void plc_hmi_vout_poll(void);
extern void _plc_ain_adc_poll(void);
extern void _plc_aout_dac_poll(void);

void PLC_WAIT_TMR_ISR(void)
{

    if (timer_get_flag(PLC_WAIT_TMR, TIM_SR_UIF))
    {

        /* Clear compare interrupt flag. */
        timer_clear_flag(PLC_WAIT_TMR, TIM_SR_UIF);
        plc_wait_cnt++;

        plc_hmi_vout_poll();
        _plc_ain_adc_poll();
        _plc_aout_dac_poll();

        if (0 == plc_wait_cnt%10)
        {
            plc_sys_timer++;
            plc_iom_tick();
        }
    }
}
