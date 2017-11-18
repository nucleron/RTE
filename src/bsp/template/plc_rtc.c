/******************************************************************************
*                   This file contains RTC related stuff.                     *
*  You can use src/bsp/common/stm32/f2/plc_rtc.c as reference implementation. *
******************************************************************************/

/* Platform specific includes. */
/*
STM32 example:
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
*/

/*****************************************************************************/
/* YAPLC includes. */
#include <plc_config.h>
#include <plc_rtc.h>

/*****************************************************************************/
/* Add project specific includes here. */


/*****************************************************************************/
/* RTC failure flag. */
volatile uint8_t plc_rtc_failure = 0; //Not failed by default

/*****************************************************************************/
/* 
This function is called on device reset.
It sets up all hardware needed for propper RTC work and checks RTC status.

If RTC is OK then this function must return 1, else 0.
 */
uint32_t plc_rtc_clken_and_check(void)
{
    uint32_t ret = 1; /*RTC is OK by default!*/
    /* Insert your code here. */
    return ret;
}

/*---------------------------------------------------------------------------*/
/* This function is called when we need to init/reset the RTC. */
void plc_rtc_init(tm* time)
{
    (void)time; /*A pointer to date/time buffer variable.*/
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/* This function is called when we need to change date/time. */
void plc_rtc_dt_set(tm* time)
{
    (void)time; /*A pointer to date/time buffer variable.*/
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/* 
This function is called when we need to get current date/time.

Time is written to *time variable.
 */
void plc_rtc_dt_get(tm* time)
{
    (void)time; /*A pointer to date/time buffer variable.*/
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function is called by periodic timer interrupt handler.

It is needed to update RTC state.
*/
void _plc_rtc_poll(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function is called by Beremiz-generated code.

It works just like plc_rtc_dt_get but the output format is IEC_TIME.
*/
void plc_rtc_time_get(IEC_TIME *current_time)
{
    /* Insert your code here. */
}



