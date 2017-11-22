/******************************************************************************
*              This file contains PLC tick related stuff                      *
*   You can use src/bsp/common/plc_tick.c as reference implementation.        *
******************************************************************************/

/* Platform specific includes should be places here*/
/*
STM32 example
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
*/


/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_tick.h>

/*****************************************************************************/
/* Add project specific includes here. */


/*****************************************************************************/
/* Variables */
uint64_t plc_tick_time = PLC_TICK_MIN_PER; /*softPLC tick time in nanoseconds*/

/*
This flag is set by periodic timer interrupt when softPLC must be called.   
This should happen every "plc_tick_time" nanoseconds.
*/
volatile bool plc_tick_flag = false;

/*****************************************************************************/
/*
This function sets up soft/hardware needed to generte ticks.
It is called on the device reset.
*/
void plc_tick_init(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function is called in "main" loop. 

Actually it is empty now and it's likely to bedeprecated in next 
YAPLC releases.
*/
void plc_tick_poll(void)
{
}

/*---------------------------------------------------------------------------*/
/* 
This function must be called every "plc_tick_time" nanoseconds by 
priodic timer interrupt handler.

Or it can be priodic timer interrupt handler itself.

Feel free to rename this function as you need!
*/
void please_rename_me_tick_handler(void)
{
    //Soft watchdog
    if (plc_tick_flag)
    {
        /*
        If "plc_tick_flag" wasn't cleared by "main" function,
        then softPLC didn't met deadline, this is error
        which should be handled.
        */
        /* Insert your code here. */
    }
    else
    {
        /*
        softPLC should be called
        */
        plc_tick_flag = true;
        /* Insert your code here if needed. */
    }
    /* Insert your code here if needed. */
}

/*---------------------------------------------------------------------------*/
/*
This function is called by softPLC on start. 
And may be called from "align_tick" function,
see next comments.
*/
void plc_tick_setup(uint64_t tick_next, uint64_t tick_period)
{
    /*
    Do next tick in "tick_next" nanoseconds.
    
    This parameter may be leaved unused, if you won't use "align_tick"
    function, see beremiz/targets/plc_main_tail.c for details. 

    We don't use it in RTE code.
    */
    (void)tick_next;//disable warning
    /*
    Do tick every "tick_period" nanoseconds, or "PLC_TICK_MIN_PER" nanoseconds
    if "tick_period" is too small.

    This is mandatory parameter.
    */
    (void)tick_period;//disable warning

    /* Insert your code here. */
}
