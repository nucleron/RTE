/******************************************************************************
*                                                                             *
******************************************************************************/

/*
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
*/

/*****************************************************************************/
#include <plc_config.h>
#include <plc_tick.h>

/*****************************************************************************/
uint64_t plc_tick_time      = PLC_TICK_MIN_PER;
volatile bool plc_tick_flag = false;

/*****************************************************************************/
void plc_tick_init(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void plc_tick_poll(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
void sys_tick_handler(void)
{
    //Soft watchdog
    if (plc_tick_flag)
    {
        /* Insert your code here. */
    }
    else
    {
        plc_tick_flag = true;
    }
}

/*---------------------------------------------------------------------------*/
//Tick period in ns
void plc_tick_setup(uint64_t tick_next, uint64_t tick_period)
{
    (void)tick_next;//disable warning
    (void)tick_period;//disable warning
    /* Insert your code here. */
}
