/******************************************************************************
*              This file contains PLC clock related stuff                     *
*You can use src/bsp/common/stm32/f2/plc_clock.c as reference implementation. *
******************************************************************************/
/* Standard includes are placed here*/
#include <stdbool.h>

/* Platform specific includes should be places here*/
/*
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
*/

/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_clock.h>

/*****************************************************************************/
/* Add project specific includes here. */

/*****************************************************************************/
/*
This is clock setup function.
It is responsible for setting up main MCU clock frequency.

If you MCU has somethicng like "clock sequrity system",
we recomend you to setup it here.
*/
void plc_clock_setup(void)
{
    /*Insert your code here.*/
}

/*
You may need to add aditional functions here,
e.g. interrupt handler for the case of main clock fail, so...
*/
/*Insert your code here, if needed.*/
