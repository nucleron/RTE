#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <dbnc_flt.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_gpio.h>
#include <plc_hw.h>
#include <plc_hmi.h>
#include <plc_iom.h>
#include <plc_wait_tmr.h>

//#include <hmi_7seg_conf.h>

///Перенести в конфиг!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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

#define PLC_HMI_LED_2 0
#define PLC_HMI_LED_3 1
#define PLC_HMI_LED_4 5
#define PLC_HMI_LED_5 6
#define PLC_HMI_LED_6 3
#define PLC_HMI_LED_7 2
#define PLC_HMI_LED_8 4
#define PLC_HMI_LED_9 7

///Перенести в конфиг!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define MIN(x,y) (((x)>(y))?(y):(x))

//==================================================================
//                   Keyboard related things
//==================================================================
#define PLC_HMI_BUTTON_THR 1500 //1500ms

typedef struct
{
    dbnc_flt_t flt; //debounce filter
    uint32_t tmr;   //timer
    bool state;     //state
    char ch_short;  //short press char
    char ch_long;   //long press char
}
btn_fsm_t; //button FSM

#define PLC_HMI_BTN_DOWN true  //Down state
#define PLC_HMI_BTN_UP   false //Up state

#define PLC_HMI_BTN_NO_CHAR  0 //Up state

static inline void btn_fsm_init(btn_fsm_t * self, char ch_short, char ch_long)
{
    dbnc_flt_init((dbnc_flt_t *)self);

    self->state    = PLC_HMI_BTN_UP;
    self->tmr      = 0;
    self->ch_short = ch_short;
    self->ch_long  = ch_long;
}

static char btn_fsm_poll(btn_fsm_t * self, uint32_t tick, bool in)
{
    dbnc_flt_poll((dbnc_flt_t *)self, tick, in);

    if (PLC_HMI_BTN_DOWN == self->state)
    {
        if (PLC_HMI_BTN_UP == ((dbnc_flt_t *)self)->flg)
        {
            self->state = PLC_HMI_BTN_UP;

            if (PLC_HMI_BUTTON_THR > (tick - self->tmr))
            {
                return self->ch_short;
            }
            else
            {
                return self->ch_long;
            }
        }
    }
    else
    {
        if (PLC_HMI_BTN_DOWN == ((dbnc_flt_t *)self)->flg)
        {
            self->state = PLC_HMI_BTN_DOWN;
            self->tmr = tick;
        }
    }

    return PLC_HMI_BTN_NO_CHAR;
}

static const plc_gpio_t button[] =
{
    PLC_GPIO_REC(HMI_BTN_0),
    PLC_GPIO_REC(HMI_BTN_1),
    PLC_GPIO_REC(HMI_BTN_2),
};
#define BUTTONS_NUM (sizeof(button)/sizeof(plc_gpio_t))

#define PLC_HMI_BTN_NO_CHAR  0 //Up state

#define PLC_HMI_BTN_UP_S 1
#define PLC_HMI_BTN_UP_L 2
#define PLC_HMI_BTN_OK_S 3
#define PLC_HMI_BTN_OK_L 4
#define PLC_HMI_BTN_DW_S 5
#define PLC_HMI_BTN_DW_L 6

static const button_cfg[3][2] =
{
    //     SHORT            LONG
    {PLC_HMI_BTN_UP_S, PLC_HMI_BTN_UP_L},
    {PLC_HMI_BTN_OK_S, PLC_HMI_BTN_OK_L},
    {PLC_HMI_BTN_DW_S, PLC_HMI_BTN_DW_L}
};

btn_fsm_t button_fsm[BUTTONS_NUM];

static void plc_hmi_kb_init(void)
{
    uint8_t i;
    for (i=0; i < BUTTONS_NUM; i++)
    {
        plc_gpio_cfg_in(button     + i);
        btn_fsm_init   (button_fsm + i, button_cfg[i][0], button_cfg[i][1]);
    }
}

static char plc_hmi_kb_poll(uint32_t tick)
{
    uint8_t i;
    char ret = PLC_HMI_BTN_NO_CHAR;

    for (i=0; i < BUTTONS_NUM; i++)
    {
        ret = btn_fsm_poll(button_fsm + i, tick, plc_gpio_get(button + i));
        if( PLC_HMI_BTN_NO_CHAR != ret )
        {
            break;
        }
    }

    return ret;
}
//==================================================================
//                 Framebuffer related things
//==================================================================
static uint8_t buf1[HMI_DIGITS];
static uint8_t buf2[HMI_DIGITS];

static uint8_t* video     = buf1;
static uint8_t* convert   = buf2;
static bool     ready_flg = false;
static uint8_t  brightness = 0;
char            str_buf[HMI_DIGITS+1];

static inline void swap_buf(void)
{
    char* tmp;

    tmp = video;
    video = convert;
    convert= tmp;

    ready_flg = false;
}

static const plc_gpio_t hmi_led[PLC_HMI_DO_NUM] =
{
    PLC_GPIO_REC(LED_TX),
    PLC_GPIO_REC(LED_RX)
};

static const plc_gpio_t anodes[HMI_DIGITS] =
{
    PLC_GPIO_REC(HMI_ANODE_0),
    PLC_GPIO_REC(HMI_ANODE_1),
    PLC_GPIO_REC(HMI_ANODE_2),
    PLC_GPIO_REC(HMI_ANODE_3),
    PLC_GPIO_REC(HMI_ANODE_4),
    PLC_GPIO_REC(HMI_ANODE_5)
};

static const plc_gpio_t segments[8] =
{
    PLC_GPIO_REC(HMI_SEG_0),
    PLC_GPIO_REC(HMI_SEG_1),
    PLC_GPIO_REC(HMI_SEG_2),
    PLC_GPIO_REC(HMI_SEG_3),
    PLC_GPIO_REC(HMI_SEG_4),
    PLC_GPIO_REC(HMI_SEG_5),
    PLC_GPIO_REC(HMI_SEG_6),
    PLC_GPIO_REC(HMI_SEG_7),
};

static inline void plc_hmi_vout_init(void)
{
    //LED
    PLC_GPIO_GR_CFG_OUT(hmi_led);
    //Anodes init
    PLC_GPIO_GR_CFG_OUT(anodes);
    //Segment lines init
    PLC_GPIO_GR_CFG_OUT(segments);
}

void plc_hmi_vout_poll(void)
{
    static uint8_t pwm=0;
    static uint8_t anode_n=0;
    uint8_t i;

    if(pwm>7) //end of one pwm cycle
    {
        plc_gpio_clear(anodes + anode_n);//switch off previous char

        for (i=0; i<8; i++)
        {
            plc_gpio_clear(segments + i);//set segment values
        }
        anode_n++; //go to next place
        if (anode_n >= HMI_DIGITS) //End of frame
        {
            anode_n = 0;
        }
        //we have new data to display
        if (true == ready_flg)
        {
            swap_buf();
        }
        //set segment values
        for (i=0; i<8; i++)
        {
            if ((video[anode_n]&(1<<i)) == 0)
            {
                plc_gpio_clear(segments + i);
            }
            else
            {
                plc_gpio_set(segments + i);
            }
        }
        plc_gpio_set(anodes + anode_n); //turn on new char
        pwm=0;
    }

    if (pwm > brightness)
    {
        plc_gpio_clear(anodes + anode_n);
    }
    pwm++;
}

static void plc_hmi_set_dout( uint32_t i, bool val )
{
    void (*do_set)(uint32_t, uint16_t);

    if (PLC_DO_NUM<=i)
    {
        return;
    }
    do_set = (val)?gpio_set:gpio_clear;
    do_set(hmi_led[i].port, hmi_led[i].pin);
}

//==================================================================
//                     Data conversion things
//==================================================================
//bit:  7 6 5 4 3 2 1 0
//seg: dp a b c d e f g
//     a
//    ___
// f |   | b
//   |___|
//   | g |
// e |___| c  oP
//     d
static const uint8_t segmentLookup[] =
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

static uint8_t char_to_7seg(unsigned char c)
{
    if((c>='0') && (c<='9')) //numeric characters
    {
        return segmentLookup[c-'0'];
    }
    switch(c) //process characters that have both upper and lower case
    {
    case 'C':
        return segmentLookup[12];
    case 'c':
        return segmentLookup[34];
    case 'H':
        return segmentLookup[35];
    case 'h':
        return segmentLookup[17];
    case 'o':
        return segmentLookup[32];
    case 'O':
        return segmentLookup[0];
    }
    if((c>='A') && (c<='Z'))
    {
        c+=0x20; // convert every other character to lowercaser
    }
    if((c>='a' && c<='h')) //a..h
    {
        return segmentLookup[c-'a'+10];
    }

    switch(c) //all other
    {

    case 'j':
        return segmentLookup[18];
    case 'p':
        return segmentLookup[20];
    case '-':
        return segmentLookup[24];
    case 'n':
        return segmentLookup[31];
    case 't':
        return segmentLookup[33];
    case ' ':
        return segmentLookup[23];
    case 'r':
        return segmentLookup[21];
    case 'u':
        return segmentLookup[22];
    case 'l':
        return segmentLookup[19];
    }

    return segmentLookup[sizeof(segmentLookup)-1]; //character not found. displaying '_'
}

static const plc_hmi_led_rec led_cfg[] =
{
    //Leds
    {5,1<<PLC_HMI_LED_2},
    {5,1<<PLC_HMI_LED_3},
    {5,1<<PLC_HMI_LED_4},
    {5,1<<PLC_HMI_LED_5},
    {5,1<<PLC_HMI_LED_6},
    {5,1<<PLC_HMI_LED_7},
    {5,1<<PLC_HMI_LED_8},
    {5,1<<PLC_HMI_LED_9},
    //Param value dots
    {1,1<<7},
    {2,1<<7},
    {3,1<<7},
    {4,1<<7},
    //Param num dot
    {0,1<<7}
};

static void plc_hmi_convert(char * buff, uint32_t led_val)
{
    int i;
    uint32_t led_msk;

    for (i=0; i<(HMI_DIGITS-1); i++)
    {
        convert[i]=char_to_7seg(buff[i]); //process text
        convert[i]&=0x7f; //clear DP bit
    }

    for (i=0, led_msk = 1; i<sizeof(led_cfg); i++, led_msk <<= 1)
    {
        const plc_hmi_led_rec * cfg;

        cfg = led_cfg + i;

        if (led_val&led_msk)
        {
            convert[cfg->shift] |= cfg->msk;
        }
        else
        {
            convert[cfg->shift] &= ~cfg->msk;
        }
    }

    ready_flg = true;
}
//==================================================================
//                     Menu related things
//==================================================================
static void par_value_str(char* s, plc_hmi_par_t par_type, uint16_t val)
{
    switch(par_type)
    {
    case PLC_HMI_BOOL_TF:
    case PLC_HMI_RO_BOOL_TF:
        sprintf(s,(val!=0)?"True":"FAL5E");
        break;
    case PLC_HMI_BOOL_OO:
    case PLC_HMI_RO_BOOL_OO:
        sprintf(s,(val!=0)?" On ":" Off");
        break;
    case PLC_HMI_UINT:
    case PLC_HMI_RO_UINT:
        sprintf(s,"%4d",val);
        break;
    case PLC_HMI_HEX:
    case PLC_HMI_RO_HEX:
        sprintf(s,"%4x",val);
        break;
    case PLC_HMI_MMDD:
    case PLC_HMI_HHMM:
    case PLC_HMI_RO_MMDD:
    case PLC_HMI_RO_HHMM:
        sprintf(s,"%04d",val);
        break;
    default:
        sprintf(s,"Err ");
        break;
    }
}

static const char par_rep[] =
{
    '0','1','2','3','4','5','6','7','8','9',

    'a','b','c','d','e','f','g','h',

    'j','l','p','r','u'
};
#define PLC_HMI_GET_PAR_REP(num) ((num>22)?'?':par_rep[num])


static plc_hmi_t hmi;
///TODO: make extern
plc_hmi_dm_t plc_hmi_sys;
///TODO: write init, etc.
plc_hmi_dm_t plc_hmi_app;

#define PLC_HMI_MDL_DFLT plc_hmi_sys

static void plc_hmi_view(void)
{
    uint8_t i;
    uint8_t cur_par;
    plc_hmi_par_t ptype;

    cur_par = hmi.cur_par;
    ptype = hmi.mdl->ptype[cur_par];

    hmi.buf[0] = PLC_HMI_GET_PAR_REP(hmi.cur_par);

    if (PLC_HMI_STATE_VIEW == hmi.state)
    {
        par_value_str(&hmi.buf[1], ptype, hmi.mdl->par_get(cur_par));
    }
    else
    {
        par_value_str(&hmi.buf[1], ptype, hmi.tmp);
        if (!hmi.cur_show)
        {
            //Blink cursor on RW types
            switch (ptype)
            {
            case PLC_HMI_BOOL_OO:
            case PLC_HMI_BOOL_TF:
                for (i=1; i<HMI_DIGITS; i++)
                {
                    hmi.buf[i] = 0;//blink all!
                }
                break;
            case PLC_HMI_HEX:
            case PLC_HMI_UINT:
            case PLC_HMI_MMDD:
            case PLC_HMI_HHMM:
                hmi.buf[hmi.cursor] = 0;//blink one digit!
                break;
            case PLC_HMI_NOT_USED:
            default:
                break;
            }

        }
    }
    plc_hmi_convert(hmi.buf, hmi.mdl->leds);
}

static void plc_hmi_init(void)
{
    hmi.cur_par = 0;
    hmi.state   = PLC_HMI_STATE_VIEW;
    hmi.tmp     = 0;
    hmi.cursor  = 0;
    hmi.cur_show = true;
    hmi.mdl     = &(PLC_HMI_MDL_DFLT);
    plc_hmi_view();
}

static inline void exit_edit_mode(void)
{
    hmi.state  = PLC_HMI_STATE_VIEW;
}

#define HMI_CUR_START (HMI_DIGITS-2) //Last "digit" is not digit.

static void plc_hmi_controller(char input)
{
    uint8_t i;
    uint8_t cur_par;
    plc_hmi_par_t ptype;

    cur_par = hmi.cur_par;
    ptype = hmi.mdl->ptype[cur_par];

    if (PLC_HMI_STATE_VIEW == hmi.state)
    {
        switch(input)
        {
        case PLC_HMI_BTN_UP_L: //Move up
        case PLC_HMI_BTN_UP_S:
            for (i = hmi.mdl->psize; i>0; i++)//Limit iterations
            {
                if (PLC_HMI_NOT_USED != hmi.mdl->ptype[cur_par])
                {
                    hmi.cur_par = cur_par;
                    break;
                }
                cur_par--;
                if (0 == cur_par)
                {
                    cur_par = hmi.mdl->psize-1;
                }
            }
            break;
        case PLC_HMI_BTN_DW_L: //Move down
        case PLC_HMI_BTN_DW_S:
            for (i = hmi.mdl->psize; i>0; i++)//Limit iterations
            {
                if (PLC_HMI_NOT_USED != hmi.mdl->ptype[cur_par])
                {
                    hmi.cur_par = cur_par;
                    break;
                }
                cur_par++;
                if (0 == hmi.mdl->psize - cur_par)
                {
                    cur_par = 0;
                }
            }
            break;

        case PLC_HMI_BTN_OK_S: //Enter edit mode
            if (PLC_HMI_RO_START < ptype)
            {
                break; //Can't edit read only things
            }
            if (PLC_HMI_NOT_USED == ptype)
            {
                break; //Can't edit empty params
            }
            //Enter edit mode
            hmi.state  = PLC_HMI_STATE_EDIT;
            hmi.cursor = HMI_CUR_START;
            hmi.delta  = 1;
            hmi.tmp    = hmi.mdl->par_get(cur_par); //Fetch current param
            break;
        case PLC_HMI_BTN_OK_L: //Enter/Exit system menu
            if (&plc_hmi_app == hmi.mdl)
            {
                hmi.mdl = &plc_hmi_sys;
            }
            else
            {
                hmi.mdl = &plc_hmi_app;
            }
            break;
        default:
            break;
        }
    }
    else
    {
        uint32_t       mul = 10; //Defaul multiplier
        uint32_t max_delta = 1;
        uint32_t   max_val = 1;
        switch (ptype)
        {
        case PLC_HMI_BOOL_OO:
        case PLC_HMI_BOOL_TF:
            switch (input)
            {
            case PLC_HMI_BTN_UP_L: //Toggle val
            case PLC_HMI_BTN_UP_S:
                hmi.tmp = !hmi.tmp;
                break;
            case PLC_HMI_BTN_OK_S: //OK
                hmi.mdl->par_set(hmi.cur_par, hmi.tmp);
                //Now exit edit mode
            case PLC_HMI_BTN_OK_L: //Cansel
                exit_edit_mode();
                break;
            default: //Do nothing
                break;
            }
            break;
        case PLC_HMI_HEX:
            mul = 0x10; //Change default multiplier
        case PLC_HMI_UINT:
        case PLC_HMI_MMDD:
        case PLC_HMI_HHMM:
            for (i = 0; i<=HMI_CUR_START; i++)
            {
                max_delta *= mul;
            }
            max_val = max_delta * mul - 1;

            switch (input)
            {
            case PLC_HMI_BTN_UP_L: //Minus
                hmi.tmp -= hmi.delta;
                hmi.tmp %= max_val;
                hmi.tmp = hmi.mdl->par_chk(hmi.cur_par, (uint16_t)hmi.tmp);
                break;
            case PLC_HMI_BTN_UP_S: //Plus
                hmi.tmp += hmi.delta;
                hmi.tmp %= max_val;
                hmi.tmp = hmi.mdl->par_chk(hmi.cur_par, (uint16_t)hmi.tmp);
                break;
            case PLC_HMI_BTN_DW_L: //Prev digit
                hmi.cursor--;
                hmi.delta *= mul;
                if (0 == hmi.cursor)
                {
                    hmi.cursor = HMI_CUR_START;
                    hmi.delta  = 1;
                }
                break;
            case PLC_HMI_BTN_DW_S: //Next digit
                hmi.cursor++;
                hmi.delta *= mul;
                if (HMI_CUR_START < hmi.cursor)
                {
                    hmi.cursor = 0;
                    hmi.delta  = max_delta;
                }
                break;
            case PLC_HMI_BTN_OK_S: //OK
                hmi.mdl->par_set(hmi.cur_par, (uint16_t)hmi.tmp);
                //Now exit edit mode
            case PLC_HMI_BTN_OK_L: //Cansel
                exit_edit_mode();
                break;
            default: //Do nothing
                break;
            }
            break;
        case PLC_HMI_NOT_USED:
        default:
            break;
        }
    }
}

static void plc_hmi_start(void)
{
}

static void plc_hmi_poll(void)
{
}

static void plc_hmi_stop(void)
{
}
