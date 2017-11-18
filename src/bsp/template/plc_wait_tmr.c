/******************************************************************************
*           This file contains retain wait timer related stuff.               *
*You can use src/bsp/common/stm32/plc_wait_tmr.c as reference implementation. *
******************************************************************************/

/*****************************************************************************/
/* Platform specific includes should be places here*/

/*
STM32 example:
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
*/


/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_wait_tmr.h>


/*****************************************************************************/
/* Add project specific includes here. */

/*****************************************************************************/
/* 
Delay timer base variable. 

This variable is used to count milliseconds for delays in PLC hardware drivers.

It must be incremented by periodic timer interrupt handler every millisecond. 
*/
volatile uint32_t plc_sys_timer = 0;

/*****************************************************************************/
/*This function should be used to setup periodic timer at device reset.*/
void plc_wait_tmr_init(void)
{
    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/*
This function must inrement "plc_sys_timer" every millisecond.

It should be called by priodic timer interrupt handler.

Or it can be priodic timer interrupt handler itself.

Feel free to rename this function as you need!
*/
void please_rename_me_wait_timer_tick_handler(void)
{
    /*
    You should use this function to increment "plc_sys_timer" and 
    to do other periodic actions.
    */

    /*Insert your code here.*/
}
