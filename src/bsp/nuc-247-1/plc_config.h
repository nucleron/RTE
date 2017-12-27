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
#define USART_2

#define USART_MBS_PERIPH USART2
#define USART_MBS_RCC_PERIPH RCC_USART2
#define USART_MBS_VECTOR NVIC_USART2_IRQ
#define USART_MBS_ISR usart2_isr

#define USART_MBS_TXEN_PERIPH RCC_GPIOD
#define USART_MBS_TXEN_PORT   GPIOD
#define USART_MBS_TXEN_PIN    GPIO7

#define USART_MBS_TX_PERIPH RCC_GPIOD
#define USART_MBS_TX_PORT   GPIOD
#define USART_MBS_TX_PIN    GPIO5

#define USART_MBS_RX_PERIPH RCC_GPIOD
#define USART_MBS_RX_PORT   GPIOD
#define USART_MBS_RX_PIN    GPIO6

/*
* Modbus master USART
*/
#define USART_MBM

#define USART_MBM_PERIPH USART3
#define USART_MBM_RCC_PERIPH RCC_USART3
#define USART_MBM_VECTOR NVIC_USART3_IRQ
#define USART_MBM_ISR usart3_isr

#define USART_MBM_TXEN_PERIPH RCC_GPIOA
#define USART_MBM_TXEN_PORT   GPIOA
#define USART_MBM_TXEN_PIN    GPIO15

#define USART_MBM_TX_PERIPH RCC_GPIOC
#define USART_MBM_TX_PORT   GPIOC
#define USART_MBM_TX_PIN    GPIO10

#define USART_MBM_RX_PERIPH RCC_GPIOC
#define USART_MBM_RX_PORT   GPIOC
#define USART_MBM_RX_PIN    GPIO11

#define MBS_USART        0
#define MBM_USART       1
/*
*  Modbus timer
*/
#define MBS_TMR_PERIPH RCC_TIM6
#define MBS_TMR        TIM6
#define MBS_TMR_VECTOR NVIC_TIM6_DAC_IRQ
#define MBS_TMR_ISR    tim6_dac_isr

#define MBM_TMR_PERIPH RCC_TIM3
#define MBM_TMR        TIM3
#define MBM_TMR_VECTOR NVIC_TIM3_IRQ
#define MBM_TMR_ISR    tim3_isr

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
#define PLC_DI_NUM    12

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

#define PLC_I8_PERIPH RCC_GPIOC
#define PLC_I8_PORT   GPIOC
#define PLC_I8_PIN    GPIO4

#define PLC_I9_PERIPH RCC_GPIOC
#define PLC_I9_PORT   GPIOC
#define PLC_I9_PIN    GPIO5

#define PLC_I10_PERIPH RCC_GPIOB
#define PLC_I10_PORT   GPIOB
#define PLC_I10_PIN    GPIO0

#define PLC_I11_PERIPH RCC_GPIOB
#define PLC_I11_PORT   GPIOB
#define PLC_I11_PIN    GPIO1

/*
* PLC Outputs
*/
#define PLC_DO_NUM    8

#define PLC_O0_PERIPH RCC_GPIOB
#define PLC_O0_PORT   GPIOB
#define PLC_O0_PIN    GPIO6

#define PLC_O1_PERIPH RCC_GPIOA
#define PLC_O1_PORT   GPIOA
#define PLC_O1_PIN    GPIO7

#define PLC_O2_PERIPH RCC_GPIOB
#define PLC_O2_PORT   GPIOD
#define PLC_O2_PIN    GPIO4

#define PLC_O3_PERIPH RCC_GPIOB
#define PLC_O3_PORT   GPIOB
#define PLC_O3_PIN    GPIO3

#define PLC_O4_PERIPH RCC_GPIOC
#define PLC_O4_PORT   GPIOC
#define PLC_O4_PIN    GPIO9

#define PLC_O5_PERIPH RCC_GPIOA
#define PLC_O5_PORT   GPIOA
#define PLC_O5_PIN    GPIO8

#define PLC_O6_PERIPH RCC_GPIOD
#define PLC_O6_PORT   GPIOD
#define PLC_O6_PIN    GPIO1

#define PLC_O7_PERIPH RCC_GPIOD
#define PLC_O7_PORT   GPIOD
#define PLC_O7_PIN    GPIO2


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
#define PLC_APP ((plc_app_abi_t *)0x08008000)
/*
*  PLC RTE Version
*/
#define PLC_RTE_VER_MAJOR 4
#define PLC_RTE_VER_MINOR 0
#define PLC_RTE_VER_PATCH 0

///TODO: Fix this in RC0
#define PLC_HW_ID      2470

/*
*  PLC RTE error numbers
*/
/* DIO */
#define PLC_ERRNO_DIO_ASZ     10
#define PLC_ERRNO_DIO_ASZ_FLT 11
#define PLC_ERRNO_DIO_TP      12
#define PLC_ERRNO_DIO_TP_FLT  13
#define PLC_ERRNO_DIO_ILIM    14
#define PLC_ERRNO_DIO_FLT_LIM 15
#define PLC_ERRNO_DIO_OLIM    16
/* MBS */
#define PLC_ERRNO_MBS_ASZ     20
#define PLC_ERRNO_MBS_TP      21
#define PLC_ERRNO_MBS_ADDR    22
#define PLC_ERRNO_MBS_INIT    23
#define PLC_ERRNO_MBS_TP_INIT 24
#define PLC_ERRNO_MBS_EX_INIT 25

/* MBM */
#define PLC_ERRNO_MBM_ASZ     300
#define PLC_ERRNO_MBM_TP      301
#define PLC_ERRNO_MBM_SZ      302
#define PLC_ERRNO_MBM_ST      303

#define PLC_ERRNO_MBM_RQ_TP   304  /*RQ type*/
#define PLC_ERRNO_MBM_RQ_SA   305  /*Salve address*/
#define PLC_ERRNO_MBM_RQ_RA   306  /*Register address*/
#define PLC_ERRNO_MBM_RQ_MS   307  /*Milliseconds*/
#define PLC_ERRNO_MBM_RQ_ID   308  /*Request ID is not unique*/
#define PLC_ERRNO_MBM_RQ_LIM  309  /*Request ID is out of bounds*/

#define PLC_ERRNO_MBM_REG_ID  310

#define PLC_ERRNO_MBM_BAUD    311
#define PLC_ERRNO_MBM_MODE    312
#define PLC_ERRNO_MBM_USFG    313 /**/

#define PLC_ERRNO_MBM_ULT     314

#define PLC_ERRNO_MBM_VSZ     315

#define PLC_ERRNO_MBM_DEL     316 /**/
/* HMI */
#define PLC_ERRNO_HMI_ASZ     400
#define PLC_ERRNO_HMI_TP      401
#define PLC_ERRNO_HMI_DIR     402
#define PLC_ERRNO_HMI_OLIM    403
#define PLC_ERRNO_HMI_MLIM    404
#define PLC_ERRNO_HMI_MULTI   405
#define PLC_ERRNO_HMI_MTYPE   406
#define PLC_ERRNO_HMI_ITYPE   407
#define PLC_ERRNO_HMI_REPR    408
#define PLC_ERRNO_HMI_LLIM    409
#define PLC_ERRNO_HMI_LTF     410
#define PLC_ERRNO_HMI_LUSED   411
/* AOUT */
#define PLC_ERRNO_AOUT_ASZ  50
#define PLC_ERRNO_AOUT_TP   51
#define PLC_ERRNO_AOUT_ADDR 52
/* AIN */
#define PLC_ERRNO_AIN_ASZ 60
#define PLC_ERRNO_AIN_MEM 61
#define PLC_ERRNO_AIN_CHN 62
#define PLC_ERRNO_AIN_IX  63
#define PLC_ERRNO_AIN_IX9 64
#define PLC_ERRNO_AIN_QB  65
#define PLC_ERRNO_AIN_QBX 66
#define PLC_ERRNO_AIN_QWX 67
#define PLC_ERRNO_AIN_IW7 68



#endif /* _PLC_CONFIG_H_ */
