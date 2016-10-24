/*
 * Copyright Nucleron R&D LLC 2016
 */

#ifndef _PLC_CONFIG_H_
#define _PLC_CONFIG_H_

/*
*  NUC-227-DEV configuration!
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/cortex.h>

#define PLC_DISABLE_INTERRUPTS cm_disable_interrupts
#define PLC_ENABLE_INTERRUPTS  cm_enable_interrupts

/*
*  PLC clocks
*/
#define PLC_HSE_CONFIG rcc_hse_16mhz_3v3
#define PLC_RCC_AHB_FREQ 168

/*
*  Debug USART
*/
#define DBG_USART USART1
#define DBG_USART_PERIPH RCC_USART1
#define DBG_USART_VECTOR NVIC_USART1_IRQ
#define DBG_USART_ISR usart1_isr

#define DBG_USART_TX_PORT GPIOA
#define DBG_USART_RX_PORT GPIOA

#define DBG_USART_TX_PIN GPIO9
#define DBG_USART_RX_PIN GPIO10

#define DBG_USART_TX_PERIPH RCC_GPIOA
#define DBG_USART_RX_PERIPH RCC_GPIOA

/*
*  Boot pin
*/
#define PLC_BOOT_PERIPH RCC_GPIOD
#define PLC_BOOT_PORT GPIOD
#define PLC_BOOT_PIN GPIO10

/*
*  PLC LEDS
*/
#define PLC_LED_STG_PERIPH RCC_GPIOB
#define PLC_LED_STG_PORT GPIOB
#define PLC_LED_STG_PIN GPIO12

#define PLC_LED_STR_PERIPH RCC_GPIOB
#define PLC_LED_STR_PORT GPIOB
#define PLC_LED_STR_PIN GPIO13

#define PLC_LED3_PERIPH RCC_GPIOB
#define PLC_LED3_PORT GPIOB
#define PLC_LED3_PIN GPIO14

extern void plc_heart_beat(void);

#define PLC_BLINK() plc_heart_beat()

/*
* PLC Inputs
*/
#define PLC_I1_PERIPH RCC_GPIOE
#define PLC_I1_PORT GPIOE
#define PLC_I1_PIN GPIO0

#define PLC_I2_PERIPH RCC_GPIOB
#define PLC_I2_PORT GPIOB
#define PLC_I2_PIN GPIO8

#define PLC_I3_PERIPH RCC_GPIOB
#define PLC_I3_PORT GPIOB
#define PLC_I3_PIN GPIO6

#define PLC_I4_PERIPH RCC_GPIOB
#define PLC_I4_PORT GPIOB
#define PLC_I4_PIN GPIO4

#define PLC_I5_PERIPH RCC_GPIOD
#define PLC_I5_PORT GPIOD
#define PLC_I5_PIN GPIO6

#define PLC_I6_PERIPH RCC_GPIOD
#define PLC_I6_PORT GPIOD
#define PLC_I6_PIN GPIO4

#define PLC_I7_PERIPH RCC_GPIOD
#define PLC_I7_PORT GPIOD
#define PLC_I7_PIN GPIO5

#define PLC_I8_PERIPH RCC_GPIOD
#define PLC_I8_PORT GPIOD
#define PLC_I8_PIN GPIO7

/*
* PLC Outputs
*/
#define PLC_O1_PERIPH RCC_GPIOE
#define PLC_O1_PORT GPIOE
#define PLC_O1_PIN GPIO1

#define PLC_O2_PERIPH RCC_GPIOB
#define PLC_O2_PORT GPIOB
#define PLC_O2_PIN GPIO9

#define PLC_O3_PERIPH RCC_GPIOB
#define PLC_O3_PORT GPIOB
#define PLC_O3_PIN GPIO7

#define PLC_O4_PERIPH RCC_GPIOB
#define PLC_O4_PORT GPIOB
#define PLC_O4_PIN GPIO5

/*
*  PLC system timer
*/
#define PLC_WAIT_TMR_PERIPH RCC_TIM7
#define PLC_WAIT_TMR TIM7
#define PLC_WAIT_TMR_VECTOR NVIC_TIM7_IRQ
#define PLC_WAIT_TMR_ISR tim7_isr

/*
*  Backup domain offsets
*/
#define PLC_BKP_VER1_OFFSET      0
#define PLC_BKP_VER2_OFFSET      4
#define PLC_BKP_RTC_IS_OK_OFFSET 8
#define PLC_BKP_REG_OFFSET       0x24

#define PLC_BKP_IRQ1_OFFSET      0xC
#define PLC_BKP_IRQ2_OFFSET      0x10
#define PLC_BKP_IRQ3_OFFSET      0x14
#define PLC_BKP_IRQ4_OFFSET      0x18
#define PLC_BKP_IRQ5_OFFSET      0x1C
#define PLC_BKP_IRQ6_OFFSET      0x20

#define BACKUP_REGS_BASE    RTC_BKP_BASE
//#define PLC_BKP_REG_OFFSET       0x50
//#define PLC_BKP_REG_NUM 19

/*Diag info*/
#define PLC_DIAG_IRQS ((uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_IRQ1_OFFSET))
#define PLC_DIAG_INUM (96)


/*
*  PLC app abi
*/
#define PLC_APP ((plc_app_abi_t *)0x08008000)

/*
*  PLC RTE Version
*/
#define PLC_RTE_VER_MAJOR 2
#define PLC_RTE_VER_MINOR 0
#define PLC_RTE_VER_PATCH 0

#define PLC_HW_ID 227
/*
*  Logging
*/
#define LOG_LEVELS 4
#define LOG_CRITICAL 0
#define LOG_WARNING 1
#define LOG_INFO 2
#define LOG_DEBUG 3
#endif /* _PLC_CONFIG_H_ */
