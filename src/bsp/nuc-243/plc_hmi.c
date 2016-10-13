/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_wait_tmr.h>
#include <plc_hw.h>
#include <plc_iom.h>
#include <dbnc_flt.h>
#include <iec_std_lib.h>
#include <hmi_7seg_conf.h>



typedef struct
{
    enum rcc_periph_clken periph;
    uint32_t port;
    uint16_t pin;
} dio_t;

#define MIN(x,y) (((x)>(y))?(y):(x))

#define PLC_hmi_CONCAT(a,b) a##b
#define PLC_hmi_CONCAT2(a,b) PLC_hmi_CONCAT(a,b)

#define PLC_hmi_THING(t,n,name) (PLC_hmi_CONCAT2(PLC_hmi_CONCAT(PLC_HMI_,t),PLC_hmi_CONCAT(n,name)))

#define PLC_hmi_PERIPH(t,n) PLC_hmi_THING(t,n,_PERIPH)
#define PLC_hmi_PORT(t,n)   PLC_hmi_THING(t,n,_PORT)
#define PLC_hmi_PIN(t,n)    PLC_hmi_THING(t,n,_PIN)

#define PLC_hmi_REC(t,n) {PLC_hmi_PERIPH(t,n),PLC_hmi_PORT(t,n),PLC_hmi_PIN(t,n)}

static uint8_t buf1[HMI_DIGITS];
static uint8_t buf2[HMI_DIGITS];

static uint8_t* video_buffer = buf1;
static uint8_t* convert_buffer = buf2;
static bool conversion_ready = false;
char str_data[HMI_DIGITS+1];

static uint8_t menu_pos=0; //current position
static uint8_t number_place=0; //decimal/hexadecimal place
static uint8_t in_chnl;
static uint16_t menu_tmp = 0; //for edit mode
static uint32_t screensaver_timer=0;
static bool screensaver = true;

extern uint32_t mb_baudrate;
extern uint8_t mb_slave_addr;
extern dbnc_flt_t in_flt[];

extern uint16_t digital_out;
#define MENU_TYPE_SYS (1<<7)
#define MENU_MODE_EDIT (1<<6)
#define MENU_POS_MASK ~(0x3<<6)

const uint32_t menu_readonly = SYS_MENU_READONLY;
#define IS_READONLY(x) ((menu_readonly&(1<<(x&MENU_POS_MASK)))!=0)

#include "dbnc_flt.h"
static dbnc_flt_t btn_flt[HMI_NBUTTONS]; //Button debounce filters
static uint32_t pressed_timer[HMI_NBUTTONS]; //long press timers

uint8_t hmi_leds;
uint8_t dec_points;
uint8_t brightness=0;

typedef enum
{
    pt_bool_tf,
    pt_bool_oo,
    pt_word,
    pt_hex,
    pt_dt,
} param_type;


//HMI locations available to PLC application
#define PLC_HMI_NUM_PARAMS 16
static uint16_t app_params[PLC_HMI_NUM_PARAMS];
static param_type app_param_type[PLC_HMI_NUM_PARAMS];
static bool locations_initialized = false;

#define REGISTER_PARAM(n) params_used|=(1<<(n))
#define IS_PARAM_REGISTERED(n) ((params_used&(1<<n))!=0)

uint32_t params_used;

void flip_buffers(void)
{
    char* thirdglass;
    thirdglass = video_buffer;
    video_buffer = convert_buffer;
    convert_buffer = thirdglass;
    conversion_ready = false;
}

static const dio_t hmi[PLC_HMI_DO_NUM] =
{
    PLC_hmi_REC(O,0),
    PLC_hmi_REC(O,1),
};

static const dio_t anodes[HMI_DIGITS] =
{
    PLC_ANODE_REC(0),
    PLC_ANODE_REC(1),
    PLC_ANODE_REC(2),
    PLC_ANODE_REC(3),
    PLC_ANODE_REC(4),
    PLC_ANODE_REC(5)
};

static const dio_t segments[8] =
{
    PLC_SEG_REC(0),
    PLC_SEG_REC(1),
    PLC_SEG_REC(2),
    PLC_SEG_REC(3),
    PLC_SEG_REC(4),
    PLC_SEG_REC(5),
    PLC_SEG_REC(6),
    PLC_SEG_REC(7),
};

static const dio_t buttons[HMI_NBUTTONS] =
{
    PLC_BTN_REC(0),
    PLC_BTN_REC(1),
    PLC_BTN_REC(2),
};

uint8_t char_to_7seg(unsigned char c)
{
    if((c>='0') && (c<='9')) //numeric characters
    {
        return segmentLookup[c-'0'];
    }
    switch(c) //process characters that have both upper and lower case
    {
        case 'C': return segmentLookup[12];
        case 'c': return segmentLookup[34];
        case 'H': return segmentLookup[35];
        case 'h': return segmentLookup[17];
        case 'o': return segmentLookup[32];
        case 'O': return segmentLookup[0];
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

        case 'j': return segmentLookup[18];
        case 'p': return segmentLookup[20];
        case '-': return segmentLookup[24];
        case 'n': return segmentLookup[31];
        case 't': return segmentLookup[33];
        case ' ': return segmentLookup[23];
        case 'r': return segmentLookup[21];
        case 'u': return segmentLookup[22];
        case 'l': return segmentLookup[19];
    }

    return segmentLookup[sizeof(segmentLookup)-1]; //character not found. displaying '_'
}

void menu_next()
{
    int i;
    if((menu_pos&MENU_TYPE_SYS))
    {
        menu_pos++;
        if((menu_pos&MENU_POS_MASK) > HMI_MENU_SYS_LAST )//enu position overflow
            menu_pos&=~MENU_POS_MASK; //Set position to 0
    }
    else //go to next enabled parameter
    {
        for(i=0;i<=PLC_HMI_NUM_PARAMS;i++)
        {
            menu_pos++;
            if((menu_pos&MENU_POS_MASK)>PLC_HMI_NUM_PARAMS)//reached last parameter
                menu_pos&=~MENU_POS_MASK; //Set position to 0
            if(IS_PARAM_REGISTERED(menu_pos&MENU_POS_MASK))//stop on first registered parameter
                break;
        }
    }
}

void menu_previous()
{
    int i;
    if((menu_pos&MENU_TYPE_SYS))
    {
        if((menu_pos&MENU_POS_MASK) == 0 ) //menu underflow
        {
            menu_pos&=~MENU_POS_MASK; //Set position to 0
            menu_pos|=HMI_MENU_SYS_LAST;
        }
        else
        {
             menu_pos--;
        }
    }
    else //go to prevoius enabled parameter
    {
        for(i=0;i<=PLC_HMI_NUM_PARAMS;i++)
        {

            if((menu_pos&MENU_POS_MASK) == 0 ) //menu underflow
            {
                menu_pos&=~MENU_POS_MASK; //Set position to 0
                menu_pos|=PLC_HMI_NUM_PARAMS-1;
            }
            else
                 menu_pos--;
            if(IS_PARAM_REGISTERED(menu_pos&MENU_POS_MASK))//stop on first registered parameter
                break;
        }
    }
}
uint16_t menu_get_par(void)
{
    uint8_t num = menu_pos&MENU_POS_MASK;
    static tm now;
    if(menu_pos&MENU_TYPE_SYS)
    {
        if(num>=0 && num <= 2)
            plc_rtc_dt_get(&now);
        switch(num)
        {
        case 0: //YYYY
            return now.tm_year;
        case 1: //MM.DD
            return now.tm_day+now.tm_mon*100;
        case 2: //HH.MM
            return now.tm_min + now.tm_hour*100;
        case 3: //hmi display brightness
            return brightness;
        case 4:
            return in_chnl;
        case 5:
            return in_flt[in_chnl].thr_on;
        case 6:
            return in_flt[in_chnl].thr_off;
        case 7:
            return digital_out;

        //modbus baudrate
        case 19:
            return mb_baudrate/100;
        case 20:
            return mb_slave_addr;

        default:
            return 42; //dummy
        }

    }

    else
    {
        return app_params[num];
    }
}

param_type menu_get_par_type()
{
    uint8_t num = menu_pos&MENU_POS_MASK;
    if(menu_pos&MENU_TYPE_SYS)
    {
        if(num==1 || num==2) return pt_dt;
        if(num==7) return pt_hex;
        return pt_word; //dummy
    }
    else
    {
        return app_param_type[num];
    }
}

void menu_save_par(void)
{
    uint8_t num = menu_pos&MENU_POS_MASK;
    static tm now;
    if(menu_pos&MENU_TYPE_SYS)
    {
        if(num>=0 && num <= 2)
            plc_rtc_dt_get(&now);
        switch(num)
        {
        case 0: //YYYY
            now.tm_year = menu_tmp;
            plc_rtc_dt_set(&now);
            break;
        case 1: //MM.DD
            now.tm_day = menu_tmp%100;
            now.tm_mon = menu_tmp/100;
            plc_rtc_dt_set(&now);
            break;
        case 2: //HH.MM
            now.tm_min = menu_tmp%100;
            now.tm_hour = menu_tmp/100;
            plc_rtc_dt_set(&now);
            break;
        case 3: //hmi display brightness
            brightness = MIN(menu_tmp,7);
            break;
        case 4:
            in_chnl = MIN(menu_tmp,(PLC_DI_NUM-1));
            break;
        }

    }
    else
    {
            app_params[num]=menu_tmp%9999; //clamp decimal value to 4 decimal places
    }
}


void menu_increment()
{
    uint8_t digit;
    uint16_t mul;
    int i;
    param_type type = menu_get_par_type();
    switch (type)
    {
        case pt_bool_oo:
        case pt_bool_tf:
            if(menu_tmp!=0)
                menu_tmp=0;
            else
                menu_tmp=1;
            break;

        case pt_hex:
            {
                 mul = 1;
                for(i=number_place;i<3;i++)
                    mul*=0x10;
                digit = (menu_tmp/mul)%0x10;
                if(digit==0x0f)
                    menu_tmp-=mul*0x0f; //set place to 0
                else
                    menu_tmp+=mul; //we can just add 1*16^<place>
            }
            break;
        case pt_word:
        default:
            {
                mul = 1;
                for(i=number_place;i<3;i++)
                    mul*=10;
                digit = (menu_tmp/mul)%10;
                if(digit==9)
                    menu_tmp-=mul*9; //set place to 0
                else
                    menu_tmp+=mul; //we can just add 1*10^<place>
            }
    }


}

void button_pressed(uint8_t button,bool longpress)
{
    int cur_top = (menu_pos&MENU_TYPE_SYS)?HMI_MENU_SYS_LAST:HMI_MENU_PLC_LAST;
    uint8_t pos = menu_pos&MENU_POS_MASK;
    switch(button)
    {
    case HMI_BTN_UP:
        if(!(menu_pos&MENU_MODE_EDIT))
        {
             menu_next();
        }
        else
        {
            if((menu_pos&MENU_TYPE_SYS)&&((pos==3) || (pos==8) || (pos==15)))
                return;
            number_place++;
            if(number_place>3)
            {
                if((menu_pos&MENU_TYPE_SYS)&&(pos==4))
                    number_place=2;
                else
                    number_place=0;
            }
        }

        break;

    case HMI_BTN_OK:
        if(longpress==true)
        {
            if(menu_pos&MENU_MODE_EDIT)
            {
                menu_pos&=~MENU_MODE_EDIT;
            }
            else
            {
                menu_pos^=MENU_TYPE_SYS;
            }
        }
        //we do everything else on button release
        break;

    case (HMI_BTN_RELEASE|HMI_BTN_OK):
        if(menu_pos&MENU_MODE_EDIT)
        {
            menu_save_par();
        }
        else //entering edit mode
        {
            if(IS_READONLY(menu_pos)) break;
            menu_tmp = menu_get_par();
            number_place = 3;
        }
        menu_pos^=MENU_MODE_EDIT;
        break;

    case HMI_BTN_DOWN:
        if(!(menu_pos&MENU_MODE_EDIT))
        {
            menu_previous();
        }
        else
        {
            menu_increment();
        }
        break;
    default:
        //nop
        break;
    }

}

void hmi_display_poll(void )
{
    int i;
    if(screensaver_timer==0) return;
    for(i=0;i<(HMI_DIGITS-1);i++)
    {
        convert_buffer[i]=char_to_7seg(str_data[i]); //process text
    }

    convert_buffer[HMI_DIGITS-1]=hmi_leds; //process led states

    for(i=0;i<(HMI_DIGITS-1);i++) //process decimal point states
    {
        convert_buffer[i]&=0x7f; //clear DP bit
        convert_buffer[i]|=(dec_points<<(7-i)&0x80); //set 7-th bit for each place
    }
     //do not report conversion done while scr
        conversion_ready = true;
}

char menu_pos_to_char(uint8_t pos)
{
    switch(pos)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return '0'+menu_pos&MENU_POS_MASK;
            break;
        case 10: //a
        case 11: //b
        case 12: //c
        case 13: //d
        case 14: //e
        case 15: //f
        case 16: //g
        case 17: //h
            return 'a'+(menu_pos&MENU_POS_MASK)-10;
            break;
        case 18:
            return 'j'; break;
        case 19:
            return 'l'; break;
        case 20:
            return 'p'; break;
        case 21:
            return 'r'; break;
        case 22:
            return 'u';
            break;
        default:
            return '?';
          break;
        }
}

void par_value_str(char* s)
{
    uint16_t val;
    if(menu_pos&MENU_MODE_EDIT)
        val = menu_tmp;
    else
        val = menu_get_par();

    switch(menu_get_par_type())
    {
        case pt_bool_tf:
            sprintf(s,(val!=0)?"True":"FAL5E");
            break;
        case pt_bool_oo:
            sprintf(s,(val!=0)?" On ":" Off");
            break;
        case pt_word:
            sprintf(s,"%4d",val);
            break;
        case pt_hex:
            sprintf(s,"%4x",val);
            break;
        case pt_dt:
            sprintf(s,"%2d%02d",val/100,val%100);
            break;
        default:
            sprintf(s,"Err ");
            break;
    }
}

void menu_display_poll(uint32_t tick)
{
    static uint32_t blink_timer;
    static uint8_t counter;

    if(screensaver_timer!=0)
    {
        str_data[0]=menu_pos_to_char(menu_pos&MENU_POS_MASK); //display current position
        par_value_str(str_data+1); //display current parameter value
        /*

        */
    }

    if(tick>blink_timer)//.5s blink & animation timer
    {
        blink_timer=tick+HMI_REPEAT;
        counter++;
        if(counter>=HMI_NSS)
            counter = 0;
        if(screensaver_timer==0)//screensaver active
        {
            memset(convert_buffer,0,HMI_DIGITS);
            convert_buffer[screensaver_seq[counter][0]]= segmentLookup[screensaver_seq[counter][1]];
            conversion_ready = true;
        }
        else
        {
            if((menu_pos&MENU_TYPE_SYS)!=0)
               dec_points=counter%2; //blink 1st dot in sys menu
        }
    }

    if((menu_pos&MENU_MODE_EDIT)&&(counter%2))
        str_data[number_place+1]='_'; //blink value in edit mode
}

void hmi_button_poll(uint32_t tick)
{
    uint32_t i;
    static bool wait_exit_screensaver= false;
    static bool laststate[HMI_NBUTTONS];
    for (i = 0; i < HMI_NBUTTONS; i++)
    {
        dbnc_flt_poll(btn_flt+i, tick, !gpio_get(buttons[i].port, buttons[i].pin)); //get and filter button states
    }
    if((screensaver_timer!=0)&(tick > screensaver_timer)) //screensaver timeout passed
    {
        screensaver_timer=0;
    }
    for(i = 0; i < HMI_NBUTTONS; i++)
    {
        if(dbnc_flt_get( btn_flt + i)) //button is down
        {
            if(wait_exit_screensaver)
                break;

            if(screensaver_timer==0)
            {
                screensaver_timer = tick+HMI_SCREENASVER; //close screensaver and set timer
                 wait_exit_screensaver = true; //do not process button release or hold if this button invoked screensaver close
                 break;
            }
            screensaver_timer = tick+HMI_SCREENASVER; //close screensaver and set timer
            if(pressed_timer[i]==0)//button just pressed
            {
                pressed_timer[i] = tick+HMI_LONGPRESS;
                laststate[i] = true;
                button_pressed(i,false); //Button pressed event(edge)
                break;
            }
            if((pressed_timer[i]!=0) && (pressed_timer[i]<tick)) //hold timeout/repeat cycle
            {
                if(laststate[i]==false) break;
                pressed_timer[i] = tick+HMI_REPEAT;
                if(i==HMI_BTN_OK) //no repeat for OK button
                    laststate[i] = false;
                button_pressed(i,true); //Long press event/repeat event
                break;
            }
        }
        else //button is up
        {
            if(i==(HMI_NBUTTONS-1))//all buttons up
            {
                if(wait_exit_screensaver) laststate[i] = false; //do not send release event when closing screensaver
                wait_exit_screensaver = false;
            }

            if(laststate[i]==true) //Button was just released
                button_pressed(i+128,false);

            laststate[i]=false; //reset everything
            pressed_timer[i]=0;
        }
    }
}

void dynamic_7seg_poll(void)
{
    static uint8_t pwm=0;
    static uint8_t anode_n=0;
    uint8_t i;
    void (*do_set)(uint32_t, uint16_t);

    if(pwm>7) //end of one pwm cycle
    {
        gpio_clear(anodes[anode_n].port,anodes[anode_n].pin); //switch off previous char
        for(i=0;i<8;i++)
        {
            gpio_clear(segments[i].port, segments[i].pin);     //set segment values
        }
        anode_n++; //go to next place
        if(anode_n>=HMI_DIGITS) //End of frame
        {
            anode_n = 0;
        }

        if(conversion_ready==true) //we have new data to display
                flip_buffers();

        for(i=0;i<8;i++)
        {
            do_set = ((video_buffer[anode_n]&(1<<i))!=0)?gpio_set:gpio_clear;
            do_set(segments[i].port, segments[i].pin);     //set segment values
        }
        gpio_set(anodes[anode_n].port,anodes[anode_n].pin); //turn on new char
        pwm=0;
    }

   if(pwm>brightness)
    {
       gpio_clear(anodes[anode_n].port,anodes[anode_n].pin);
    }
    pwm++;

}



void plc_hmi_set_dout( uint32_t i, bool val )
{
    void (*do_set)(uint32_t, uint16_t);

    if (PLC_DO_NUM<=i)
    {
        return;
    }
    do_set = (val)?gpio_set:gpio_clear;

    do_set(hmi[i].port, hmi[i].pin);
}

//HMI interface (dual led)
#define LOCAL_PROTO plc_hmi
void PLC_IOM_LOCAL_INIT(void)
{
    //Anodes init
    uint32_t i;
    for (i=0; i<HMI_DIGITS; i++)
    {
        rcc_periph_clock_enable(anodes[i].periph );
        gpio_mode_setup        (anodes[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, anodes[i].pin);
        gpio_set_output_options(anodes[i].port, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, anodes[i].pin);
        gpio_clear             (anodes[i].port, anodes[i].pin);
    }

    //Segment lines init
    for (i=0; i<8; i++)
    {
        rcc_periph_clock_enable(segments[i].periph );
        gpio_mode_setup        (segments[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, segments[i].pin);
        gpio_set_output_options(segments[i].port, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, segments[i].pin);
        gpio_clear             (segments[i].port, segments[i].pin);
    }

    //Buttons init
    for (i=0; i<HMI_NBUTTONS; i++)
    {
        dbnc_flt_init(btn_flt+i);

        rcc_periph_clock_enable(buttons[i].periph);
        gpio_mode_setup          (buttons[i].port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, buttons[i].pin);
    }
}

bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

const char plc_hmi_err_dir[]      = "Only output and memory locations are supported for HMI";
const char plc_hmi_err_type[]      = "Only BOOL type output locations are supported for HMI";
const char plc_hmi_err_olim[]    = "HMI output must have address 0...1!";
const char plc_hmi_err_mlim[]    = "Parameter address out of limits!";
const char plc_hmi_err_multi[] = "Memory location address must be unique!";
const char plc_hmi_err_mem_type[] = "Memory location data type must be bool or word!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;

    addr = PLC_APP->l_tab[i]->a_data[0];


    if(PLC_APP->l_tab[i]->v_type == PLC_LT_Q ) //led outputs
    {
        if( PLC_APP->l_tab[i]->v_size == PLC_LSZ_X )
        {

            if ((PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_DOTS) < addr)
            {
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_olim, sizeof(plc_hmi_err_olim));
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_type, sizeof(plc_hmi_err_type));
            return false;
        }
    }
    else if(PLC_APP->l_tab[i]->v_type == PLC_LT_M ) //menu parameters
    {
        if (PLC_HMI_NUM_PARAMS < addr)
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_mlim, sizeof(plc_hmi_err_mlim));
            return false;
        }
        else
        {
            if(!IS_PARAM_REGISTERED(addr))
            {
                REGISTER_PARAM(addr);
                if(PLC_APP->l_tab[i]->a_size!=2)
                {
                    app_param_type[addr] = (PLC_APP->l_tab[i]->v_size == PLC_LSZ_X)?pt_bool_tf:pt_word;
                }
                else //type specified explicitly
                {
                    if(PLC_APP->l_tab[i]->v_size == PLC_LSZ_X)
                    {
                        if(PLC_APP->l_tab[i]->a_data[1]==1)
                            app_param_type[addr] = pt_bool_oo;
                        else
                            app_param_type[addr] = pt_bool_tf;

                    }
                    else if(PLC_APP->l_tab[i]->v_size == PLC_LSZ_W)
                    {
                        if(PLC_APP->l_tab[i]->a_data[1]==1)
                            app_param_type[addr] = pt_hex;
                        else
                            app_param_type[addr] = pt_word;
                    }
                    else //bad type
                    {
                        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_mem_type, sizeof(plc_hmi_err_mem_type));
                        return false;
                    }
                }
                return true;
            }
            else
            {
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_multi, sizeof(plc_hmi_err_multi));
                return false;
            }
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
    hmi_button_poll(tick);
    menu_display_poll(tick);
    hmi_display_poll();
}
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}
uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    if(!locations_initialized) return 0;
    if(PLC_APP->l_tab[i]->v_type == PLC_LT_M)
    {
        switch(PLC_APP->l_tab[i]->v_size)
        {
        case PLC_LSZ_X:
            *(bool *)(plc_curr_app->l_tab[i]->v_buf) =  app_params[(plc_curr_app->l_tab[i]->a_data[0])];
            break;
        case PLC_LSZ_W:
            *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf) =  app_params[(plc_curr_app->l_tab[i]->a_data[0])];
            break;
        }
    }
    return 0;
}
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    locations_initialized = true;
    int addr = (plc_curr_app->l_tab[i]->a_data[0]);
    if(PLC_APP->l_tab[i]->v_type == PLC_LT_Q ) //led outputs
    {
        if(addr<PLC_HMI_DO_NUM) //RxTx led
        {
            plc_hmi_set_dout( plc_curr_app->l_tab[i]->a_data[0], *(bool *)(plc_curr_app->l_tab[i]->v_buf) );
        }
        else if((PLC_HMI_DO_NUM <= addr) && (addr< (PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_LEDS))) //main leds
        {
            if(*(bool *)(plc_curr_app->l_tab[i]->v_buf)==true)
                hmi_leds |=(1<<addr-PLC_HMI_DO_NUM);
            else
                hmi_leds &=~(1<<(addr-PLC_HMI_DO_NUM));
        }//dots
        else if(((PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_LEDS) <= addr) && (addr< (PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_LEDS+PLC_HMI_DISPLAY_DOTS))) //dots
        {
            if(*(bool *)(plc_curr_app->l_tab[i]->v_buf)==true)
                dec_points |=(1<<addr-PLC_HMI_DO_NUM-PLC_HMI_DISPLAY_LEDS);
            else
                dec_points &=~(1<<(addr-PLC_HMI_DO_NUM));
        }
    }
    else if(PLC_APP->l_tab[i]->v_type == PLC_LT_M)
    {
        switch(PLC_APP->l_tab[i]->v_size)
        {
        case PLC_LSZ_X:
            app_params[addr] = *(bool *)(plc_curr_app->l_tab[i]->v_buf);
            break;
        case PLC_LSZ_W:
            app_params[addr] = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
            break;
            default:
            break;
        }
    }
    return 0;
}
#undef LOCAL_PROTO
