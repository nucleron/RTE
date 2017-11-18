/******************************************************************************
*              This file contains retain memory related stuff                 *
*You can use src/bsp/common/stm32/f2/plc_backup.c as reference implementation.*
******************************************************************************/

/* Standard includes needed for memcpy and friends */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*****************************************************************************/
/* Platform specific includes should be places here*/

/*
STM32 example:
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/f1/bkp.h>
*/

/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_backup.h>

/*****************************************************************************/
/* Retain memory implementation */

/*---------------------------------------------------------------------------*/
/*
This function is used to setup backup/retain memory 
after the device reset.
*/
void plc_backup_init(void)
{
     /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function is used whrn we want to reset RTC date/time 
and backup/retain memory content.
*/
void plc_backup_reset(void) //used to reset
{
     /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function gets called by Beremiz generated __publish_debug before softPLC 
starts to save retain variables.
*/
void plc_backup_invalidate(void)
{
     /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function gets called by Beremiz generated __publish_debug after softPLC 
has saved retain variables.
*/
void plc_backup_validate(void)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function gets called by Beremiz generated __init_debug when softPLC 
gets started.

It must return 1 if there is a valid retain memory bank, 
else it must rturn 0 
*/
int plc_backup_check(void)
{
    /* Insert your code here. */
    return 0; /*No valid retains present by default.*/ 
}

/*---------------------------------------------------------------------------*/
/*
This function gets called by Beremiz generated __init_debug when softPLC 
gets started.

It must read backup/retain memory content and place it to "p" address.

"Count" bytes are restored from "retain_memory_start + offset"
address.
*/
void plc_backup_remind(unsigned int offset, unsigned int count, void *p)
{
    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/*
This function gets called by Beremiz generated __publish_debug when softPLC 
saves retain variables.

It must read softPLC data from "p" address and place it to backup/retain memory.

"Count" bytes are stored at "retain_memory_start + offset" address.
*/
void plc_backup_retain(unsigned int offset, unsigned int count, void *p)
{
    /* Insert your code here. */
}
