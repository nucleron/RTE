/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <plc_config.h>
#include <plc_gpio.h>
#include <plc_abi.h>
#include <plc_dbg.h>
#include <plc_rtc.h>
#include <plc_hw.h>
#include <plc_iom.h>
#include <plc_wait_tmr.h>


static const plc_gpio_t led_hb[] =
{
    PLC_GPIO_REC(LED_STG),
    PLC_GPIO_REC(LED_STR)
};
//Led blink timer
static uint32_t blink_thr;
static uint32_t blink_tmr;
static bool err_test_flg = true;
static uint8_t err_prio = 10;

typedef struct
{
    uint8_t  prio;
    uint32_t msk;
    uint32_t thr;
    char*    msg;
    uint32_t msz;
} diag_cfg_t;

const char plc_crt_err_msg[] = "Critical failure!";
const char plc_hse_err_msg[] = "HSE oscilator failed!";
const char plc_lse_err_msg[] = "LSE oscilator failed!";
const char plc_dl_err_msg[] =  "Deadline violation detected, PLC is stoped!";

#define DIAG_POLL_REC(prio, msk, thr, msg) {prio, msk, thr, (char *)(msg), sizeof(msg)}

static const diag_cfg_t diag_poll_cfg[]=
{
    DIAG_POLL_REC(1,PLC_HW_ERR_HSE     ,500, plc_hse_err_msg),
    DIAG_POLL_REC(2,PLC_HW_ERR_LSE     ,500, plc_lse_err_msg),
    DIAG_POLL_REC(0,PLC_HW_ERR_DEADLINE,250, plc_dl_err_msg ),
    DIAG_POLL_REC(0,PLC_HW_ERR_CRITICAL,250, plc_crt_err_msg)
};

static bool diag_post_flg[sizeof(diag_poll_cfg)/sizeof(diag_cfg_t)];

#define LOCAL_PROTO plc_diag
void PLC_IOM_LOCAL_INIT(void)
{
    uint8_t i;
    for(i=0; i<sizeof(diag_post_flg); i++)
    {
        diag_post_flg[i] = true;
    }
    blink_thr = 500;
    PLC_GPIO_GR_CFG_OUT(led_hb);
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

#define PLC_DIAG_ADDR_HSE 0
#define PLC_DIAG_ADDR_LSE 1

#define PLC_DIAG_ADDR_NUM 2

static const char plc_diag_err_asz[]     = "Diag location adress must be one number!";
static const char plc_diag_err_tp[]      = "Diag protocol supports only input locations!";
static const char plc_diag_err_addr_i[]    = "Diag location adress must be in 0 or 1!";
static const char plc_diag_err_addr_q[]    = "Diag location adress must be in 0 or 1!";

static bool abort_present = false;

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;
    if (PLC_LSZ_X != PLC_APP->l_tab[i]->v_size)
    {
        PLC_LOG_ERR_SZ();
        return false;
    }

    if (1 != PLC_APP->l_tab[i]->a_size)
    {
        PLC_LOG_ERROR(plc_diag_err_asz);
        return false;
    }

    if (PLC_LT_Q == PLC_APP->l_tab[i]->v_type)
    {
        //Debug mode and Abort locations
        if(1 < PLC_APP->l_tab[i]->a_data[0])
        {
            PLC_LOG_ERROR(plc_diag_err_addr_q);
            return false;
        }
    }
    else
    {
        if (PLC_DIAG_ADDR_NUM <= PLC_APP->l_tab[i]->a_data[0])
        {
            PLC_LOG_ERROR(plc_diag_err_addr_i);
            return false;
        }
    }

    return true;
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
    PLC_CLEAR_TIMER(blink_tmr);
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    (void)tick;
    if (PLC_TIMER(blink_tmr) > (blink_thr>>1))
    {
        err_test_flg = true;
        switch(err_prio)
        {
        case 10:
            gpio_set( PLC_LED_STG_PORT, PLC_LED_STG_PIN );
            break;
        case 0:
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
            break;
        default:
            gpio_set( PLC_LED_STG_PORT, PLC_LED_STG_PIN );
            gpio_set( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
            break;
        }
    }

    if (PLC_TIMER(blink_tmr) > blink_thr)
    {
        if (err_test_flg)
        {
            uint8_t i;
            err_test_flg = false;

            blink_thr = 1000;
            err_prio = 10;
            for (i=0; i< sizeof(diag_post_flg); i++)
            {
                if ((plc_hw_status & diag_poll_cfg[i].msk) && (diag_poll_cfg[i].prio < err_prio))
                {
                    err_prio = diag_poll_cfg[i].prio;
                    blink_thr = diag_poll_cfg[i].thr;
                    if (diag_post_flg[i])
                    {
                        diag_post_flg[i] = false;
                        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)(diag_poll_cfg[i].msg), diag_poll_cfg[i].msz);
                    }

                }
            }
        }
        PLC_CLEAR_TIMER(blink_tmr);
        gpio_clear( PLC_LED_STR_PORT, PLC_LED_STR_PIN );
        gpio_clear( PLC_LED_STG_PORT, PLC_LED_STG_PIN );
    }
}

void PLC_IOM_LOCAL_STOP(void)
{
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    return PLC_APP->l_tab[i]->a_data[0];
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    bool tmp;

    switch(plc_curr_app->l_tab[i]->a_data[0])
    {
    case PLC_DIAG_ADDR_HSE:
        tmp = (0 != plc_hw_status  & PLC_HW_ERR_HSE);
        break;

    case PLC_DIAG_ADDR_LSE:
        tmp = (0 != plc_hw_status  & PLC_HW_ERR_LSE);
        break;

    default:
        tmp = false;
    }

    *(bool *)(plc_curr_app->l_tab[i]->v_buf) = tmp;

    return 0;
}

static const char plc_diag_abort_msg[] = "PLC execution is aborted by program!";

extern bool plc_dbg_mode;

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch (PLC_APP->l_tab[i]->a_data[0])
    {
    case 0://Enter debug mode
        if(*(bool *)(plc_curr_app->l_tab[i]->v_buf))
        {
            plc_dbg_mode = true;
        }
        break;
    case 1://Abort
        if(*(bool *)(plc_curr_app->l_tab[i]->v_buf))
        {
            PLC_LOG_ERROR(plc_diag_abort_msg);
            plc_hw_status |= PLC_HW_ERR_USER;
            plc_app_stop();
        }
        break;
    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
