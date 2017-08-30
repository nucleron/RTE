/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>

#include <iec_types_all.h>

#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_clock.h>
#include <plc_dbg.h>
#include <plc_diag.h>
#include <plc_iom.h>
#include <plc_gpio.h>
#include <plc_hw.h>
#include <plc_rtc.h>
#include <plc_wait_tmr.h>

volatile uint32_t plc_diag_status = 0;

static const plc_gpio_t led_hb[] =
{
    PLC_GPIO_REC(LED_STG),
    PLC_GPIO_REC(LED_STR)
};
//Led blink timer
static uint32_t blink_thr;
static uint32_t blink_tmr;
static bool err_test_flg = true;
//RTC check
static IEC_TIME rtc_last_val;
static uint32_t rtc_chck_tmr;

typedef struct
{
    uint8_t  level;
    uint32_t msk;
    uint32_t thr;
    char*    msg;
    uint32_t msz;
} diag_cfg_t;

#define PLC_DIAG_LEVEL_CRI 0 //Critical error stop everything
#define PLC_DIAG_LEVEL_WRN 1 //Warning
#define PLC_DIAG_LEVEL_INF 2 //Info
#define PLC_DIAG_LEVEL_NRM 3 //Normal work
static uint8_t err_level = PLC_DIAG_LEVEL_NRM;


//static const char plc_dl_err_msg[] =  "Deadline violation detected, PLC is stoped!";
static const char plc_crt_err_msg[] = "Critical failure!";
static const char plc_hse_err_msg[] = "HSE oscilator failed!";
static const char plc_lse_err_msg[] = "LSE oscilator failed!";
static const char plc_app_wrn_msg[] = "Aplication posted warning";
static const char plc_app_inf_msg[] = "Aplication posted info";

#define DIAG_POLL_REC(level, msk, thr, msg) {level, msk, thr, (char *)(msg), sizeof(msg)}

static const diag_cfg_t diag_poll_cfg[]=
{
    //Can't indicate deadline as PLC gets reset in case of deadline violation.
    //DIAG_POLL_REC(PLC_DIAG_LEVEL_CRI, PLC_DIAG_ERR_DEADLINE  , 250, plc_dl_err_msg),
    DIAG_POLL_REC(PLC_DIAG_LEVEL_CRI, PLC_DIAG_ERR_OTHER_CRIT, 250, plc_crt_err_msg),
    DIAG_POLL_REC(PLC_DIAG_LEVEL_WRN, PLC_DIAG_ERR_LSE       , 250, plc_lse_err_msg),
    DIAG_POLL_REC(PLC_DIAG_LEVEL_WRN, PLC_DIAG_ERR_HSE       , 125, plc_hse_err_msg),
    DIAG_POLL_REC(PLC_DIAG_LEVEL_WRN, PLC_DIAG_ERR_APP_WARN  , 500, plc_app_wrn_msg),
    DIAG_POLL_REC(PLC_DIAG_LEVEL_INF, PLC_DIAG_ERR_APP_INFO  , 250, plc_app_inf_msg),
};

static bool diag_post_flg[sizeof(diag_poll_cfg)/sizeof(diag_cfg_t)];
static void clr_post_flg(void)
{
    uint8_t i;
    for(i=0; i<sizeof(diag_post_flg); i++)
    {
        diag_post_flg[i] = true;
    }
}
#define LOCAL_PROTO plc_diag

static bool plc_has_wdt = false;

static bool plc_wcct_pivot_present = false;
volatile uint64_t plc_diag_wcct = 0; //Worst case cycle time

void PLC_IOM_LOCAL_INIT(void)
{
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    SCB_SHCSR |= SCB_SHCSR_USGFAULTENA|SCB_SHCSR_BUSFAULTENA|SCB_SHCSR_MEMFAULTENA;
#endif
    plc_has_wdt = dwt_enable_cycle_counter();
    clr_post_flg();
    blink_thr = 500;
    PLC_GPIO_GR_CFG_OUT(led_hb);
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

#define PLC_DIAG_I_HSE 0
#define PLC_DIAG_I_LSE 1

#define PLC_DIAG_I_NUM 2

#define PLC_DIAG_Q_DBGM 0  //
#define PLC_DIAG_Q_CRIT 1
#define PLC_DIAG_Q_WARN 2
#define PLC_DIAG_Q_INFO 3

#define PLC_DIAG_Q_NUM 4

#define PLC_DIAG_ADDR_NUM 2

static const char plc_diag_err_asz[]     = "Diag location adress must be one number!";
static const char plc_diag_err_tp[]      = "Diag protocol supports only input locations!";
static const char plc_diag_err_addr_i[]  = "Diag I location adress must be in 0 or 1!";
static const char plc_diag_err_addr_q[]  = "Diag Q location adress must be in 0..3!";
static const char plc_diag_err_addr_m[]  = "WCCT location adress must be 0!";

//static bool abort_present = false;

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    if (PLC_LT_M == PLC_APP->l_tab[i]->v_type)
    {
        //WCCT location
        if (PLC_LSZ_D != PLC_APP->l_tab[i]->v_size)
        {
            PLC_LOG_ERR_SZ();
            return false;
        }

        if (1 != PLC_APP->l_tab[i]->a_size)
        {
            PLC_LOG_ERROR(plc_diag_err_asz);
            return false;
        }

        if (1 <= PLC_APP->l_tab[i]->a_data[0])
        {
            PLC_LOG_ERROR(plc_diag_err_addr_m);
            return false;
        }
    }
    else
    {
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
            if (PLC_DIAG_Q_NUM <= PLC_APP->l_tab[i]->a_data[0])
            {
                PLC_LOG_ERROR(plc_diag_err_addr_q);
                return false;
            }
        }
        else
        {
            if (PLC_DIAG_I_NUM <= PLC_APP->l_tab[i]->a_data[0])
            {
                PLC_LOG_ERROR(plc_diag_err_addr_i);
                return false;
            }
        }
    }

    return true;
}

void PLC_IOM_LOCAL_BEGIN(uint16_t lid)
{
    (void)lid;
}
void PLC_IOM_LOCAL_END(uint16_t lid)
{
    (void)lid;
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    (void)lid;
    (void)tick;
    return 0;
}

void PLC_IOM_LOCAL_START(void)
{
    PLC_CLEAR_TIMER(rtc_chck_tmr);
    plc_rtc_time_get(&rtc_last_val);

    PLC_CLEAR_TIMER(blink_tmr);
    clr_post_flg();
    plc_diag_status &= ~(PLC_DIAG_ERR_APP_INFO|PLC_DIAG_ERR_APP_WARN|PLC_DIAG_ERR_APP_CRIT); //Clear software errors
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    //WCCT measurement.
    (void)tick;
    if (plc_has_wdt)
    {
        static uint32_t pivot = 0;
        uint32_t point;
        uint32_t cycle_time;

        point =  dwt_read_cycle_counter();
        cycle_time = (point - pivot)/PLC_RCC_AHB_FREQ; //CT in us

        if (plc_wcct_pivot_present && (plc_diag_wcct < cycle_time))
        {
            plc_diag_wcct = cycle_time;
        }

        pivot = point;
        plc_wcct_pivot_present = true;
    }
    //Check if RTC failed
    //RTC time must change every 3 secconds
    if (0 != plc_rtc_failure)
    {
        plc_diag_status |= PLC_DIAG_ERR_LSE;
    }

    if (0!= plc_clock_hse_failure)
    {
        plc_diag_status |= PLC_DIAG_ERR_HSE;
    }

    //Blink status led
    if (PLC_TIMER(blink_tmr) > (blink_thr>>1))
    {
        err_test_flg = true;
        switch (err_level)
        {
        case PLC_DIAG_LEVEL_NRM:
        case PLC_DIAG_LEVEL_INF:
            gpio_set(PLC_LED_STG_PORT, PLC_LED_STG_PIN);
            break;
        case PLC_DIAG_LEVEL_CRI:
            gpio_set(PLC_LED_STR_PORT, PLC_LED_STR_PIN);
            break;
        default:
            gpio_set(PLC_LED_STG_PORT, PLC_LED_STG_PIN);
            gpio_set(PLC_LED_STR_PORT, PLC_LED_STR_PIN);
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
            err_level = PLC_DIAG_LEVEL_NRM;
            for (i=0; i< sizeof(diag_post_flg); i++)
            {
                if ((plc_diag_status & diag_poll_cfg[i].msk) && (diag_poll_cfg[i].level <= err_level))
                {
                    err_level = diag_poll_cfg[i].level;
                    blink_thr = diag_poll_cfg[i].thr;
                    if (diag_post_flg[i])
                    {
                        uint8_t log_level;
                        switch (err_level)
                        {
                        case PLC_DIAG_LEVEL_CRI:
                            log_level = LOG_CRITICAL;
                            break;
                        case PLC_DIAG_LEVEL_WRN:
                            log_level = LOG_WARNING;
                            break;
                        case PLC_DIAG_LEVEL_INF:
                            log_level = LOG_INFO;
                            break;
                        default:
                            log_level = LOG_DEBUG;
                            break;
                        }
                        diag_post_flg[i] = false;
                        plc_curr_app->log_msg_post(log_level, (char *)(diag_poll_cfg[i].msg), diag_poll_cfg[i].msz);
                    }
                }
                else
                {
                    diag_post_flg[i] = true;
                }
            }
        }
        PLC_CLEAR_TIMER(blink_tmr);
        gpio_clear(PLC_LED_STR_PORT, PLC_LED_STR_PIN);
        gpio_clear(PLC_LED_STG_PORT, PLC_LED_STG_PIN);
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
    if (PLC_LT_M == PLC_APP->l_tab[i]->v_type)
    {
        *(IEC_DWORD *)(plc_curr_app->l_tab[i]->v_buf) = plc_diag_wcct;
    }
    else
    {
        bool tmp;

        switch (plc_curr_app->l_tab[i]->a_data[0])
        {
        case PLC_DIAG_I_HSE:
            tmp = (0 != (plc_diag_status & PLC_DIAG_ERR_HSE));
            break;

        case PLC_DIAG_I_LSE:
            tmp = (0 != (plc_diag_status & PLC_DIAG_ERR_LSE));
            break;

        default:
            tmp = false;
        }

        *(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf) = tmp;
    }
    return 0;
}

static const char plc_diag_abort_msg[] = "PLC execution is aborted by program!";

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    if (PLC_LT_M == PLC_APP->l_tab[i]->v_type)
    {
        //WCCT location
        return 0;
    }

    switch (PLC_APP->l_tab[i]->a_data[0])
    {
    case PLC_DIAG_Q_DBGM://Enter debug mode
        if (*(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf))
        {
            plc_dbg_mode = true;
        }
        break;
    case PLC_DIAG_Q_CRIT://Abort
        if (*(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf))
        {
            PLC_LOG_ERROR(plc_diag_abort_msg);
            plc_diag_status |= PLC_DIAG_ERR_APP_CRIT;
            plc_app_stop();
        }
        break;
    case PLC_DIAG_Q_WARN://Warning
        if (*(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf))
        {
            //No message, user must do it in PLC program.
            plc_diag_status |= PLC_DIAG_ERR_APP_WARN;
        }
        else
        {
            //No message, user must do it in PLC program.
            plc_diag_status &= ~PLC_DIAG_ERR_APP_WARN;
        }
        break;
    case PLC_DIAG_Q_INFO://Warning
        if (*(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf))
        {
            //No message, user must do it in PLC program.
            plc_diag_status |= PLC_DIAG_ERR_APP_INFO;
        }
        else
        {
            //No message, user must do it in PLC program.
            plc_diag_status &= ~PLC_DIAG_ERR_APP_INFO;
        }

        break;
    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO

