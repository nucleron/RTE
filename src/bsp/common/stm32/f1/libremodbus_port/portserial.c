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
 * File: $Id: portserial.c, v 1.1 2006/08/22 21:35:13 wolti Exp $
 */
/* ----------------------- Platform includes --------------------------------*/
#include <stdbool.h>

/* ----------------------- libopencm3 STM32F includes -------------------------------*/
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include <mb.h>

#include <plc_config.h>

mb_port_ser mbs_inst_usart;
/* ----------------------- Enable USART interrupts -----------------------------*/
void
vMBPortSerialEnable(mb_port_ser* inst, BOOL xRxEnable, BOOL xTxEnable)
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
    if (xRxEnable)
    {
        inst->tx_en = false;
        usart_enable_rx_interrupt(inst->uart_num);
    }
    else
    {
        usart_disable_rx_interrupt(inst->uart_num);
    }

    if (xTxEnable)
    {
        inst->tx_en = true;
        if (inst->uart_num==MB_USART)
        {
            gpio_set(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
        }

        usart_enable_tx_interrupt(inst->uart_num);
    }
    else
    {
        inst->tx_en = false;
        usart_disable_tx_interrupt(inst->uart_num);
    }
}

/* ----------------------- Initialize USART ----------------------------------*/
/* Called with databits = 8 for RTU */

BOOL xMBPortSerialInit(mb_port_ser* inst, ULONG baud, UCHAR ucDataBits,
                   mb_parity_enum parity)
{
    BOOL bStatus;
    if ((&mbs_inst_usart) != inst)
    {
        return false;
    }

    inst->uart_num = MB_USART;

    rcc_periph_clock_enable(RCC_AFIO           );
    rcc_periph_clock_enable(MB_USART_PERIPH    );
    rcc_periph_clock_enable(MB_USART_TX_PERIPH );
#if (MB_USART_TX_PERIPH != MB_USART_RX_PERIPH)
    rcc_periph_clock_enable(MB_USART_RX_PERIPH );
#endif
#if (MB_USART_TX_PERIPH != MB_USART_RX_PERIPH)
    rcc_periph_clock_enable(MB_USART_TXEN_PERIPH);
#endif
    /*Setup TxEN pin*/
    gpio_set_mode(MB_USART_TXEN_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, MB_USART_TXEN_PIN);
    gpio_clear(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
    /*Setup TxD pin*/
    gpio_set_mode(MB_USART_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, MB_USART_TX_PIN);
    /*Setup RxD pin*/
    gpio_set_mode(MB_USART_RX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, MB_USART_RX_PIN);
    /* Enable the MB_USART interrupt. */
    nvic_enable_irq(MB_USART_VECTOR);
    /* Setup UART parameters. */
    usart_set_baudrate    (MB_USART, baud           );
    usart_set_stopbits    (MB_USART, USART_STOPBITS_1     );
    usart_set_flow_control(MB_USART, USART_FLOWCONTROL_NONE);
    usart_set_mode        (MB_USART, USART_MODE_TX_RX     );

    bStatus = TRUE;
    switch (parity)
    {
    case MB_PAR_NONE:
        usart_set_parity(MB_USART, USART_PARITY_NONE);
        break;
    case MB_PAR_ODD:
        usart_set_parity(MB_USART, USART_PARITY_ODD);
        break;
    case MB_PAR_EVEN:
        usart_set_parity(MB_USART, USART_PARITY_EVEN);
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
        if (parity == MB_PAR_NONE)
        {
            wordLength = 8;
        }
        else
        {
            wordLength = 9;
        }
        usart_set_databits(inst->uart_num, wordLength);
        break;
//    case 7:
//        if (parity == MB_PAR_NONE)
//        {
//            bStatus = FALSE;
//        }
//        else
//        {
//            usart_set_databits(inst->uart_num, 8);
//        }
//        break;
    default:
        bStatus = FALSE;
    }

    if (bStatus == TRUE)
    {
        /* Finally enable the USART. */
        usart_disable_rx_interrupt(MB_USART);
        usart_disable_tx_interrupt(MB_USART);
        usart_enable(MB_USART);
    }
    return bStatus;
}


/* -----------------------Send character  ----------------------------------*/
BOOL
xMBPortSerialPutByte(mb_port_ser* inst, CHAR ucByte)
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty() has been
     * called. */
    usart_send(inst->uart_num, ucByte);
    return TRUE;
}

/* ----------------------- Get character ----------------------------------*/
BOOL
xMBPortSerialGetByte(mb_port_ser* inst, CHAR * pucByte)
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived() has been called.
     */
    *pucByte = (CHAR)usart_recv(inst->uart_num);
    return TRUE;
}
/* ----------------------- Close Serial Port ----------------------------------*/
void
vMBPortSerialClose(mb_port_ser* inst)
{
    if ((&mbs_inst_usart) != inst)
    {
        return;
    }
    nvic_disable_irq(MB_USART_VECTOR);
    usart_disable   (MB_USART);
}

/* ----------------------- USART ISR ----------------------------------*/
/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty() which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte() to send the character.
 */
/* Find out what interrupted and get or send data as appropriate */
void MB_USART_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(MB_USART) & USART_CR1_RXNEIE) != 0) && ((USART_SR(MB_USART) & USART_SR_RXNE) != 0))
    {
        mbs_inst_usart.base.cb->byte_rcvd(mbs_inst_usart.base.arg);
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(MB_USART) & USART_CR1_TXEIE) != 0) && ((USART_SR(MB_USART) & USART_SR_TXE) != 0))
    {
        mbs_inst_usart.base.cb->tx_empty(mbs_inst_usart.base.arg);
        /* Check if we need to disable transmitter*/
        if (!mbs_inst_usart.tx_en)
        {
            USART_SR (MB_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
            USART_CR1(MB_USART) |= USART_CR1_TCIE; /* Enable transfer complite interrupt*/
        }
    }
    /* Disable transmitter on transfer comlite*/
    if (((USART_CR1(MB_USART) & USART_CR1_TCIE) != 0) && ((USART_SR(MB_USART) & USART_SR_TC) != 0))
    {
        USART_CR1(MB_USART) &= ~USART_CR1_TCIE;/* Disble transfer complite interrupt*/
        USART_SR (MB_USART) &= ~USART_SR_TC;   /* Clear TC flag*/
        /* Disable transmitter*/
        gpio_clear(MB_USART_TXEN_PORT, MB_USART_TXEN_PIN);
    }
}

