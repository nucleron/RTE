/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Platform includes --------------------------------*/
#include <stdbool.h>
#include "serial_port.h"
#include "serial_multi.h"

/* ----------------------- libopencm3 STM32F includes -------------------------------*/
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"

#include <plc_config.h>


#ifndef SERIAL_MULTIPORT

/* ----------------------- Static variables ---------------------------------*/
uint8_t  uartnum=1;
static bool txen = false;
#else

#define txen  inst->txen
#define uartnum inst->uartnum

MBSerialInstance* uart2_inst;
MBSerialInstance* uart3_inst;

#endif
/* ----------------------- Enable USART interrupts -----------------------------*/
void
vMBPortSerialEnable(MULTIPORT_SERIAL_ARG BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
    if (xRxEnable)
    {
        txen = false;
        usart_enable_rx_interrupt(uartnum);
    }
    else
    {
        usart_disable_rx_interrupt(uartnum);
    }

    if (xTxEnable)
    {
        txen = true;
        if(uartnum==USART2)
            gpio_set(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
        else if(uartnum==USART3)
            gpio_set(MBM_USART_TXEN_PORT, MBM_USART_TXEN_PIN);

        usart_enable_tx_interrupt(uartnum);
    }
    else
    {
        txen = false;
        usart_disable_tx_interrupt(uartnum);
    }
}

/* ----------------------- Initialize USART ----------------------------------*/
/* Called with databits = 8 for RTU */

BOOL
xMBPortSerialInit(MULTIPORT_SERIAL_ARG UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits,
                   eMBParity eParity )
{
    BOOL bStatus;

    if((ucPort!=2)&&(ucPort!=3)) return false; //we have 2 available uarts for modbus. #1 and #2
    uartnum = ucPort;

    if(uartnum==2)
    {
        uartnum = USART2;
        #ifdef SERIAL_MULTIPORT
        uart2_inst = inst;
        #endif
        //rcc_periph_clock_enable(RCC_AFIO            ); //FIXME
        rcc_periph_clock_enable(MB_USART_PERIPH     );
        rcc_periph_clock_enable(MB_USART_TX_PERIPH  );
    #if (MB_USART_TX_PERIPH != MB_USART_RX_PERIPH)
        rcc_periph_clock_enable(MB_USART_RX_PERIPH  );
    #endif
    #if (MB_USART_TX_PERIPH != MB_USART_RX_PERIPH)
        rcc_periph_clock_enable(MB_USART_TXEN_PERIPH);
    #endif
        /*Setup TxEN pin*/
        gpio_mode_setup(MB_USART_TXEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MB_USART_TXEN_PIN);
        gpio_set_output_options(MB_USART_TXEN_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP,MB_USART_TXEN_PIN);
        gpio_clear(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
        /*Setup TxD pin*/
        gpio_mode_setup(MB_USART_TX_PORT,GPIO_MODE_AF, GPIO_PUPD_NONE, MB_USART_TX_PIN);
        gpio_set_output_options(MB_USART_TX_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP, MB_USART_TX_PIN);
        /*Setup RxD pin*/
        gpio_mode_setup(MB_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MB_USART_RX_PIN);

        gpio_set_af(MB_USART_TX_PORT, GPIO_AF7, MB_USART_TX_PIN);
        gpio_set_af(MB_USART_RX_PORT, GPIO_AF7, MB_USART_RX_PIN);

        /* Enable the MB_USART interrupt. */
        nvic_enable_irq(MB_USART_VECTOR);
    }
    else if(uartnum==3)
    {
        uartnum = USART3;
        #ifdef SERIAL_MULTIPORT
        uart3_inst = inst;
        #endif
        //rcc_periph_clock_enable(RCC_AFIO            ); //FIXME
        rcc_periph_clock_enable(MBM_USART_PERIPH     );
        rcc_periph_clock_enable(MBM_USART_TX_PERIPH  );
    #if (MBM_USART_TX_PERIPH != MBM_USART_RX_PERIPH)
        rcc_periph_clock_enable(MBM_USART_RX_PERIPH  );
    #endif
    #if (MBM_USART_TX_PERIPH != MBM_USART_RX_PERIPH)
        rcc_periph_clock_enable(MBM_USART_TXEN_PERIPH);
    #endif
        /*Setup TxEN pin*/
        gpio_mode_setup(MBM_USART_TXEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MBM_USART_TXEN_PIN);
        gpio_set_output_options(MBM_USART_TXEN_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP,MBM_USART_TXEN_PIN);
        gpio_clear(MBM_USART_TXEN_PORT, MBM_USART_TXEN_PIN);
        /*Setup TxD pin*/
        gpio_mode_setup(MBM_USART_TX_PORT,GPIO_MODE_AF, GPIO_PUPD_NONE, MBM_USART_TX_PIN);
        gpio_set_output_options(MBM_USART_TX_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP, MBM_USART_TX_PIN);
        /*Setup RxD pin*/
        gpio_mode_setup(MBM_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MBM_USART_RX_PIN);

        gpio_set_af(MBM_USART_TX_PORT, GPIO_AF7, MBM_USART_TX_PIN);
        gpio_set_af(MBM_USART_RX_PORT, GPIO_AF7, MBM_USART_RX_PIN);

        /* Enable the MB_USART interrupt. */
        nvic_enable_irq(MBM_USART_VECTOR);
    }

    /* Setup UART parameters. */
    usart_set_baudrate    (uartnum, ulBaudRate            );
    usart_set_stopbits    (uartnum, USART_STOPBITS_1      );
    usart_set_flow_control(uartnum, USART_FLOWCONTROL_NONE);
    usart_set_mode        (uartnum, USART_MODE_TX_RX      );

    bStatus = TRUE;
    switch (eParity)
    {
    case MB_PAR_NONE:
        usart_set_parity(uartnum, USART_PARITY_NONE);
        break;
    case MB_PAR_ODD:
        usart_set_parity(uartnum, USART_PARITY_ODD);
        break;
    case MB_PAR_EVEN:
        usart_set_parity(uartnum, USART_PARITY_EVEN);
        break;
    default:
        bStatus = FALSE;
        break;
    }

    /* Oddity of STM32F series: word length includes parity. 7 bits no parity
    not possible */
    CHAR wordLength;
    switch (ucDataBits)
    {
    case 8:
        if (eParity == MB_PAR_NONE)
        {
            wordLength = 8;
        }
        else
        {
            wordLength = 9;
        }
        usart_set_databits(uartnum,wordLength);
        break;
    case 7:
        if (eParity == MB_PAR_NONE)
        {
            bStatus = FALSE;
        }
        else
        {
            usart_set_databits(uartnum,8);
        }
        break;
    default:
        bStatus = FALSE;
    }

    if( bStatus == TRUE )
    {
        /* Finally enable the USART. */
        usart_disable_rx_interrupt(uartnum);
        usart_disable_tx_interrupt(uartnum);
        usart_enable(uartnum);
    }
    return bStatus;
}

/* -----------------------Send character  ----------------------------------*/
BOOL
xMBPortSerialPutByte(MULTIPORT_SERIAL_ARG CHAR ucByte)
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
    usart_send(uartnum, ucByte);
    return TRUE;
}

/* ----------------------- Get character ----------------------------------*/
BOOL
xMBPortSerialGetByte(MULTIPORT_SERIAL_ARG CHAR * pucByte)
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
    *pucByte = (CHAR)usart_recv(uartnum);
    return TRUE;
}

/* ----------------------- Close Serial Port ----------------------------------*/
void
vMBPortSerialClose ( MULTIPORT_SERIAL_ARG_VOID )
{
    nvic_disable_irq(MB_USART_VECTOR);
    usart_disable   (uartnum);
}

/* ----------------------- USART ISR ----------------------------------*/
/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
/* Find out what interrupted and get or send data as appropriate */

#undef txen

void MB_USART_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(MB_USART) & USART_CR1_RXNEIE) != 0) && ((USART_SR(MB_USART) & USART_SR_RXNE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart2_inst->parent))->parent))->pxMBFrameCBByteReceived((MBRTUInstance*)(uart2_inst->parent));
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(MB_USART) & USART_CR1_TXEIE) != 0) && ((USART_SR(MB_USART) & USART_SR_TXE) != 0))
    {
       ((MBInstance*)(((MBRTUInstance*)(uart2_inst->parent))->parent))->pxMBFrameCBTransmitterEmpty((MBRTUInstance*)(uart2_inst->parent));
        /* Check if we need to disable transmitter*/
        if(!uart2_inst->txen)
        {
            USART_SR (MB_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
            USART_CR1(MB_USART) |= USART_CR1_TCIE; /* Enable transfer complite interrupt*/
        }
    }
    /* Disable transmitter on transfer comlite*/
    if (((USART_CR1(MB_USART) & USART_CR1_TCIE) != 0) && ((USART_SR(MB_USART) & USART_SR_TC) != 0))
    {
        USART_CR1(MB_USART) &= ~USART_CR1_TCIE;/* Disble transfer complete interrupt*/
        USART_SR (MB_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
        /* Disable transmitter*/
        gpio_clear(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
    }
}

void MBM_USART_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(MBM_USART) & USART_CR1_RXNEIE) != 0) && ((USART_SR(MBM_USART) & USART_SR_RXNE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart3_inst->parent))->parent))->pxMBFrameCBByteReceived((MBRTUInstance*)(uart3_inst->parent));
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(MBM_USART) & USART_CR1_TXEIE) != 0) && ((USART_SR(MBM_USART) & USART_SR_TXE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart3_inst->parent))->parent))->pxMBFrameCBTransmitterEmpty((MBRTUInstance*)(uart3_inst->parent));
        /* Check if we need to disable transmitter*/
        if(!uart3_inst->txen)
        {
            USART_SR (MBM_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
            USART_CR1(MBM_USART) |= USART_CR1_TCIE; /* Enable transfer complete interrupt*/
        }
    }
    /* Disable transmitter on transfer comlete*/
    if (((USART_CR1(MBM_USART) & USART_CR1_TCIE) != 0) && ((USART_SR(MBM_USART) & USART_SR_TC) != 0))
    {
        USART_CR1(MBM_USART) &= ~USART_CR1_TCIE;/* Disble transfer complete interrupt*/
        USART_SR (MBM_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
        /* Disable transmitter*/
        gpio_clear(MBM_USART_TXEN_PORT, MBM_USART_TXEN_PIN);
    }
}

