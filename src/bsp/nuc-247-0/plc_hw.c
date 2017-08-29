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
#include <plc_gpio.h>
#include <plc_abi.h>
#include <plc_iom.h>
#include <plc_dbg.h>
#include <plc_wait_tmr.h>
#include <plc_hw.h>

uint16_t digital_out=0;

void plc_jmpr_init(void)
{
    rcc_periph_clock_enable(PLC_JMP_RST_PERIPH);
    gpio_mode_setup(PLC_JMP_RST_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN , PLC_JMP_RST_PIN);
    gpio_clear(PLC_JMP_RST_PORT, PLC_JMP_RST_PIN); //Pull down!

    rcc_periph_clock_enable(PLC_JMP_DBG_PERIPH);
    gpio_mode_setup(PLC_JMP_DBG_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP , PLC_JMP_DBG_PIN);
    gpio_set(PLC_JMP_DBG_PORT, PLC_JMP_DBG_PIN);
}

bool plc_dbg_jmpr_get(void)
{
    return !gpio_get(PLC_JMP_DBG_PORT, PLC_JMP_DBG_PIN);
}

volatile bool reset_jmp = false;

bool plc_rst_jmpr_get(void)
{
    return gpio_get(PLC_JMP_RST_PORT, PLC_JMP_RST_PIN);
}

void plc_boot_init(void)
{
    rcc_periph_clock_enable(PLC_BOOT_PERIPH);
    gpio_mode_setup(PLC_BOOT_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PLC_BOOT_PIN);
    gpio_set_output_options(PLC_BOOT_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, PLC_BOOT_PIN);
    gpio_set(PLC_BOOT_PORT, PLC_BOOT_PIN); //Exit boot mode
}

void plc_boot_mode_enter(void)
{
    uint32_t delay;
    //Set boot pin
    gpio_clear(PLC_BOOT_PORT, PLC_BOOT_PIN);
    //Wait boot pin voltage to set
    PLC_CLEAR_TIMER(delay);
    while (PLC_TIMER(delay) < 2000);
    //May reset the system now
    scb_reset_system();
}

#include "dbnc_flt.h"
dbnc_flt_t in_flt[PLC_DI_NUM];

static const plc_gpio_t din[PLC_DI_NUM] =
{
    PLC_GPIO_REC(I0),
    PLC_GPIO_REC(I1),
    PLC_GPIO_REC(I2),
    PLC_GPIO_REC(I3),
    PLC_GPIO_REC(I4),
    PLC_GPIO_REC(I5),
    PLC_GPIO_REC(I6),
    PLC_GPIO_REC(I7),
    PLC_GPIO_REC(I8),
    PLC_GPIO_REC(I9),
    PLC_GPIO_REC(I10),
    PLC_GPIO_REC(I11),
};

static const plc_gpio_t dout[PLC_DO_NUM] =
{
    PLC_GPIO_REC(O0),
    PLC_GPIO_REC(O1),
    PLC_GPIO_REC(O2),
    PLC_GPIO_REC(O3),
    PLC_GPIO_REC(O4),
    PLC_GPIO_REC(O5),
    PLC_GPIO_REC(O6),
    PLC_GPIO_REC(O7)
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

void plc_set_dout(uint32_t i, bool val)
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

void plc_set_aout(uint32_t i, uint32_t val)
{
    (void)i;
    (void)val;
}


//Digital io
#define LOCAL_PROTO plc_dio
void PLC_IOM_LOCAL_INIT(void)
{
    uint32_t i;
    ///Outputs
    PLC_GPIO_GR_CFG_OUT(dout);
    ///Inputs
    for (i=0; i<PLC_DI_NUM; i++)
    {
        dbnc_flt_init(in_flt+i);
        plc_gpio_cfg_in(din+i);
    }
}

bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
/*
const char plc_dio_err_asz[]     = "Digital I/O address must be one number!";
const char plc_dio_err_asz_flt[] = "Digital input filter address must consist of two numbers!";

const char plc_dio_err_tp[]      = "Digital I/O does not support memory locations!";
const char plc_dio_err_tp_flt[]  = "Digital input filter must have memory type!";

const char plc_dio_err_ilim[]    = "Digital input must have address 0...7!";
const char plc_dio_err_flt_lim[] = "Digital input filter must have 0 or 1 address!";
const char plc_dio_err_olim[]    = "Digital output must have address 0...3!";
*/

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;

    addr = PLC_APP->l_tab[i]->a_data[0];

    switch (PLC_APP->l_tab[i]->v_size)
    {
    case PLC_LSZ_X:
        if (1 != PLC_APP->l_tab[i]->a_size)
        {
            //PLC_LOG_ERROR(plc_dio_err_asz);
            plc_iom_errno_print(PLC_ERRNO_DIO_ASZ);
            return false;
        }
        //Check type and address
        switch (PLC_APP->l_tab[i]->v_type)
        {
        case PLC_LT_I:
            if (PLC_DI_NUM <= addr)
            {
                //PLC_LOG_ERROR(plc_dio_err_ilim);
                plc_iom_errno_print(PLC_ERRNO_DIO_ILIM);
                return false;
            }
            else
            {
                return true;
            }

        case PLC_LT_M:
        default:
            PLC_LOG_ERR_SZ();
            return false;

        case PLC_LT_Q:
            if (PLC_DO_NUM <= addr)
            {
                //PLC_LOG_ERROR(plc_dio_err_olim);
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
            //PLC_LOG_ERROR(plc_dio_err_asz_flt);
            plc_iom_errno_print(PLC_ERRNO_DIO_ASZ_FLT);
            return false;
        }

        if (PLC_LT_M != PLC_APP->l_tab[i]->v_type)
        {
            //PLC_LOG_ERROR(plc_dio_err_tp_flt);
            plc_iom_errno_print(PLC_ERRNO_DIO_TP_FLT);
            return false;
        }

        if (PLC_DI_NUM <= addr)
        {
            //PLC_LOG_ERROR(plc_dio_err_ilim);
            plc_iom_errno_print(PLC_ERRNO_DIO_ILIM);
            return false;
        }

        addr = PLC_APP->l_tab[i]->a_data[1];
        if (addr > 1)
        {
            //PLC_LOG_ERROR(plc_dio_err_flt_lim);
            plc_iom_errno_print(PLC_ERRNO_DIO_FLT_LIM);
            return false;
        }
        return true;

    default:
        PLC_LOG_ERR_SZ();
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
    uint32_t i;
    for (i=0; i<PLC_DO_NUM; i++)
    {
        gpio_clear(dout[i].port, dout[i].pin);
    }
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}
void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    uint32_t i;
    for (i = 0; i < PLC_DI_NUM; i++)
    {
        dbnc_flt_poll(in_flt+i, tick, !gpio_get(din[i].port, din[i].pin));
    }
}
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}
uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_I:
        *(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf) = dbnc_flt_get(in_flt + plc_curr_app->l_tab[i]->a_data[0]);
        break;

    case PLC_LT_M://Filter configuration is write only
//        if (plc_curr_app->l_tab[i]->a_data[1])
//        {
//            *(IEC_USINT *)(plc_curr_app->l_tab[i]->v_buf) = (uint8_t)in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_on;
//        }
//        else
//        {
//            *(IEC_USINT *)(plc_curr_app->l_tab[i]->v_buf) = (uint8_t)in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_off;
//        }
        break;

    default:
        break;
    }
    return 0;
}
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_Q:
        plc_set_dout(plc_curr_app->l_tab[i]->a_data[0], *(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf));
        if (*(IEC_BOOL *)(plc_curr_app->l_tab[i]->v_buf))
        {
            digital_out|=(1<<(plc_curr_app->l_tab[i]->a_data[0]));
        }
        else
        {
            digital_out&=~(1<<(plc_curr_app->l_tab[i]->a_data[0]));
        }
        break;

    case PLC_LT_M:
        if (plc_curr_app->l_tab[i]->a_data[1])
        {
            in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_on = (uint32_t)*(IEC_USINT *)(plc_curr_app->l_tab[i]->v_buf);
        }
        else
        {
            in_flt[plc_curr_app->l_tab[i]->a_data[0]].thr_off = (uint32_t)*(IEC_USINT *)(plc_curr_app->l_tab[i]->v_buf);
        }
        break;

    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
