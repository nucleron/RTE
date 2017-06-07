/*
 * Copyright Nucleron R&D LLC 2016
 */
#ifndef _PLC_CONFIG_H_
#define _PLC_CONFIG_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/cortex.h>

#define PLC_DISABLE_INTERRUPTS cm_disable_interrupts
#define PLC_ENABLE_INTERRUPTS  cm_enable_interrupts

#define DBG_USART USART3
#define DBG_USART_PERIPH RCC_USART3
#define DBG_USART_VECTOR NVIC_USART3_IRQ
#define DBG_USART_ISR usart3_isr

#define DBG_USART_TX_PORT GPIOB
#define DBG_USART_RX_PORT GPIOB

#define DBG_USART_TX_PIN GPIO10
#define DBG_USART_RX_PIN GPIO11

#define DBG_USART_TX_PERIPH RCC_GPIOB
#define DBG_USART_RX_PERIPH RCC_GPIOB

#define PLC_BOOT_PERIPH RCC_GPIOD
#define PLC_BOOT_PORT GPIOD
#define PLC_BOOT_PIN GPIO10

#define PLC_WAIT_TMR_PERIPH RCC_TIM7
#define PLC_WAIT_TMR TIM7
#define PLC_WAIT_TMR_VECTOR NVIC_TIM7_IRQ
#define PLC_WAIT_TMR_ISR tim7_isr

#define PLC_BKP_VER1_OFFSET      0
#define PLC_BKP_VER2_OFFSET      4
#define PLC_BKP_RTC_IS_OK_OFFSET 8

#include <plc_glue_dbg.h>

#endif /* _PLC_CONFIG_H_ */
