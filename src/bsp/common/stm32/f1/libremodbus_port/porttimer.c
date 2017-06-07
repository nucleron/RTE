/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "serial_port.h"
#include "tcp_port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- libopencm3 STM32F includes -------------------------------*/
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <plc_config.h>

/* ----------------------- Initialize Timer -----------------------------*/
BOOL
xMBPortTimersInit(MULTIPORT_SERIAL_ARG  USHORT usTim1Timerout50us )
{
    //rcc_periph_clock_enable( RCC_GPIOA );
    //gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
    //gpio_clear( GPIOA, GPIO1 );

    /* Enable TIM clock. */
    rcc_periph_clock_enable(MB_TMR_PERIPH);
    nvic_enable_irq        (MB_TMR_VECTOR);
    timer_reset            (MB_TMR);
    /* Timer global mode: - Divider 4, Alignment edge, Direction up */
    timer_set_mode       (MB_TMR, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_continuous_mode(MB_TMR);
    timer_set_prescaler  (MB_TMR, ((2*rcc_apb1_frequency)/20000ul - 1)); /* 50 microseconds period */
    timer_set_period     (MB_TMR, usTim1Timerout50us);
    return TRUE;
}

/* ----------------------- Enable Timer -----------------------------*/
void
vMBPortTimersEnable(MULTIPORT_SERIAL_ARG_VOID)
{
    /* Restart the timer with the period value set in xMBPortTimersInit( ) */
    TIM_CNT(MB_TMR) = 1; /* Yes, this must be 1 !!! */

    timer_enable_irq     (MB_TMR, TIM_DIER_UIE);
    timer_enable_counter (MB_TMR);
}

/* ----------------------- Disable timer -----------------------------*/
void
vMBPortTimersDisable(MULTIPORT_SERIAL_ARG_VOID)

{
    timer_disable_irq    (MB_TMR, TIM_DIER_UIE);
    timer_disable_counter(MB_TMR);
}

void vMBPortTimersDelay(MULTIPORT_SERIAL_ARG USHORT usTimeOutMS )
{
    /*Not supproted*/
#if MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS > 0
#   error "MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS > 0 is not supported!!!"
#endif
}


/* ----------------------- Timer ISR -----------------------------*/
/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static CHAR count;
void MB_TMR_ISR(void)
{
    count++;
    if (timer_interrupt_source(MB_TMR, TIM_SR_UIF))
    {
        timer_clear_flag(MB_TMR, TIM_SR_UIF); /* Clear interrrupt flag. */
    }
    timer_get_flag(MB_TMR, TIM_SR_UIF);	/* Reread to force the previous (buffered) write before leaving */
    pxMBPortCBTimerExpired();
}

