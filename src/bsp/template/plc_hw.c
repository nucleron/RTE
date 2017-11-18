
/******************************************************************************
*          This file contains Boot pin and Jumpers related stuff.             *
******************************************************************************/

/* Standard includes */
#include <stdbool.h>

/*****************************************************************************/
/* Platform specific includes */

/* STM32 example
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
*/

/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_abi.h>
#include <plc_iom.h>
#include <plc_dbg.h>
#include <plc_wait_tmr.h>
#include <plc_hw.h>


/*****************************************************************************/
/* Jumper related stuff. */

/*---------------------------------------------------------------------------*/
/*
This function is used to setup reset and debug jumpers at device reset.
*/
void plc_jmpr_init(void)
{
     /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/* Get debug jumper */
bool plc_dbg_jmpr_get(void)
{
    /* Insert your code here. */
    return false; /*Default*/
}

/*---------------------------------------------------------------------------*/
/* Get reset jumper */
bool plc_rst_jmpr_get(void)
{
    /* Insert your code here. */
    return false; /*Default*/
}


/*****************************************************************************/
/* Boot mode related stuff */

/*---------------------------------------------------------------------------*/
/*
This function is used to setup the device bootloader at the device reset.
*/
void plc_boot_init(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/* 
This function is used to enter boot mode. 

Ir must transfer the controll to the bootloader.

The bootloader must restart RTE after softPLC load.
*/
void plc_boot_mode_enter(void)
{
    /* Insert your code here. */
}


/******************************************************************************
* Deprecated stuff. Will be removed at YAPLC-3.0.0 release.                   *
* Discrete and Analog IO is handled by IO manager, see plc_iom.c/h for details*
******************************************************************************/
/*---------------------------------------------------------------------------*/
bool plc_get_din(uint32_t i)
{
    return false;
}

/*---------------------------------------------------------------------------*/
bool plc_get_dout(uint32_t i)
{
    return false;
}

/*---------------------------------------------------------------------------*/
void plc_set_dout(uint32_t i, bool val)
{
    (void)i;
    (void)val;
}

/*---------------------------------------------------------------------------*/
uint32_t plc_get_ain(uint32_t i)
{
    (void)i;
    return 0;
}

/*---------------------------------------------------------------------------*/
void plc_set_aout(uint32_t i, uint32_t val)
{
    (void)i;
    (void)val;
}
