/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
    dbnc_flt_poll(&self->flt, tick, in);

    if (PLC_HMI_BTN_DOWN == self->state)
    {
        if (PLC_HMI_BTN_UP == self->flt.flg)
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
        if (PLC_HMI_BTN_DOWN == self->flt.flg)
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

void plc_hmi_kb_init(void)
{
    uint8_t i;
    for (i=0; i < BUTTONS_NUM; i++)
    {
        plc_gpio_cfg_in(button     + i);
        btn_fsm_init   (button_fsm + i, button_cfg[i][0], button_cfg[i][1]);
    }
}

char plc_hmi_kb_poll(uint32_t tick)
{
    uint8_t i;
    volatile char ret = PLC_HMI_BTN_NO_CHAR;

    for (i=0; i < BUTTONS_NUM; i++)
    {
        ret = btn_fsm_poll(button_fsm + i, tick, !plc_gpio_get(button + i));
        if (PLC_HMI_BTN_NO_CHAR != ret)
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
uint8_t         plc_hmi_bri = 0;
char            str_buf[HMI_DIGITS+1];

static inline void swap_buf(void)
{
    char* tmp;

    tmp = video;
    video = convert;
    convert= tmp;
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

void plc_hmi_vout_init(void)
{
    //LED
    PLC_GPIO_GR_CFG_OUT(hmi_led);
    //Anodes init
    PLC_GPIO_GR_CFG_OUT(anodes);
    //Segment lines init
    PLC_GPIO_GR_CFG_OUT(segments);

    plc_hmi_bri = MIN(plc_backup_load_brightness(),PLC_HMI_BRI_LIM);
}

void plc_hmi_vout_poll(void)
{
    static uint8_t pwm=0;
    static uint8_t anode_n=0;
    static bool clr_flg = true;
    uint8_t i;

    if (pwm>7) //end of one pwm cycle
    {
        uint8_t * frame;
        frame = video;
        anode_n++; //go to next place
        if (anode_n >= HMI_DIGITS) //End of frame
        {
            anode_n = 0;
        }
        //set segment values
        for (i=0; i<8; i++)
        {
            if ((frame[anode_n]&(1<<i)) == 0)
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
        clr_flg = true;
    }

    if ((clr_flg) && (pwm > plc_hmi_bri))
    {
        clr_flg = false;
        plc_gpio_clear(anodes + anode_n);
        for (i=0; i<8; i++)
        {
            plc_gpio_clear(segments + i);
        }
    }
    pwm++;
}

static void plc_hmi_set_dout(uint32_t i, bool val)
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
    if ((c>='0') && (c<='9')) //numeric characters
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
    if ((c>='A') && (c<='Z'))
    {
        c+=0x20; // convert every other character to lowercaser
    }
    if ((c>='a' && c<='h')) //a..h
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

#define PLC_HMI_PAR_NUM_DOT (1<<12)
#define PLC_HMI_PAR_TIME_DOT (1<<9)

static void plc_hmi_convert(char * buff, uint32_t led_val)
{
    int i;
    uint32_t led_msk;

    for (i=0; i<(HMI_DIGITS-1); i++)
    {
        convert[i]=char_to_7seg(buff[i]); //process text
        convert[i]&=0x7f; //clear DP bit
    }


    led_msk = 1;
    for (i=0; i<sizeof(led_cfg)/sizeof(plc_hmi_led_rec); i++)
    {
        const plc_hmi_led_rec * cfg;

        cfg = led_cfg + i;

        if (led_val&led_msk)
        {
            convert[cfg->shift] |= (cfg->msk);
        }
        else
        {
            convert[cfg->shift] &= ~(cfg->msk);
        }
        led_msk <<= 1;
    }

    PLC_DISABLE_INTERRUPTS();
    swap_buf();
    PLC_ENABLE_INTERRUPTS();
}
//==================================================================
//                     Menu related things
//==================================================================
static void par_value_str(char* s, plc_hmi_par_t par_type, int32_t val)
{
    switch(par_type)
    {
    case PLC_HMI_BOOL_TF:
    case PLC_HMI_RO_BOOL_TF:
        sprintf(s,(val!=0)?"True":"FAL5");
        break;
    case PLC_HMI_BOOL_OO:
    case PLC_HMI_RO_BOOL_OO:
        sprintf(s,(val!=0)?" On ":" Off");
        break;
    case PLC_HMI_UINT:
    case PLC_HMI_RO_UINT:
        sprintf(s,"%4d",val);
        break;
    case PLC_HMI_SINT:
    case PLC_HMI_RO_SINT:
        {
            if (val>HMI_MAX_SINT)
            {
                val = HMI_MAX_SINT;
            }
            if (val<HMI_MIN_SINT)
            {
                val = HMI_MIN_SINT;
            }
            sprintf(s, "%4d", (int)val);
        }
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
    case PLC_HMI_NOT_USED:
    case PLC_HMI_EMPTY:
        sprintf(s,"    ");
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

static char     hmi_sys_poll(uint32_t tick, char input);

extern const plc_hmi_par_t plc_hmi_sys_ptype[];

plc_hmi_dm_t plc_hmi_sys =
{
    .leds  = 0,
    .ptype = (plc_hmi_par_t*)plc_hmi_sys_ptype,

    .par_get = hmi_sys_get,
    .par_set = hmi_sys_set,
    .par_chk = hmi_sys_chk,
    .poll    = hmi_sys_poll,

    .psize = PLC_HMI_SYS_PSIZE
};

static char     hmi_app_poll(uint32_t tick, char input);
static int32_t hmi_app_get(uint8_t par);
static int32_t hmi_app_chk(uint8_t par, int32_t val);
static void     hmi_app_set(uint8_t par, int32_t val);

#define PLC_HMI_NUM_PARAMS 16

static plc_hmi_par_t plc_hmi_app_ptype[] =
{
    PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED,
    PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED,
    PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED,
    PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED, PLC_HMI_NOT_USED
};

plc_hmi_dm_t plc_hmi_app =
{
    .leds  = 0,
    .ptype = plc_hmi_app_ptype,

    .par_get = hmi_app_get,
    .par_set = hmi_app_set,
    .par_chk = hmi_app_chk,
    .poll    = hmi_app_poll,

    .psize = PLC_HMI_NUM_PARAMS
};

plc_hmi_t hmi;
#define HMI_SCREENASVER 30000

static uint8_t ss_par = 1;

static char hmi_app_poll(uint32_t tick, char input)
{
    static uint32_t ss_tmr = 0;
    static bool ss_mode = false;
    static uint8_t saved_par;
    static uint8_t saved_plc_hmi_bri;

    plc_hmi_app.leds |= PLC_HMI_PAR_NUM_DOT;

    if (PLC_HMI_BTN_NO_CHAR != input)
    {
        ss_tmr = tick;
        if (ss_mode)
        {
            ss_mode     = false;
            hmi.cur_par = saved_par;
            hmi.state   = PLC_HMI_STATE_VIEW;
            plc_hmi_bri = saved_plc_hmi_bri;

            return PLC_HMI_BTN_NO_CHAR;
        }
    }

    if ((HMI_SCREENASVER < (tick - ss_tmr)) && !ss_mode)
    {
        ss_mode          = true;
        saved_par        = hmi.cur_par;
        hmi.cur_par      = ss_par;
        hmi.state        = PLC_HMI_STATE_VIEW;
        saved_plc_hmi_bri = plc_hmi_bri;
        plc_hmi_bri       >>= 1;
    }

    return input;
}

static int32_t hmi_app_pdata[16];

static int32_t hmi_app_get(uint8_t par)
{
    return hmi_app_pdata[par];
}
static int32_t hmi_app_chk(uint8_t par, int32_t val)
{
    (void)par;
    return val;
}
static void     hmi_app_set(uint8_t par, int32_t val)
{
    hmi_app_pdata[par] = val;
}
//===================================================================
static char hmi_sys_poll(uint32_t tick, char input)
{
    (void)tick;

    switch (hmi.mdl->ptype[hmi.cur_par])
    {
    case PLC_HMI_MMDD:
    case PLC_HMI_HHMM:
        plc_hmi_sys.leds |= PLC_HMI_PAR_TIME_DOT;
        break;
    default:
        plc_hmi_sys.leds &= ~PLC_HMI_PAR_TIME_DOT;
        break;
    }

    if (hmi.cur_show)
    {
        plc_hmi_sys.leds |= PLC_HMI_PAR_NUM_DOT;
    }
    else
    {
        plc_hmi_sys.leds &= ~PLC_HMI_PAR_NUM_DOT;
    }

    return input;
}
//===================================================================
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
                    hmi.buf[i] = ' ';//blink all!
                }
                break;
            case PLC_HMI_MMDD:
            case PLC_HMI_HHMM:
            case PLC_HMI_HEX:
            case PLC_HMI_UINT:
            case PLC_HMI_SINT:
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
static void find_par(void)
{
    uint8_t i;
    hmi.cur_par = 0;
    for (i = 0; i<hmi.mdl->psize; i++)
    {
        if (PLC_HMI_NOT_USED != hmi.mdl->ptype[i])
        {
            hmi.cur_par = i;
            break;
        }
    }
}
#define PLC_HMI_MDL_DFLT plc_hmi_app
void _plc_hmi_init(void)
{
    uint8_t i;
    plc_hmi_kb_init();
    plc_hmi_vout_init();

    hmi.cur_par = 0;
    hmi.state   = PLC_HMI_STATE_VIEW;
    hmi.tmp     = 0;
    hmi.cursor  = 0;
    hmi.cur_show = true;

    if (0 != PLC_HMI_MDL_DFLT.psize)
    {
        hmi.mdl  = &(PLC_HMI_MDL_DFLT);
    }
    else
    {
        hmi.mdl  = &plc_hmi_sys;
    }

    find_par();

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
                if (0 == cur_par)
                {
                    cur_par = hmi.mdl->psize-1;
                }
                else
                {
                    cur_par--;
                }
                if (PLC_HMI_NOT_USED != hmi.mdl->ptype[cur_par])
                {
                    break;
                }
            }
            hmi.cur_par = cur_par;
            break;
        case PLC_HMI_BTN_DW_L: //Move down
        case PLC_HMI_BTN_DW_S:
            for (i = hmi.mdl->psize; i>0; i++)//Limit iterations
            {
                if (hmi.mdl->psize-1 == cur_par)
                {
                    cur_par = 0;
                }
                else
                {
                    cur_par++;
                }
                if (PLC_HMI_NOT_USED != hmi.mdl->ptype[cur_par])
                {
                    break;
                }
            }
            hmi.cur_par = cur_par;
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
            find_par();
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
        case PLC_HMI_SINT:
        case PLC_HMI_MMDD:
        case PLC_HMI_HHMM:
            for (i = 0; i<HMI_CUR_START-1; i++)
            {
                max_delta *= mul;
            }
            max_val = max_delta * mul;

            switch (input)
            {
            case PLC_HMI_BTN_UP_L: //Minus
                if(ptype==PLC_HMI_SINT)
                {
                    hmi.tmp -= hmi.delta;
                    if (hmi.tmp<HMI_MIN_SINT)
                    {
                        hmi.tmp = HMI_MAX_SINT;
                    }
                }else if (hmi.tmp < hmi.delta)
                {
                    hmi.tmp += hmi.delta*(mul-1);
                }
                else
                {
                    hmi.tmp -= hmi.delta;
                }
                hmi.tmp = hmi.mdl->par_chk(hmi.cur_par, hmi.tmp);
                break;
            case PLC_HMI_BTN_UP_S: //Plus

                hmi.tmp += hmi.delta;
                if (PLC_HMI_SINT == ptype)
                {
                    if (HMI_MAX_SINT < hmi.tmp)
                    {
                        hmi.tmp = HMI_MIN_SINT;
                    }
                }else if (max_val <= hmi.tmp)
                {
                    hmi.tmp %= max_val;
                }
                hmi.tmp = hmi.mdl->par_chk(hmi.cur_par, hmi.tmp);
                break;
            case PLC_HMI_BTN_DW_L: //Prev digit
                hmi.cursor++;
                hmi.delta *= mul;
                if (HMI_CUR_START < hmi.cursor)
                {
                    hmi.cursor = 1;
                    hmi.delta  = max_delta;
                }
                break;
            case PLC_HMI_BTN_DW_S: //Next digit
                hmi.cursor--;
                hmi.delta *= mul;
                if (0 == hmi.cursor)
                {
                    hmi.cursor = HMI_CUR_START;
                    hmi.delta  = 1;
                }
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
        default:
            break;
        }
    }
}

void _plc_hmi_poll(uint32_t tick)
{
    static uint32_t blink;
    volatile char key;

    if (500 < (tick - blink))
    {
        blink = tick;
        hmi.cur_show = !hmi.cur_show;
    }
    //Controller
    key = plc_hmi_kb_poll(tick);
    //Model
    key = hmi.mdl->poll(tick, key);

    if (PLC_HMI_BTN_NO_CHAR != key)
    {
        plc_hmi_controller(key);
    }
    //View
    plc_hmi_view();
}


//HMI interface (dual led)
#define LOCAL_PROTO plc_hmi
void PLC_IOM_LOCAL_INIT(void)
{
    _plc_hmi_init();
}

bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

const char plc_hmi_err_dir[]      = "Only output and memory locations are supported for HMI";
const char plc_hmi_err_type[]      = "Only BOOL type output locations are supported for HMI";
const char plc_hmi_err_olim[]    = "HMI output must have address 0...13!";
const char plc_hmi_err_mlim[]    = "Parameter address out of limits!";
const char plc_hmi_err_multi[] = "Memory location address must be unique!";
const char plc_hmi_err_mem_type[] = "Memory location data type must be bool or word!";
const char plc_hmi_err_addr_sz[]    = "Wrong address size";
const char plc_hmi_err_repr[]    = "Wrong parameter representation!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;

    addr = PLC_APP->l_tab[i]->a_data[0];


    if (PLC_APP->l_tab[i]->v_type == PLC_LT_Q) //led outputs and screensaver parameter
    {
        if (PLC_APP->l_tab[i]->v_size == PLC_LSZ_X)
        {
            if ((PLC_HMI_DO_NUM+PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_DOTS) <= addr)
            {
                PLC_LOG_ERROR(plc_hmi_err_olim);
                return false;
            }
            else
            {
                return true;
            }
        }
        else if (PLC_APP->l_tab[i]->v_size == PLC_LSZ_B)
        {
            if (addr!=0)
            {
                PLC_LOG_ERROR(plc_hmi_err_mlim);
                return false;
            }
            else
                return true;
        }
        else
        {
            PLC_LOG_ERROR(plc_hmi_err_type);
            return false;
        }
    }
    else if (PLC_APP->l_tab[i]->v_type == PLC_LT_M) //menu parameters
    {
        if (PLC_HMI_NUM_PARAMS < addr)
        {
            PLC_LOG_ERROR(plc_hmi_err_mlim);
            return false;
        }
        else
        {
            if (PLC_HMI_NOT_USED == plc_hmi_app_ptype[addr])
            {
                int ptid;
                if (PLC_APP->l_tab[i]->a_size!=2)
                {
                    PLC_LOG_ERROR(plc_hmi_err_addr_sz);
                    return false;
                }

                ptid = PLC_APP->l_tab[i]->a_data[1];

                switch(PLC_APP->l_tab[i]->v_size)
                {
                case PLC_LSZ_X:
                {
                    static const plc_hmi_par_t ptype[] = {PLC_HMI_BOOL_TF, PLC_HMI_BOOL_OO, PLC_HMI_RO_BOOL_TF, PLC_HMI_RO_BOOL_OO};
                    if (ptid < sizeof(ptype)/sizeof(plc_hmi_par_t))
                    {
                        plc_hmi_app_ptype[addr] = ptype[ptid];
                        return true;
                    }
                    else
                    {
                        PLC_LOG_ERROR(plc_hmi_err_repr);
                        return false;
                    }
                }
                case PLC_LSZ_W:
                {
                    static const plc_hmi_par_t ptype[] = {PLC_HMI_UINT, PLC_HMI_SINT, PLC_HMI_HEX, PLC_HMI_RO_UINT, PLC_HMI_RO_SINT, PLC_HMI_RO_HEX};
                    if (ptid < sizeof(ptype)/sizeof(plc_hmi_par_t))
                    {
                        plc_hmi_app_ptype[addr] = ptype[ptid];
                        return true;
                    }
                    else
                    {
                        PLC_LOG_ERROR(plc_hmi_err_repr);
                        return false;
                    }
                }
                default:
                {
                    PLC_LOG_ERROR(plc_hmi_err_mem_type);
                    return false;
                }
                }
            }
            else
            {
                PLC_LOG_ERROR(plc_hmi_err_multi);
                return false;
            }
        }
    }
    else if (PLC_APP->l_tab[i]->v_type == PLC_LT_I)
    {
        if (PLC_APP->l_tab[i]->v_size == PLC_LSZ_B) //menu current position location
        {
            if (addr==0)
            {
                return true;
            }
            else
            {
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_mlim, sizeof(plc_hmi_err_mlim));
                return false;
            }
        }
        else //bad type
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_mem_type, sizeof(plc_hmi_err_mem_type));
            return false;
        }
    }
    else
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_dir, sizeof(plc_hmi_err_dir));
        return false;
    }

}

void PLC_IOM_LOCAL_BEGIN(uint16_t lid)
{
}
void PLC_IOM_LOCAL_END(uint16_t lid)
{
}

void PLC_IOM_LOCAL_START(void)
{
    find_par();
    plc_hmi_view();
}

void PLC_IOM_LOCAL_STOP(void)
{
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    _plc_hmi_poll(tick);
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}

static bool loc_init = false;

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    if (!loc_init)
    {
        return 0;
    }
    if (plc_curr_app->l_tab[i]->v_type == PLC_LT_M)
    {
        switch (plc_hmi_app_ptype[plc_curr_app->l_tab[i]->a_data[0]])
        {
        case PLC_HMI_BOOL_OO:
        case PLC_HMI_BOOL_TF:
        case PLC_HMI_RO_BOOL_OO:
        case PLC_HMI_RO_BOOL_TF:
            *(bool *)(plc_curr_app->l_tab[i]->v_buf) = (0 != hmi_app_pdata[(plc_curr_app->l_tab[i]->a_data[0])]);
            break;
        case PLC_HMI_SINT: case PLC_HMI_RO_SINT:
            *(int16_t *)(plc_curr_app->l_tab[i]->v_buf) =  (int16_t)hmi_app_pdata[(plc_curr_app->l_tab[i]->a_data[0])];
            break;
        case PLC_HMI_UINT:
        case PLC_HMI_HEX:
        case PLC_HMI_RO_UINT:
        case PLC_HMI_RO_HEX:
            *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf) =  (uint16_t)hmi_app_pdata[(plc_curr_app->l_tab[i]->a_data[0])];
            break;
        default:
            break;
        }
    }
    else if (plc_curr_app->l_tab[i]->v_type == PLC_LT_I)
    {
        *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf) = hmi.cur_par;
    }
    return 0;
}
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    int addr;

    loc_init = true;

    addr = (plc_curr_app->l_tab[i]->a_data[0]);

    if (plc_curr_app->l_tab[i]->v_type == PLC_LT_Q) //led outputs and screensaver parameter
    {
        if (plc_curr_app->l_tab[i]->v_size == PLC_LSZ_X)
        {
            if (addr<PLC_HMI_DO_NUM) //RxTx led
            {
                plc_hmi_set_dout(plc_curr_app->l_tab[i]->a_data[0], *(bool *)(plc_curr_app->l_tab[i]->v_buf));
            }
            else if ((PLC_HMI_DO_NUM <= addr) && (addr< (PLC_HMI_DO_NUM+PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_DOTS))) //main leds
            {
#define         PLC_HMI_LED_MSK(a) (1<<(a-PLC_HMI_DO_NUM))
                if (*(bool *)(plc_curr_app->l_tab[i]->v_buf)==true)
                {
                    plc_hmi_app.leds |= PLC_HMI_LED_MSK(addr);
                }
                else
                {
                    plc_hmi_app.leds &=~PLC_HMI_LED_MSK(addr);
                }
            }
        }
        else if (plc_curr_app->l_tab[i]->v_size == PLC_LSZ_B) //screensaver parameter is the only byte value for now
        {
            ss_par = *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        }

    }
    else if (plc_curr_app->l_tab[i]->v_type == PLC_LT_M)
    {
        switch (plc_hmi_app_ptype[addr])
        {
        case PLC_HMI_BOOL_OO:
        case PLC_HMI_BOOL_TF:
        case PLC_HMI_RO_BOOL_OO:
        case PLC_HMI_RO_BOOL_TF:
            hmi_app_pdata[addr] = *(bool *)(plc_curr_app->l_tab[i]->v_buf);
            break;
        case PLC_HMI_SINT: case PLC_HMI_RO_SINT:
            hmi_app_pdata[addr] = *(int16_t *)(plc_curr_app->l_tab[i]->v_buf);
            break;
        case PLC_HMI_UINT:
        case PLC_HMI_HEX:
        case PLC_HMI_RO_UINT:
        case PLC_HMI_RO_HEX:
            hmi_app_pdata[addr] = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
            break;
        default:
            break;
        }
    }
    return 0;
}
#undef LOCAL_PROTO
