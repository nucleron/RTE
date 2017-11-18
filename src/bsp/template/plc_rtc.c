/******************************************************************************
*                                                                             *
******************************************************************************/

/*
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
*/

/*****************************************************************************/
#include <plc_config.h>
#include <plc_rtc.h>

/*****************************************************************************/
extern volatile uint8_t plc_rtc_failure;

/*****************************************************************************/
uint32_t plc_rtc_clken_and_check(void)
{
    uint32_t ret;
    /* Insert your code here. */
    return ret ;
}

/*---------------------------------------------------------------------------*/
void plc_rtc_init(tm* time)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void plc_rtc_dt_set(tm* time)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void plc_rtc_dt_get(tm* time)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void _plc_rtc_poll(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void plc_rtc_time_get(IEC_TIME *current_time)
{
    /* Insert your code here. */
}



