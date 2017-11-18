/******************************************************************************
*           This file contains debug serial port related stuff                *
*You can use src/bsp/common/stm32/f2/plc_serial.c as reference implementation.*
*                                                                             *
*    Debug serial port is used to connect PLC with Host which is running IDE. *
* We use full duplex serial link for the debug. Two FIFOs are used to transfer*
* the data in each direction.                                                 *
*                                                                             *
*    This file should be used as Low level part of PLC side debuger. I must   *
* contain debug UART driver with the following API.                           *
******************************************************************************/

/*****************************************************************************/
/* Platform specific includes */

/* STM32 example
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
*/

/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_dbg.h>      /*See this file for dbg_fifo API.*/

/*  FIFOS or TX and RX */
static dbg_fifo_t usart_rx_buf, usart_tx_buf;

/*****************************************************************************/
/* This function is used to setup the debug serial port at the device reset. */
void dbg_serial_init(void)
{
    /* First init FIFOs. */
    dbg_fifo_flush(&usart_rx_buf);
    dbg_fifo_flush(&usart_tx_buf);

    /* Then init serial port. */

    /* Insert your code here. */
}

/*---------------------------------------------------------------------------*/
/* This function is used to write data to usart_tx_buf. */
int dbg_serial_write(unsigned char *d, unsigned short n)
{
    int res = 0;
    PLC_DISABLE_INTERRUPTS();
    /* Insert your code here. */
    /* dbg_fifo_write_byte, and dbg_fifo_write can be used here.*/
    PLC_ENABLE_INTERRUPTS();
    return res; /*Return number of bytes written.*/
}

/* This function is used to read data from usart_rx_buf. */
int dbg_serial_read(unsigned char *d, unsigned short n)
{
    int res = 0;
    PLC_DISABLE_INTERRUPTS();
    /* Insert your code here. */
    /* dbg_fifo_read_byte, and dbg_read_write can be used here.*/
    PLC_ENABLE_INTERRUPTS();
    return res; /*Return number of bytes red.*/
}
