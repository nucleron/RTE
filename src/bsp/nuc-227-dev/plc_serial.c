/*
 * This file is based on the libopencm3 project.
 *
 * Copyright (C) 2016 Nucleron R&D LLC (main@nucleron.ru)
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>

#include <plc_dbg.h>
#include <plc_config.h>

static unsigned char usart_data = 0x00;
static dbg_fifo_t usart_rx_buf, usart_tx_buf;

void dbg_serial_init(void)
{
    dbg_fifo_flush( &usart_rx_buf );
    dbg_fifo_flush( &usart_tx_buf );

    rcc_periph_clock_enable( DBG_USART_PERIPH );
    rcc_periph_clock_enable( DBG_USART_TX_PERIPH );
#if (DBG_USART_RX_PERIPH != DBG_USART_TX_PERIPH)
    rcc_periph_clock_enable( DBG_USART_RX_PERIPH );
#endif
    /* Enable the DBG_USART interrupt. */
    nvic_enable_irq(DBG_USART_VECTOR);

    /* Setup GPIO pins for USART transmit. */
    gpio_mode_setup(DBG_USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, DBG_USART_TX_PIN);

    /* Setup GPIO pins for USART receive. */
    gpio_mode_setup(DBG_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DBG_USART_RX_PIN);
    gpio_set_output_options(DBG_USART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, DBG_USART_RX_PIN);

    /* Setup USART TX and RX pin as alternate function. */
    gpio_set_af(DBG_USART_TX_PORT, GPIO_AF7, DBG_USART_TX_PIN);
    gpio_set_af(DBG_USART_RX_PORT, GPIO_AF7, DBG_USART_RX_PIN);

    /* Setup USART parameters. */
    usart_set_baudrate    (DBG_USART, 57600);
    usart_set_databits    (DBG_USART, 8);
    usart_set_stopbits    (DBG_USART, USART_STOPBITS_1);
    usart_set_mode        (DBG_USART, USART_MODE_TX_RX);
    usart_set_parity      (DBG_USART, USART_PARITY_NONE);
    usart_set_flow_control(DBG_USART, USART_FLOWCONTROL_NONE);
    /* Enable USART Receive interrupt. */
    usart_enable_rx_interrupt(DBG_USART);

    /* Finally enable the USART. */
    usart_enable(DBG_USART);
}

void DBG_USART_ISR(void)
{
    /* Check if we were called because of RXNE. */
    if (((USART_CR1(DBG_USART) & USART_CR1_RXNEIE) != 0) && ((USART_SR(DBG_USART) & USART_SR_RXNE) != 0))
    {
        usart_data = usart_recv(DBG_USART);
        if( !dbg_fifo_write_byte( &usart_rx_buf, usart_data ) )
        {
            usart_disable_rx_interrupt(DBG_USART);
        }
    }
    /* Check if we were called because of TXE. */
    if (((USART_CR1(DBG_USART) & USART_CR1_TXEIE) != 0) && ((USART_SR(DBG_USART) & USART_SR_TXE) != 0))
    {
        /* Put data into the transmit register. */
        if( dbg_fifo_read_byte( &usart_tx_buf, &usart_data ) )
        {
            usart_send(DBG_USART, usart_data);
        }
        else
        {
            /* Disable the TXE interrupt as we don't need it anymore. */
            usart_disable_tx_interrupt(DBG_USART);
        }
    }
}

int dbg_serial_write( unsigned char *d, unsigned short n )
{
    int res = 0;
    cm_disable_interrupts();
    res = dbg_fifo_write( &usart_tx_buf, d, n );
    if( res && !(USART_CR1(DBG_USART) & USART_CR1_TXEIE) )
    {
        if( dbg_fifo_read_byte( &usart_tx_buf, &usart_data ) )
        {
            while( !(USART_SR(DBG_USART) & USART_SR_TXE) );///пока буфер не пуст
            usart_send(DBG_USART, usart_data);
            usart_enable_tx_interrupt(DBG_USART);
        }
    }
    cm_enable_interrupts();
    return res;
}

int dbg_serial_read( unsigned char *d, unsigned short n )
{
    int res;
    res = 0;
    while( n-- )
    {
        cm_disable_interrupts();
        if( dbg_fifo_read_byte( &usart_rx_buf, d ) )
        {
            cm_enable_interrupts();
            d++;
            res++;
        }
        else
        {
            cm_enable_interrupts();
            break;
        }
    }
    return res;
}
