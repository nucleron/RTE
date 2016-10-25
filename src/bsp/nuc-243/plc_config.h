/*
 * Copyright Nucleron R&D LLC 2016
 */

#ifndef _PLC_CONFIG_H_
#define _PLC_CONFIG_H_
/*
*  NUC-243 configuration!
*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/cortex.h>

#define PLC_DISABLE_INTERRUPTS cm_disable_interrupts
#define PLC_ENABLE_INTERRUPTS  cm_enable_interrupts
/*
*  PLC clocks
*/
#define PLC_HSE_CONFIG cfg_hse_16Mhz
#define PLC_RCC_AHB_FREQ 120
/*
*  Debug USART
*/
#define DBG_USART        USART1
#define DBG_USART_PERIPH RCC_USART1
#define DBG_USART_VECTOR NVIC_USART1_IRQ
#define DBG_USART_ISR    usart1_isr

#define DBG_USART_TX_PERIPH RCC_GPIOA
#define DBG_USART_TX_PORT   GPIOA
#define DBG_USART_TX_PIN    GPIO9

#define DBG_USART_RX_PERIPH RCC_GPIOA
#define DBG_USART_RX_PORT   GPIOA
#define DBG_USART_RX_PIN    GPIO10
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
#define PLC_JMP_RST_PERIPH RCC_GPIOD
#define PLC_JMP_RST_PORT   GPIOD
#define PLC_JMP_RST_PIN    GPIO10

#define PLC_JMP_DBG_PERIPH RCC_GPIOD
#define PLC_JMP_DBG_PORT   GPIOD
#define PLC_JMP_DBG_PIN    GPIO3
/*
*  Boot pin
*/
#define PLC_BOOT_PERIPH RCC_GPIOE
#define PLC_BOOT_PORT   GPIOE
#define PLC_BOOT_PIN    GPIO7
/*
*  Modbus usart
*/
#define MB_USART        USART2
#define MB_USART_PERIPH RCC_USART2
#define MB_USART_VECTOR NVIC_USART2_IRQ
#define MB_USART_ISR    usart2_isr

#define MB_USART_TXEN_PERIPH RCC_GPIOD
#define MB_USART_TXEN_PORT   GPIOD
#define MB_USART_TXEN_PIN    GPIO7

#define MB_USART_TX_PERIPH RCC_GPIOD
#define MB_USART_TX_PORT   GPIOD
#define MB_USART_TX_PIN    GPIO5

#define MB_USART_RX_PERIPH RCC_GPIOD
#define MB_USART_RX_PORT   GPIOD
#define MB_USART_RX_PIN    GPIO6
/*
*  Modbus timer
*/
#define MB_TMR_PERIPH RCC_TIM6
#define MB_TMR        TIM6
#define MB_TMR_VECTOR NVIC_TIM6_DAC_IRQ
#define MB_TMR_ISR    tim6_dac_isr

/*
*  PLC LEDS
*/
#define PLC_LED_STG_PERIPH RCC_GPIOE
#define PLC_LED_STG_PORT   GPIOE
#define PLC_LED_STG_PIN    GPIO14

#define PLC_LED_STR_PERIPH RCC_GPIOE
#define PLC_LED_STR_PORT   GPIOE
#define PLC_LED_STR_PIN    GPIO15

#define PLC_LED_TX_PERIPH RCC_GPIOE
#define PLC_LED_TX_PORT   GPIOE
#define PLC_LED_TX_PIN    GPIO12

#define PLC_LED_RX_PERIPH RCC_GPIOE
#define PLC_LED_RX_PORT   GPIOE
#define PLC_LED_RX_PIN    GPIO13
/*
* PLC Inputs
*/
#define PLC_DI_NUM    8

#define PLC_I0_PERIPH RCC_GPIOE
#define PLC_I0_PORT   GPIOE
#define PLC_I0_PIN    GPIO4

#define PLC_I1_PERIPH RCC_GPIOE
#define PLC_I1_PORT   GPIOE
#define PLC_I1_PIN    GPIO3

#define PLC_I2_PERIPH RCC_GPIOE
#define PLC_I2_PORT   GPIOE
#define PLC_I2_PIN    GPIO2

#define PLC_I3_PERIPH RCC_GPIOE
#define PLC_I3_PORT   GPIOE
#define PLC_I3_PIN    GPIO1

#define PLC_I4_PERIPH RCC_GPIOE
#define PLC_I4_PORT   GPIOE
#define PLC_I4_PIN    GPIO0

#define PLC_I5_PERIPH RCC_GPIOB
#define PLC_I5_PORT   GPIOB
#define PLC_I5_PIN    GPIO9

#define PLC_I6_PERIPH RCC_GPIOB
#define PLC_I6_PORT   GPIOB
#define PLC_I6_PIN    GPIO8

#define PLC_I7_PERIPH RCC_GPIOB
#define PLC_I7_PORT   GPIOB
#define PLC_I7_PIN    GPIO7

/*
* PLC Outputs
*/
#define PLC_DO_NUM    6

#define PLC_O0_PERIPH RCC_GPIOB
#define PLC_O0_PORT   GPIOB
#define PLC_O0_PIN    GPIO6

#define PLC_O1_PERIPH RCC_GPIOB
#define PLC_O1_PORT   GPIOB
#define PLC_O1_PIN    GPIO5

#define PLC_O2_PERIPH RCC_GPIOB
#define PLC_O2_PORT   GPIOB
#define PLC_O2_PIN    GPIO4

#define PLC_O3_PERIPH RCC_GPIOB
#define PLC_O3_PORT   GPIOB
#define PLC_O3_PIN    GPIO3

#define PLC_O4_PERIPH RCC_GPIOC
#define PLC_O4_PORT   GPIOC
#define PLC_O4_PIN    GPIO6

#define PLC_O5_PERIPH RCC_GPIOC
#define PLC_O5_PORT   GPIOC
#define PLC_O5_PIN    GPIO7

// Analog inputs
#define ADC_CHANNEL_REF2v5  ADC_CHANNEL2
#define ADC_CHANNEL_18v     ADC_CHANNEL7
#define ADC_CHANNEL_AIN0    ADC_CHANNEL14
#define ADC_CHANNEL_AIN1    ADC_CHANNEL15
#define ADC_CHANNEL_AIN2    ADC_CHANNEL8
#define ADC_CHANNEL_AIN3    ADC_CHANNEL9

#define U_REF_2V5           2500  // мВ
#define U_SENS_AT_0         6975  // 0.0 мВ
#define U_SENS_MV_C         25    // 0.0 мВ / С

#define PLC_DEFAULT_MEDIAN_DEPH         5 // ИСПОЛЬЗУЕМО элементов массива медианного фильтра
#define PLC_DEFAULT_MEDIAN_IDX          0 // текущий элемент массива
#define PLC_MIN_MEDIAN_DEPH             3 // МИНИМАЛЬНО  элементов массива медианного фильтра
#define PLC_DEFAULT_MEDIAN_VAL          0

#define PLC_DEFAULT_AVE_DEPH            4 // ИСПОЛЬЗУЕМО элементов массива усредняющего фильтра
#define PLC_DEFAULT_AVE_IDX             0 // текущий элемент массива
#define PLC_MIN_AVE_DEPH                1 // МИНИМАЛЬНО элементов массива усредняющего фильтра
#define PLC_DEFAULT_AVE_VAL             0

#define PLC_DEFAULT_POLL_CNT             0
#define PLC_DEFAULT_POLL_PERIOD          1 // периодичность запуска измерения, мс

#define PLC_DEFAULT_THR_LOW              4000
#define PLC_DEFAULT_THR_HIGH                 6000
#define PLC_DEFAULT_ADC_VAL              0
#define PLC_DEFAULT_CMP_VAL              false
#define PLC_DEFAULT_ADC_FLG              0
#define PLC_DEFAULT_ADC_MODE             1

#define PLC_DEFAULT_COEF_10V             41952
#define PLC_DEFAULT_COEF_20MA            41769
#define PLC_DEFAULT_COEF_100R            218
#define PLC_DEFAULT_COEF_4K              3141

#define PLC_DEFAULT_SUM                  2 // количество данных для усреднения
#define PLC_DEFAULT_CNTR                 0 // счётчик количества усреднений
#define PLC_DEFAULT_DATA_IN              0 // сбор данных для усредения
#define PLC_DEFAULT_OUT                  0 // актуальные усреднённые данные
#define PLC_DEFAULT_CALC                 0 // пересчитанное в величины

// Питание аналоговых портов

#define PLC_PWR_AI00_PERIPH  RCC_GPIOD
#define PLC_PWR_AI00_PORT    GPIOD
#define PLC_PWR_AI00_PIN     GPIO8

#define PLC_PWR_AI10_PERIPH  RCC_GPIOD
#define PLC_PWR_AI10_PORT    GPIOD
#define PLC_PWR_AI10_PIN     GPIO9

#define PLC_PWR_AI01_PERIPH  RCC_GPIOB
#define PLC_PWR_AI01_PORT    GPIOB
#define PLC_PWR_AI01_PIN     GPIO14

#define PLC_PWR_AI11_PERIPH  RCC_GPIOB
#define PLC_PWR_AI11_PORT    GPIOB
#define PLC_PWR_AI11_PIN     GPIO15

#define PLC_PWR_AI02_PERIPH  RCC_GPIOB
#define PLC_PWR_AI02_PORT    GPIOB
#define PLC_PWR_AI02_PIN     GPIO12

#define PLC_PWR_AI12_PERIPH  RCC_GPIOB
#define PLC_PWR_AI12_PORT    GPIOB
#define PLC_PWR_AI12_PIN     GPIO13

#define PLC_PWR_AI03_PERIPH  RCC_GPIOB
#define PLC_PWR_AI03_PORT    GPIOB
#define PLC_PWR_AI03_PIN     GPIO10

#define PLC_PWR_AI13_PERIPH  RCC_GPIOB
#define PLC_PWR_AI13_PORT    GPIOB
#define PLC_PWR_AI13_PIN     GPIO11

// Управление шунтами аналоговых входов
#define PLC_NON00_PERIPH    RCC_GPIOD
#define PLC_NON00_PORT      GPIOD
#define PLC_NON00_PIN       GPIO15

#define PLC_NON10_PERIPH    RCC_GPIOD
#define PLC_NON10_PORT      GPIOD
#define PLC_NON10_PIN       GPIO14

#define PLC_NON01_PERIPH    RCC_GPIOA
#define PLC_NON01_PORT      GPIOA
#define PLC_NON01_PIN       GPIO8

#define PLC_NON11_PERIPH    RCC_GPIOC
#define PLC_NON11_PORT      GPIOC
#define PLC_NON11_PIN       GPIO9

#define PLC_NON02_PERIPH    RCC_GPIOC
#define PLC_NON02_PORT      GPIOC
#define PLC_NON02_PIN       GPIO8

#define PLC_NON12_PERIPH    RCC_GPIOD
#define PLC_NON12_PORT      GPIOD
#define PLC_NON12_PIN       GPIO13

#define PLC_NON03_PERIPH    RCC_GPIOD
#define PLC_NON03_PORT      GPIOD
#define PLC_NON03_PIN       GPIO12

#define PLC_NON13_PERIPH    RCC_GPIOD
#define PLC_NON13_PORT      GPIOD
#define PLC_NON13_PIN       GPIO11

// Аналоговые входы

#define PLC_AIN0_PERIPH     RCC_GPIOC
#define PLC_AIN0_PORT       GPIOC
#define PLC_AIN0_PIN        GPIO4

#define PLC_AIN1_PERIPH     RCC_GPIOC
#define PLC_AIN1_PORT       GPIOC
#define PLC_AIN1_PIN        GPIO5

#define PLC_AIN2_PERIPH     RCC_GPIOB
#define PLC_AIN2_PORT       GPIOB
#define PLC_AIN2_PIN        GPIO0

#define PLC_AIN3_PERIPH     RCC_GPIOB
#define PLC_AIN3_PORT       GPIOB
#define PLC_AIN3_PIN        GPIO1

// Входы АЦП

#define PLC_REF2V5_PERIPH   RCC_GPIOA
#define PLC_REF2V5_PORT     GPIOA
#define PLC_REF2V5_PIN      GPIO2

#define PLC_ADC18_PERIPH    RCC_GPIOA
#define PLC_ADC18_PORT      GPIOA
#define PLC_ADC18_PIN       GPIO7

// Analog outputs

#define PLC_DAC_CLK_PERIPH      RCC_GPIOC
#define PLC_DAC_CLK_PORT        GPIOC
#define PLC_DAC_CLK_PIN         GPIO12

#define PLC_DAC_DIN_PERIPH      RCC_GPIOD
#define PLC_DAC_DIN_PORT        GPIOD
#define PLC_DAC_DIN_PIN         GPIO0

#define PLC_DAC_SYN0_PERIPH     RCC_GPIOD
#define PLC_DAC_SYN0_PORT       GPIOD
#define PLC_DAC_SYN0_PIN        GPIO1

#define PLC_DAC_SYN1_PERIPH     RCC_GPIOD
#define PLC_DAC_SYN1_PORT       GPIOD
#define PLC_DAC_SYN1_PIN        GPIO2

/*
*HMI
*/
#define PLC_HMI_SYS_PSIZE 23

#define PLC_HMI_DO_NUM    2
#define PLC_HMI_DISPLAY_LEDS 8
#define PLC_HMI_DISPLAY_DOTS 4
/*
GR LED
*/
#define PLC_HMI_O0_PERIPH RCC_GPIOE
#define PLC_HMI_O0_PORT   GPIOE
#define PLC_HMI_O0_PIN    GPIO12

#define PLC_HMI_O1_PERIPH RCC_GPIOE
#define PLC_HMI_O1_PORT   GPIOE
#define PLC_HMI_O1_PIN    GPIO13

/*Display anodes*/
#define PLC_HMI_ANODE_0_PERIPH RCC_GPIOC
#define PLC_HMI_ANODE_0_PORT GPIOC
#define PLC_HMI_ANODE_0_PIN  GPIO13

#define PLC_HMI_ANODE_1_PERIPH RCC_GPIOE
#define PLC_HMI_ANODE_1_PORT GPIOE
#define PLC_HMI_ANODE_1_PIN  GPIO10

#define PLC_HMI_ANODE_2_PERIPH RCC_GPIOE
#define PLC_HMI_ANODE_2_PORT GPIOE
#define PLC_HMI_ANODE_2_PIN  GPIO8

#define PLC_HMI_ANODE_3_PERIPH RCC_GPIOC
#define PLC_HMI_ANODE_3_PORT GPIOC
#define PLC_HMI_ANODE_3_PIN  GPIO3

#define PLC_HMI_ANODE_4_PERIPH RCC_GPIOA
#define PLC_HMI_ANODE_4_PORT GPIOA
#define PLC_HMI_ANODE_4_PIN  GPIO5

#define PLC_HMI_ANODE_5_PERIPH RCC_GPIOA
#define PLC_HMI_ANODE_5_PORT GPIOA
#define PLC_HMI_ANODE_5_PIN  GPIO3

//Segment pins
#define PLC_HMI_SEG_6_PERIPH RCC_GPIOA //Segment A
#define PLC_HMI_SEG_6_PORT GPIOA
#define PLC_HMI_SEG_6_PIN  GPIO0

#define PLC_HMI_SEG_5_PERIPH RCC_GPIOC //Segment B
#define PLC_HMI_SEG_5_PORT GPIOC
#define PLC_HMI_SEG_5_PIN  GPIO2

#define PLC_HMI_SEG_4_PERIPH RCC_GPIOA //Segment C
#define PLC_HMI_SEG_4_PORT GPIOA
#define PLC_HMI_SEG_4_PIN  GPIO6

#define PLC_HMI_SEG_3_PERIPH RCC_GPIOE //Segment D
#define PLC_HMI_SEG_3_PORT GPIOE
#define PLC_HMI_SEG_3_PIN  GPIO11

#define PLC_HMI_SEG_2_PERIPH RCC_GPIOE //Segment E
#define PLC_HMI_SEG_2_PORT GPIOE
#define PLC_HMI_SEG_2_PIN  GPIO9

#define PLC_HMI_SEG_1_PERIPH RCC_GPIOC //Segment F
#define PLC_HMI_SEG_1_PORT GPIOC
#define PLC_HMI_SEG_1_PIN  GPIO1

#define PLC_HMI_SEG_0_PERIPH RCC_GPIOC //Segment G
#define PLC_HMI_SEG_0_PORT GPIOC
#define PLC_HMI_SEG_0_PIN  GPIO0

#define PLC_HMI_SEG_7_PERIPH RCC_GPIOA  //Decimal point
#define PLC_HMI_SEG_7_PORT GPIOA
#define PLC_HMI_SEG_7_PIN  GPIO4

//Buttons
#define PLC_HMI_BTN_0_PERIPH RCC_GPIOE //Button 0
#define PLC_HMI_BTN_0_PORT GPIOE
#define PLC_HMI_BTN_0_PIN  GPIO5

#define PLC_HMI_BTN_1_PERIPH RCC_GPIOE //Button 1
#define PLC_HMI_BTN_1_PORT GPIOE
#define PLC_HMI_BTN_1_PIN  GPIO6

#define PLC_HMI_BTN_2_PERIPH RCC_GPIOA  //Button 2
#define PLC_HMI_BTN_2_PORT GPIOA
#define PLC_HMI_BTN_2_PIN  GPIO1

//Led shifts
#define PLC_HMI_LED_2 0
#define PLC_HMI_LED_3 1
#define PLC_HMI_LED_4 5
#define PLC_HMI_LED_5 6
#define PLC_HMI_LED_6 3
#define PLC_HMI_LED_7 2
#define PLC_HMI_LED_8 4
#define PLC_HMI_LED_9 7

/*
* Modbus defaults
*/

#define MB_DEFAULT_BAUDRATE 9600
#define MB_DEFAULT_ADDRESS 1
#define MB_DEFAULT_TRANSPORT 0//1=ASCII 0=RTU

/*
*  Backup domain things
*/

#define BACKUP_UNLOCK() PWR_CR |= PWR_CR_DBP
#define BACKUP_LOCK() PWR_CR &= ~PWR_CR_DBP

///TODO: correct these!
#define PLC_BKP_VER1_OFFSET      0x0
#define PLC_BKP_VER2_OFFSET      0x4

#define PLC_BKP_BANK2_VER1_OFFSET      0x24
#define PLC_BKP_BANK2_VER2_OFFSET      0x28
#define PLC_BKP_BRIGHT_OFFSET          0x2C

#define PLC_BKP_RTC_IS_OK_OFFSET 0x8
/// IRQ_stub info!
#define PLC_BKP_IRQ1_OFFSET      0xC
#define PLC_BKP_IRQ2_OFFSET      0x10
#define PLC_BKP_IRQ3_OFFSET      0x14
#define PLC_BKP_IRQ4_OFFSET      0x18
#define PLC_BKP_IRQ5_OFFSET      0x1C
#define PLC_BKP_IRQ6_OFFSET      0x20

#define BACKUP_REGS_BASE    RTC_BASE + 0x50
//#define PLC_BKP_REG_OFFSET       0x50
#define PLC_BKP_REG_NUM 19

/*Diag info*/
#define PLC_DIAG_IRQS ((uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_IRQ1_OFFSET))
#define PLC_DIAG_INUM (96)

extern void plc_diag_reset(void);
#define PLC_RESET_HOOK() plc_diag_reset()

/*
*  PLC app abi
*/
#define PLC_APP ((plc_app_abi_t *)0x08010000)
/*
*  PLC RTE Version
*/
#define PLC_RTE_VER_MAJOR 4
#define PLC_RTE_VER_MINOR 0
#define PLC_RTE_VER_PATCH 0

#define PLC_HW_ID      243

/**
* TODO: Add simple printf for error logging!!!
*/
#endif /* _PLC_CONFIG_H_ */
