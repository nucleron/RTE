#ifndef HMI_7SEG_CONF_H_INCLUDED
#define HMI_7SEG_CONF_H_INCLUDED

    #include <stdint.h>
    //Anodes

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


    #define PLC_ANODE_THING(n,name) (PLC_hmi_CONCAT2(HMI_ANODE_,PLC_hmi_CONCAT(n,name)))

    #define PLC_ANODE_PERIPH(n) PLC_ANODE_THING(n,_PERIPH)
    #define PLC_ANODE_PORT(n)   PLC_ANODE_THING(n,_PORT)
    #define PLC_ANODE_PIN(n)    PLC_ANODE_THING(n,_PIN)

    #define PLC_ANODE_REC(n) {PLC_ANODE_PERIPH(n),PLC_ANODE_PORT(n),PLC_ANODE_PIN(n)}

    #define PLC_SEG_THING(n,name) (PLC_hmi_CONCAT2(HMI_SEG_,PLC_hmi_CONCAT(n,name)))

    #define PLC_SEG_PERIPH(n) PLC_SEG_THING(n,_PERIPH)
    #define PLC_SEG_PORT(n)   PLC_SEG_THING(n,_PORT)
    #define PLC_SEG_PIN(n)    PLC_SEG_THING(n,_PIN)

    #define PLC_SEG_REC(n) {PLC_SEG_PERIPH(n),PLC_SEG_PORT(n),PLC_SEG_PIN(n)}

    #define PLC_BTN_THING(n,name) (PLC_hmi_CONCAT2(HMI_BTN_,PLC_hmi_CONCAT(n,name)))

    #define PLC_BTN_PERIPH(n) PLC_BTN_THING(n,_PERIPH)
    #define PLC_BTN_PORT(n)   PLC_BTN_THING(n,_PORT)
    #define PLC_BTN_PIN(n)    PLC_BTN_THING(n,_PIN)

    #define PLC_BTN_REC(n) {PLC_BTN_PERIPH(n),PLC_BTN_PORT(n),PLC_BTN_PIN(n)}

//bit:  7 6 5 4 3 2 1 0
//seg: dp a b c d e f g

//     a
//    ___
// f |   | b
//   |___|
//   | g |
// e |___| c  oP
//     d

const uint8_t segmentLookup[] =
{
    0x7e, // 0
    0x30, // 1
    0x6d, // 2
    0x79, // 3
    0x33, // 4
    0x5b, // 5   5
    0x5f, // 6
    0x70, // 7
    0x7f, // 8
    0x7b, // 9
    0x77, // A  10
    0x1f, // b
    0x4e, // C
    0x3d, // d
    0x4f, // E
    0x47, // f  15
    0x5e, // g
    0x37, // h
    0x7c, // J
    0x0e, // L
    0x67, // P  20
    0x05, // r
    0x1c, // U
    0x00, //   (space)
    0x01, // - (negative)
    (1<<6),//upper dash      25
    (1<<3),//underscore
    (1<<5),//right top (b)
    (1<<4),//right low (c)
    (1<<2),//left low  (e)
    (1<<1),//left top (f)    30
    0x15, // n
    0x1d, // o
    0x0f, // t
    0x0d, // c
    0x57, // H        35

    //

    0x08  // _ (invalid character has to be at the end of array)
    };


//const uint8_t screensaver_seq[][2]={{0,30},{0,25},{1,25},{2,25},{3,25},{4,25},{4,27},{4,28},{4,26},{3,26},{2,26},{1,26},{0,26},{0,29}};
//#define HMI_NSS 14

#define HMI_DIGITS   6
#define HMI_NBUTTONS 3

#define HMI_BTN_UP   2
#define HMI_BTN_OK   1
#define HMI_BTN_DOWN 0
#define HMI_BTN_RELEASE (1<<7)

#define HMI_LONGPRESS 1500 //ticks
#define HMI_REPEAT     500 //ticks
#define HMI_SCREENASVER 30000

#define HMI_MENU_SYS_LAST 22
#define HMI_MENU_PLC_LAST  1

#endif // HMI_7SEG_CONF_H_INCLUDED
