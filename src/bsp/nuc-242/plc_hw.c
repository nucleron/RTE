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
#include <plc_iom.h>
#include <plc_dbg.h>
#include <plc_wait_tmr.h>
#include <plc_hw.h>


void plc_jmpr_init(void)
{
    rcc_periph_clock_enable(PLC_JMP_RST_PERIPH);
    gpio_set_mode(PLC_JMP_RST_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, PLC_JMP_RST_PIN);
    gpio_clear(PLC_JMP_RST_PORT, PLC_JMP_RST_PIN); //Pull down!

    rcc_periph_clock_enable(PLC_JMP_DBG_PERIPH);
    gpio_set_mode(PLC_JMP_DBG_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, PLC_JMP_DBG_PIN);
    gpio_set(PLC_JMP_DBG_PORT, PLC_JMP_DBG_PIN); //Pull up!
}

bool plc_dbg_jmpr_get(void)
{
    bool ret = !gpio_get(PLC_JMP_DBG_PORT, PLC_JMP_DBG_PIN);
    return ret;
}

volatile bool reset_jmp = false;

bool plc_rst_jmpr_get(void)
{
    return gpio_get(PLC_JMP_RST_PORT, PLC_JMP_RST_PIN);
}

void plc_boot_init(void)
{
    //Boot pin config
    rcc_periph_clock_enable( RCC_AFIO );
    AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

    rcc_periph_clock_enable( PLC_BOOT_PERIPH );
    gpio_set_mode(PLC_BOOT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PLC_BOOT_PIN);
    gpio_set( PLC_BOOT_PORT, PLC_BOOT_PIN ); //Exit boot mode
}

void plc_boot_mode_enter(void)
{
    uint32_t delay;
    gpio_clear( PLC_BOOT_PORT, PLC_BOOT_PIN );

    PLC_CLEAR_TIMER( delay );
    while( PLC_TIMER(delay) < 2000 );
    scb_reset_system();
}

/*
//Led blink timer
static uint32_t blink_tmr;
void plc_heart_beat_init(void)
{
    //LEDs
    PLC_CLEAR_TIMER( blink_tmr );

    rcc_periph_clock_enable( PLC_LED_STG_PERIPH );
    gpio_set_mode(PLC_LED_STG_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PLC_LED_STG_PIN);
    gpio_clear( PLC_LED_STG_PORT, PLC_LED_STG_PIN );

    rcc_periph_clock_enable( PLC_LED_STR_PERIPH );
    gpio_set_mode(PLC_LED_STR_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PLC_LED_STR_PIN);
    gpio_clear( PLC_LED_STR_PORT, PLC_LED_STR_PIN );

    rcc_periph_clock_enable( PLC_LED_TX_PERIPH );
    gpio_set_mode(PLC_LED_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PLC_LED_TX_PIN);
    gpio_clear( PLC_LED_TX_PORT, PLC_LED_TX_PIN );

    rcc_periph_clock_enable( PLC_LED_RX_PERIPH );
    gpio_set_mode(PLC_LED_RX_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PLC_LED_RX_PIN);
    gpio_clear( PLC_LED_RX_PORT, PLC_LED_RX_PIN );
}

extern uint32_t plc_backup_satus;

static bool hse_post_flag = true;
const char plc_hse_err_msg[] = "HSE oscilator failed!";
static bool lse_post_flag = true;
const char plc_lse_err_msg[] = "LSE oscilator failed!";
static bool dl_post_flag = true;
const char plc_dl_err_msg[] = "Deadline violation detected, PLC is stoped!";
void plc_heart_beat_poll(void)
{
    uint32_t blink_thr;
    if( plc_diag_status > 0 )
    {
        blink_thr = 500;
    }
    else
    {
        blink_thr = 1000;
    }

    if( PLC_TIMER(blink_tmr) > (blink_thr>>1) )
    {
        if (plc_diag_status  & PLC_DIAG_ERR_CRITICAL)
        {
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        }
        else
        {
            gpio_set( PLC_LED_STG_PORT, PLC_LED_STG_PIN );
        }
        //if(  *(uint8_t *)BKPSRAM_BASE == 1 )
        if (plc_diag_status  & PLC_DIAG_ERR_HSE)
        {
            if( hse_post_flag )
            {
                hse_post_flag = false;
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hse_err_msg, sizeof(plc_hse_err_msg));
            }
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        }
        //if( *( (uint8_t *)BKPSRAM_BASE + 1) == 1 )
        if (plc_diag_status  & PLC_DIAG_ERR_LSE)
        {
            if( lse_post_flag )
            {
                lse_post_flag = false;
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_lse_err_msg, sizeof(plc_lse_err_msg));
            }
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        }

        if (plc_diag_status  & PLC_DIAG_ERR_DEADLINE)
        {
            if( dl_post_flag )
            {
                dl_post_flag = false;
                plc_app_stop();// Must stop the app now!
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dl_err_msg, sizeof(plc_dl_err_msg));
            }
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        }
    }

    if( PLC_TIMER(blink_tmr) > blink_thr )
    {
        PLC_CLEAR_TIMER(blink_tmr);
        gpio_clear( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        gpio_clear( PLC_LED_STG_PORT, PLC_LED_STG_PIN );
    }
}
*/
#include "dbnc_flt.h"
static dbnc_flt_t in_flt[PLC_DI_NUM];

typedef struct
{
    enum rcc_periph_clken periph;
    uint32_t port;
    uint16_t pin;
} dio_t;

#define PLC_DIO_CONCAT(a,b) a##b
#define PLC_DIO_CONCAT2(a,b) PLC_DIO_CONCAT(a,b)

#define PLC_DIO_THING(t,n,name) (PLC_DIO_CONCAT2(PLC_DIO_CONCAT(PLC_,t),PLC_DIO_CONCAT(n,name)))

#define PLC_DIO_PERIPH(t,n) PLC_DIO_THING(t,n,_PERIPH)
#define PLC_DIO_PORT(t,n)   PLC_DIO_THING(t,n,_PORT)
#define PLC_DIO_PIN(t,n)    PLC_DIO_THING(t,n,_PIN)

#define PLC_DIO_REC(t,n) {PLC_DIO_PERIPH(t,n),PLC_DIO_PORT(t,n),PLC_DIO_PIN(t,n)}

static const dio_t din[PLC_DI_NUM] =
{
    PLC_DIO_REC(I,0),
    PLC_DIO_REC(I,1),
    PLC_DIO_REC(I,2),
    PLC_DIO_REC(I,3),
    PLC_DIO_REC(I,4),
    PLC_DIO_REC(I,5),
    PLC_DIO_REC(I,6),
    PLC_DIO_REC(I,7),
};

static const dio_t dout[PLC_DO_NUM] =
{
    PLC_DIO_REC(O,0),
    PLC_DIO_REC(O,1),
    PLC_DIO_REC(O,2),
    PLC_DIO_REC(O,3)
};

bool plc_get_din(uint32_t i)
{
    if (PLC_DI_NUM<=i)
    {
        return false;
    }
    else
    {
        return dbnc_flt_get(in_flt+i);
    }
}

bool plc_get_dout(uint32_t i)
{
    if (PLC_DO_NUM<=i)
    {
        return false;
    }
    else
    {
        return (0 != gpio_get(dout[i].port, dout[i].pin));
    }
}

void plc_set_dout( uint32_t i, bool val )
{
    void (*do_set)(uint32_t, uint16_t);

    if (PLC_DO_NUM<=i)
    {
        return;
    }
    do_set = (val)?gpio_set:gpio_clear;

    do_set(dout[i].port, dout[i].pin);
}

uint32_t plc_get_ain(uint32_t i)
{
    (void)i;
    return 0;
}

void plc_set_aout( uint32_t i, uint32_t val )
{
    (void)i;
    (void)val;
}


//Digital io
#define LOCAL_PROTO plc_dio
void PLC_IOM_LOCAL_INIT(void)
{
    ///Outputs
    uint32_t i;
    for (i=0; i<PLC_DO_NUM; i++)
    {
        rcc_periph_clock_enable(dout[i].periph );
        gpio_set_mode          (dout[i].port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, dout[i].pin);
        gpio_clear             (dout[i].port,                                                    dout[i].pin);
    }
    ///Inputs
    for (i=0; i<PLC_DI_NUM; i++)
    {
        dbnc_flt_init(in_flt+i);

        rcc_periph_clock_enable(din[i].periph);
        gpio_set_mode          (din[i].port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, din[i].pin);
        gpio_set               (din[i].port,                                              din[i].pin);
    }
}

bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

const char plc_dio_err_asz[]     = "Digital I/O address must be one number!";
const char plc_dio_err_asz_flt[] = "Digital input filter address must consist of two numbers!";

const char plc_dio_err_tp[]      = "Digital I/O does not support memory locations!";
const char plc_dio_err_tp_flt[]  = "Digital input filter must have memory type!";

const char plc_dio_err_ilim[]    = "Digital input must have address 0...7!";
const char plc_dio_err_flt_lim[] = "Digital input filter must have 0 or 1 address!";
const char plc_dio_err_olim[]    = "Digital output must have address 0...3!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;

    addr = PLC_APP->l_tab[i]->a_data[0];

    switch( PLC_APP->l_tab[i]->v_size )
    {
    case PLC_LSZ_X:
        if (1 != PLC_APP->l_tab[i]->a_size)
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_asz, sizeof(plc_dio_err_asz));
            return false;
        }
        //Check type and address
        switch (PLC_APP->l_tab[i]->v_type)
        {
        case PLC_LT_I:
            if (PLC_DI_NUM <= addr)
            {
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_ilim, sizeof(plc_dio_err_ilim));
                return false;
            }
            else
            {
                return true;
            }

        case PLC_LT_M:
        default:
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_iom_err_sz, plc_iom_err_sz_sz);
            return false;

        case PLC_LT_Q:
            if (PLC_DO_NUM <= addr)
            {
                plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_olim, sizeof(plc_dio_err_olim));
                return false;
            }
            else
            {
                return true;
            }
        }
        return true;

    case PLC_LSZ_B:
        if (2 != PLC_APP->l_tab[i]->a_size)
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_asz_flt, sizeof(plc_dio_err_asz_flt));
            return false;
        }

        if (PLC_LT_M != PLC_APP->l_tab[i]->v_type)
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_tp_flt, sizeof(plc_dio_err_tp_flt));
            return false;
        }

        if (PLC_DI_NUM <= addr)
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_ilim, sizeof(plc_dio_err_ilim));
            return false;
        }

        addr = PLC_APP->l_tab[i]->a_data[1];
        if( addr > 1 )
        {
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_dio_err_flt_lim, sizeof(plc_dio_err_flt_lim));
            return false;
        }
        return true;

    default:
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_iom_err_sz, plc_iom_err_sz_sz);
        return false;
    }
}

void PLC_IOM_LOCAL_BEGIN(uint16_t lid)
{
}
void PLC_IOM_LOCAL_END(uint16_t lid)
{
}
uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void PLC_IOM_LOCAL_START(void)
{
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    uint32_t i;
    for (i = 0; i < PLC_DI_NUM; i++)
    {
        dbnc_flt_poll(in_flt+i, tick, !gpio_get(din[i].port, din[i].pin));
    }
}

void PLC_IOM_LOCAL_STOP(void)
{
    uint32_t i;
    for (i=0; i<PLC_DO_NUM; i++)
    {
        gpio_clear(dout[i].port, dout[i].pin);
    }
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    switch( plc_curr_app->l_tab[i]->v_type )
    {
    case PLC_LT_I:
        *(bool *)(plc_curr_app->l_tab[i]->v_buf) = dbnc_flt_get( in_flt + plc_curr_app->l_tab[i]->a_data[0]);
        break;

    case PLC_LT_M://Filter configuration is write only
//        if (plc_curr_app->l_tab[i]->a_data[1])
//        {
//            *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf) = (uint8_t)in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_on;
//        }
//        else
//        {
//            *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf) = (uint8_t)in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_off;
//        }
        break;

    default:
        break;
    }
    return 0;
}
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch( plc_curr_app->l_tab[i]->v_type )
    {
    case PLC_LT_Q:
        plc_set_dout( plc_curr_app->l_tab[i]->a_data[0], *(bool *)(plc_curr_app->l_tab[i]->v_buf) );
        break;

    case PLC_LT_M:
        if (plc_curr_app->l_tab[i]->a_data[1])
        {
            in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_on = (uint32_t)*(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        }
        else
        {
            in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_off = (uint32_t)*(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        }
        break;

    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
