/*
 * Copyright Nucleron R&D LLC 2016
 */

#ifndef _PLC_CONFIG_H_
#define _PLC_CONFIG_H_
/*
*  NUC-242 configuration!
*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/cortex.h>

#define PLC_DISABLE_INTERRUPTS cm_disable_interrupts
#define PLC_ENABLE_INTERRUPTS  cm_enable_interrupts
/*
*  PLC clocks
*/
#define PLC_HSE_CONFIG cfg_hse_16Mhz
/*
*  Debug USART
*/
#define DBG_USART        USART1
#define DBG_USART_PERIPH RCC_USART1
#define DBG_USART_VECTOR NVIC_USART1_IRQ
#define DBG_USART_ISR    usart1_isr

#define DBG_USART_TX_PERIPH RCC_GPIOA
#define DBG_USART_TX_PORT   GPIOA
#define DBG_USART_TX_PIN    GPIO_USART1_TX

#define DBG_USART_RX_PERIPH RCC_GPIOA
#define DBG_USART_RX_PORT   GPIOA
#define DBG_USART_RX_PIN    GPIO_USART1_RX
/*
*  PLC wait timer
*/
#define PLC_WAIT_TMR_PERIPH RCC_TIM7
#define PLC_WAIT_TMR        TIM7
#define PLC_WAIT_TMR_VECTOR NVIC_TIM7_IRQ
#define PLC_WAIT_TMR_ISR    tim7_isr
/*
*  Jumpers
*/
#define PLC_JMP_RST_PERIPH RCC_GPIOB
#define PLC_JMP_RST_PORT   GPIOB
#define PLC_JMP_RST_PIN    GPIO8

#define PLC_JMP_DBG_PERIPH RCC_GPIOA
#define PLC_JMP_DBG_PORT   GPIOA
#define PLC_JMP_DBG_PIN    GPIO5
/*
*  Boot pin
*/
#define PLC_BOOT_PERIPH RCC_GPIOA
#define PLC_BOOT_PORT   GPIOA
#define PLC_BOOT_PIN    GPIO15
/*
*  Modbus usart
*/
#define MB_USART        USART3
#define MB_USART_PERIPH RCC_USART3
#define MB_USART_VECTOR NVIC_USART3_IRQ
#define MB_USART_ISR    usart3_isr

#define MB_USART_TXEN_PERIPH RCC_GPIOA
#define MB_USART_TXEN_PORT   GPIOA
#define MB_USART_TXEN_PIN    GPIO8

#define MB_USART_TX_PERIPH RCC_GPIOB
#define MB_USART_TX_PORT   GPIOB
#define MB_USART_TX_PIN    GPIO_USART3_TX

#define MB_USART_RX_PERIPH RCC_GPIOB
#define MB_USART_RX_PORT   GPIOB
#define MB_USART_RX_PIN    GPIO_USART3_RX
/*
*  Modbus timer
*/
#define MB_TMR_PERIPH RCC_TIM6
#define MB_TMR        TIM6
#define MB_TMR_VECTOR NVIC_TIM6_IRQ
#define MB_TMR_ISR    tim6_isr

/*
*  PLC LEDS
*/
#define PLC_LED_STG_PERIPH RCC_GPIOC
#define PLC_LED_STG_PORT   GPIOC
#define PLC_LED_STG_PIN    GPIO6

#define PLC_LED_STR_PERIPH RCC_GPIOC
#define PLC_LED_STR_PORT   GPIOC
#define PLC_LED_STR_PIN    GPIO7

#define PLC_LED_TX_PERIPH RCC_GPIOC
#define PLC_LED_TX_PORT   GPIOC
#define PLC_LED_TX_PIN    GPIO9

#define PLC_LED_RX_PERIPH RCC_GPIOC
#define PLC_LED_RX_PORT   GPIOC
#define PLC_LED_RX_PIN    GPIO8
/*
* PLC Inputs
*/
#define PLC_DI_NUM    8

#define PLC_I0_PERIPH RCC_GPIOB
#define PLC_I0_PORT   GPIOB
#define PLC_I0_PIN    GPIO3

#define PLC_I1_PERIPH RCC_GPIOB
#define PLC_I1_PORT   GPIOB
#define PLC_I1_PIN    GPIO4

#define PLC_I2_PERIPH RCC_GPIOB
#define PLC_I2_PORT   GPIOB
#define PLC_I2_PIN    GPIO6

#define PLC_I3_PERIPH RCC_GPIOB
#define PLC_I3_PORT   GPIOB
#define PLC_I3_PIN    GPIO7

#define PLC_I4_PERIPH RCC_GPIOB
#define PLC_I4_PORT   GPIOB
#define PLC_I4_PIN    GPIO12

#define PLC_I5_PERIPH RCC_GPIOB
#define PLC_I5_PORT   GPIOB
#define PLC_I5_PIN    GPIO13

#define PLC_I6_PERIPH RCC_GPIOB
#define PLC_I6_PORT   GPIOB
#define PLC_I6_PIN    GPIO14

#define PLC_I7_PERIPH RCC_GPIOB
#define PLC_I7_PORT   GPIOB
#define PLC_I7_PIN    GPIO15
/*
* PLC Outputs
*/
#define PLC_DO_NUM    4

#define PLC_O0_PERIPH RCC_GPIOC
#define PLC_O0_PORT   GPIOC
#define PLC_O0_PIN    GPIO10

#define PLC_O1_PERIPH RCC_GPIOC
#define PLC_O1_PORT   GPIOC
#define PLC_O1_PIN    GPIO11

#define PLC_O2_PERIPH RCC_GPIOC
#define PLC_O2_PORT   GPIOC
#define PLC_O2_PIN    GPIO12

#define PLC_O3_PERIPH RCC_GPIOD
#define PLC_O3_PORT   GPIOD
#define PLC_O3_PIN    GPIO2

/*
*HMI led outputs
*/
#define PLC_HMI_DO_NUM    2

#define PLC_HMI_O0_PERIPH RCC_GPIOC
#define PLC_HMI_O0_PORT   GPIOC
#define PLC_HMI_O0_PIN    GPIO8

#define PLC_HMI_O1_PERIPH RCC_GPIOC
#define PLC_HMI_O1_PORT   GPIOC
#define PLC_HMI_O1_PIN    GPIO9

/*
* Modbus defaults
*/

#define MB_DEFAULT_BAUDRATE 9600
#define MB_DEFAULT_ADDRESS 1
#define MB_DEFAULT_TRANSPORT 0//1=ASCII 0=RTU

/*
*  Backup domain offsets
*/
///TODO: correct these!
#define PLC_BKP_VER1_OFFSET      0x4
#define PLC_BKP_VER2_OFFSET      0x8
#define PLC_BKP_RTC_IS_OK_OFFSET 0xC
/// IRQ_stub info!
#define PLC_BKP_IRQ1_OFFSET      0x10
#define PLC_BKP_IRQ2_OFFSET      0x14
#define PLC_BKP_IRQ3_OFFSET      0x18
#define PLC_BKP_IRQ4_OFFSET      0x1C
#define PLC_BKP_IRQ5_OFFSET      0x20
#define PLC_BKP_IRQ6_OFFSET      0x24

#define PLC_BKP_REG_OFFSET       0x40
#define PLC_BKP_REG_NUM 32

/*Diag info*/
#define PLC_DIAG_IRQS ((uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_IRQ1_OFFSET))
#define PLC_DIAG_INUM (96)

extern void plc_diag_reset(void);
#define PLC_RESET_HOOK() plc_diag_reset()

/*
*  PLC app abi
*/
#define PLC_APP ((plc_app_abi_t *)0x08008000)
/*
*  PLC RTE Version
*/
#define PLC_RTE_VER_MAJOR 3
#define PLC_RTE_VER_MINOR 0
#define PLC_RTE_VER_PATCH 0
/*
*  Logging
*/
#define LOG_LEVELS 4
#define LOG_CRITICAL 0
#define LOG_WARNING 1
#define LOG_INFO 2
#define LOG_DEBUG 3

/**
* TODO: Add simple printf for error logging!!!
*/
#endif /* _PLC_CONFIG_H_ */
