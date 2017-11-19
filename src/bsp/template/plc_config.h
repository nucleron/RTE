
/*
*  Template for YAPLC configuration!
*/

/******************************************************************************
*                                                                             *
******************************************************************************/
#ifndef _PLC_CONFIG_H_
#define _PLC_CONFIG_H_
/******************************************************************************
*                             Include section                                 *
*******************************************************************************
*                Place global project includes here, e.g.:                    *
*******************************************************************************
* #include <libopencm3/stm32/rcc.h>                                           *
* #include <libopencm3/cm3/cortex.h>                                          *
******************************************************************************/

/*Place your includes here!*/


/******************************************************************************
*                         Definition section                                  *
******************************************************************************/

/*Following two defines are used havily so you must...*/
#define PLC_DISABLE_INTERRUPTS  /*place your code here*/
#define PLC_ENABLE_INTERRUPTS   /*and place your code here*/


/******************************************************************************
*                                                                             *
* We need some hardware resources for RTE/softPLC to work porperly, they are: *
*     1. RTC (needed for IEC STD LIB)                                         *
*     2. Periodic interrupt source to count time for IOM sheduing and delays  *
*     3. Periodic interrupt source to call softPLC code                       *
*                                                                             *
*     4. Debug UART for IDE connection                                        *
*                                                                             *
*     5. Retain memory (e.g. EEPROM)                                          *
*                                                                             *
*     6. Jumpers or emulation                                                 *
*                                                                             *
*     7. Boot pin, or emulation                                               *
*                                                                             *
*******************************************************************************
*                                                                             *
*    RTC and periodic interrupt sources may be emulated, so at least we need  *
* one hardare timer in which we can count time for RTC, and other timers.     *
*                                                                             *
*  See plc_backup.h/c, plc_rtc.h/c, plc_tick.h/c for reference.               *
*                                                                             *
*******************************************************************************
*                                                                             *
*    Debug serial is needed to connect the device to the IDE for softPLC load *
* and debug processes, debug FIFO is used for communication.                  *
*                                                                             *
*    Usart or any other serial duplex port may be used as debug port.         *
*                                                                             *
*    See plc_dbg.h/c, plc_serial.c for details.                               *
*                                                                             *
*******************************************************************************
*                                                                             *
*    Retain memory is used to store soltPLCs retain variables.                *
*                                                                             *
*    See plc_backup.h/c for details.                                          *
*                                                                             *
*******************************************************************************
*                                                                             *
*    We use two jumpers:                                                      *
*      1. Reset jumper which is used to clear the retain memory and RTC date. *
*      2. Debug jumper which is used to force debug mode on system reset.     *
*                                                                             *
*                                                                             *
*   Specify jumper related stuff here, or just make stubs                     *
* for jumper related functions.                                               *
*                                                                             *
*    See main.c and plc_hw.c for details.                                     *
*                                                                             *
*                                                                             *
*******************************************************************************
*                                                                             *
*   Boot pin is used to force MCU to enter debuf mode on reset as we use      *
* STM32 interbnal bootloader.                                                 *
*                                                                             *
*   If you have your bootloader code inside the RTE,                          *
* then you may skip Boot pin configuration!                                   * 
*                                                                             *
*    See main.c and plc_hw.c for details.                                     *
*                                                                             *
******************************************************************************/
/*
*  PLC tick min period (ns)
*/
#define PLC_TICK_MIN_PER        /*Specify minimal cycle period here!*/


/******************************************************************************
*                                                                             *
*                Add more project global definitions here!!!                  *
*                                                                             *
******************************************************************************/
#endif /* _PLC_CONFIG_H_ */
