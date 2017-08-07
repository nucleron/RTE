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

#define SERIAL_MULTIPORT

#ifndef SERIAL_MULTIPORT

/* ----------------------- Static variables ---------------------------------*/
uint8_t  uartnum=1;
static bool txen = false;
#else

#define txen  inst->txen
#define uartnum inst->uartnum

MBSerialInstance* uart_mb_inst;
MBSerialInstance* uart_mbm_inst;

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
        if(uartnum==USART_MBS_PERIPH)
        {
            gpio_set(USART_MBS_TXEN_PORT, USART_MBS_TXEN_PIN);
        }
        #ifdef USART_MBM
        else if(uartnum==USART_MBM_PERIPH)
        {
                gpio_set(USART_MBM_TXEN_PORT, USART_MBM_TXEN_PIN);
        }
        #endif //


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

    if((ucPort!=MBS_USART)
       #ifdef USART_MBM
       &&(ucPort!=MBM_USART)
       #endif
       ) return false; //we have 2 available uarts for modbus. #2 and #3
    uartnum = ucPort;

    if(uartnum==MBS_USART)
    {
        uartnum = USART_MBS_PERIPH;
        #ifdef SERIAL_MULTIPORT
        uart_mb_inst = inst;
        #endif
        rcc_periph_clock_enable(USART_MBS_RCC_PERIPH);
        rcc_periph_clock_enable(USART_MBS_TX_PERIPH  );
    #if (USART_MBS_TX_PERIPH != USART_MBS_RX_PERIPH)
        rcc_periph_clock_enable(USART_MBS_RX_PERIPH  );
    #endif
    #if (USART_MBS_TX_PERIPH != USART_MBS_RX_PERIPH)
        rcc_periph_clock_enable(USART_MBS_TXEN_PERIPH);
    #endif
        /*Setup TxEN pin*/
        gpio_mode_setup(USART_MBS_TXEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, USART_MBS_TXEN_PIN);
        gpio_set_output_options(USART_MBS_TXEN_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP,USART_MBS_TXEN_PIN);
        gpio_clear(USART_MBS_TXEN_PORT, USART_MBS_TXEN_PIN);
        /*Setup TxD pin*/
        gpio_mode_setup(USART_MBS_TX_PORT,GPIO_MODE_AF, GPIO_PUPD_NONE, USART_MBS_TX_PIN);
        gpio_set_output_options(USART_MBS_TX_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP, USART_MBS_TX_PIN);
        /*Setup RxD pin*/
        gpio_mode_setup(USART_MBS_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_MBS_RX_PIN);

        gpio_set_af(USART_MBS_TX_PORT, GPIO_AF7, USART_MBS_TX_PIN);
        gpio_set_af(USART_MBS_RX_PORT, GPIO_AF7, USART_MBS_RX_PIN);

        /* Enable slave usart interrupt. */
        nvic_enable_irq(USART_MBS_VECTOR);
    }
    #ifdef USART_MBM
    else if(uartnum==MBM_USART)
    {
        uartnum = USART_MBM_PERIPH;
        #ifdef SERIAL_MULTIPORT
        uart_mbm_inst = inst;
        #endif
        rcc_periph_clock_enable(USART_MBM_RCC_PERIPH);
        rcc_periph_clock_enable(USART_MBM_TX_PERIPH  );
    #if (USART_MBM_TX_PERIPH != USART_MBM_RX_PERIPH)
        rcc_periph_clock_enable(USART_MBM_RX_PERIPH  );
    #endif
    #if (USART_MBM_TX_PERIPH != USART_MBM_RX_PERIPH)
        rcc_periph_clock_enable(USART_MBM_TXEN_PERIPH);
    #endif
        /*Setup TxEN pin*/
        gpio_mode_setup(USART_MBM_TXEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, USART_MBM_TXEN_PIN);
        gpio_set_output_options(USART_MBM_TXEN_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP,USART_MBM_TXEN_PIN);
        gpio_clear(USART_MBM_TXEN_PORT, USART_MBM_TXEN_PIN);
        /*Setup TxD pin*/
        gpio_mode_setup(USART_MBM_TX_PORT,GPIO_MODE_AF, GPIO_PUPD_NONE, USART_MBM_TX_PIN);
        gpio_set_output_options(USART_MBM_TX_PORT,GPIO_OSPEED_50MHZ, GPIO_OTYPE_PP, USART_MBM_TX_PIN);
        /*Setup RxD pin*/
        gpio_mode_setup(USART_MBM_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_MBM_RX_PIN);

        gpio_set_af(USART_MBM_TX_PORT, GPIO_AF7, USART_MBM_TX_PIN);
        gpio_set_af(USART_MBM_RX_PORT, GPIO_AF7, USART_MBM_RX_PIN);

        /* Enable master usart interrupt. */
        nvic_enable_irq(USART_MBM_VECTOR);
    }
    #endif

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
    case 7:
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
//    case 7:
//        if (eParity == MB_PAR_NONE)
//        {
//            bStatus = FALSE;
//        }
//        else
//        {
//            usart_set_databits(uartnum,8);
//        }
//        break;
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
    if(uartnum==USART_MBS_PERIPH)
    {
        nvic_disable_irq(USART_MBS_VECTOR);
    }
    #ifdef USART_MBM
    else if(uartnum==USART_MBM_PERIPH)
    {
        nvic_disable_irq(USART_MBS_VECTOR);
    }
    #endif // USART_MBM

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

void USART_MBS_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(USART_MBS_PERIPH) & USART_CR1_RXNEIE) != 0) && ((USART_SR(USART_MBS_PERIPH) & USART_SR_RXNE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart_mb_inst->parent))->parent))->pxMBFrameCBByteReceived((MBRTUInstance*)(uart_mb_inst->parent));
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(USART_MBS_PERIPH) & USART_CR1_TXEIE) != 0) && ((USART_SR(USART_MBS_PERIPH) & USART_SR_TXE) != 0))
    {
       ((MBInstance*)(((MBRTUInstance*)(uart_mb_inst->parent))->parent))->pxMBFrameCBTransmitterEmpty((MBRTUInstance*)(uart_mb_inst->parent));
        /* Check if we need to disable transmitter*/
        if(!uart_mb_inst->txen)
        {
            USART_SR (USART_MBS_PERIPH) &= ~USART_SR_TC;   /* Clear TC flag*/
            USART_CR1(USART_MBS_PERIPH) |= USART_CR1_TCIE; /* Enable transfer complite interrupt*/
        }
    }
    /* Disable transmitter on transfer comlite*/
    if (((USART_CR1(USART_MBS_PERIPH) & USART_CR1_TCIE) != 0) && ((USART_SR(USART_MBS_PERIPH) & USART_SR_TC) != 0))
    {
        USART_CR1(USART_MBS_PERIPH) &= ~USART_CR1_TCIE;/* Disble transfer complete interrupt*/
        USART_SR (USART_MBS_PERIPH) &= ~USART_SR_TC;   /* Clear TC flag*/
        /* Disable transmitter*/
        gpio_clear(USART_MBS_TXEN_PORT, USART_MBS_TXEN_PIN);
    }
}

#ifdef USART_MBM
void USART_MBM_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(USART_MBM_PERIPH) & USART_CR1_RXNEIE) != 0) && ((USART_SR(USART_MBM_PERIPH) & USART_SR_RXNE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart_mbm_inst->parent))->parent))->pxMBFrameCBByteReceived((MBRTUInstance*)(uart_mbm_inst->parent));
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(USART_MBM_PERIPH) & USART_CR1_TXEIE) != 0) && ((USART_SR(USART_MBM_PERIPH) & USART_SR_TXE) != 0))
    {
        ((MBInstance*)(((MBRTUInstance*)(uart_mbm_inst->parent))->parent))->pxMBFrameCBTransmitterEmpty((MBRTUInstance*)(uart_mbm_inst->parent));
        /* Check if we need to disable transmitter*/
        if(!uart_mbm_inst->txen)
        {
            USART_SR (USART_MBM_PERIPH) &= ~USART_SR_TC;   /* Clear TC flag*/
            USART_CR1(USART_MBM_PERIPH) |= USART_CR1_TCIE; /* Enable transfer complete interrupt*/
        }
    }
    /* Disable transmitter on transfer comlete*/
    if (((USART_CR1(USART_MBM_PERIPH) & USART_CR1_TCIE) != 0) && ((USART_SR(USART_MBM_PERIPH) & USART_SR_TC) != 0))
    {
        USART_CR1(USART_MBM_PERIPH) &= ~USART_CR1_TCIE;/* Disble transfer complete interrupt*/
        USART_SR (USART_MBM_PERIPH) &= ~USART_SR_TC;   /* Clear TC flag*/
        /* Disable transmitter*/
        gpio_clear(USART_MBM_TXEN_PORT, USART_MBM_TXEN_PIN);
    }
}

#endif
