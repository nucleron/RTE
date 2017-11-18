/******************************************************************************
*                                                                             *
******************************************************************************/

/*
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
*/

/*****************************************************************************/
#include <plc_config.h>
#include <plc_wait_tmr.h>

/*****************************************************************************/
extern volatile uint32_t plc_sys_timer;

/*****************************************************************************/
void plc_wait_tmr_init(void)
{
}

void PLC_WAIT_TMR_ISR(void)
{
}
