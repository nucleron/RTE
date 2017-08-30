/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
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

typedef struct
{
    enum rcc_periph_clken periph;
    uint32_t port;
    uint16_t pin;
} dio_t;

#define PLC_HMI_CONCAT(a, b) a##b
#define PLC_HMI_CONCAT2(a, b) PLC_HMI_CONCAT(a, b)

#define PLC_HMI_THING(t, n, name) (PLC_HMI_CONCAT2(PLC_HMI_CONCAT(PLC_HMI_, t), PLC_HMI_CONCAT(n, name)))

#define PLC_HMI_PERIPH(t, n) PLC_HMI_THING(t, n, _PERIPH)
#define PLC_HMI_PORT(t, n)   PLC_HMI_THING(t, n, _PORT)
#define PLC_HMI_PIN(t, n)    PLC_HMI_THING(t, n, _PIN)

#define PLC_HMI_REC(t, n) {PLC_HMI_PERIPH(t, n), PLC_HMI_PORT(t, n), PLC_HMI_PIN(t, n)}

static const dio_t hmi[PLC_HMI_DO_NUM] =
{
    PLC_HMI_REC(O, 0),
    PLC_HMI_REC(O, 1),
};

void plc_hmi_set_dout(uint32_t i, bool val)
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
    ///Outputs
    uint32_t i;
    for (i=0; i<PLC_HMI_DO_NUM; i++)
    {
        rcc_periph_clock_enable(hmi[i].periph);
        gpio_set_mode          (hmi[i].port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, hmi[i].pin);
        gpio_clear             (hmi[i].port,                                                    hmi[i].pin);
    }
}

bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}

const char plc_hmi_err_dir[]      = "Only output locations are supported for HMI";
const char plc_hmi_err_type[]      = "Only BOOL type locations are supported for HMI";
const char plc_hmi_err_olim[]    = "HMI output must have address 0...1!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;

    addr = PLC_APP->l_tab[i]->a_data[0];

    if (PLC_APP->l_tab[i]->v_size == PLC_LSZ_X)
    {
        if (PLC_APP->l_tab[i]->v_type == PLC_LT_Q)
        {
            if (PLC_HMI_DO_NUM <= addr)
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
            plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_dir, sizeof(plc_hmi_err_dir));
            return false;
        }
    }
    else
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_hmi_err_type, sizeof(plc_hmi_err_type));
        return false;
    }

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
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    (void)tick;
}

void PLC_IOM_LOCAL_STOP(void)
{
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}
uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    (void)i;
    return 0;
}
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_Q:
        plc_hmi_set_dout(plc_curr_app->l_tab[i]->a_data[0], *(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf));
        break;

    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
