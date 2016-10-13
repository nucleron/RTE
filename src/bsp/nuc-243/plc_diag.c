/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_rtc.h>
#include <plc_hw.h>
#include <plc_iom.h>

#define LOCAL_PROTO plc_diag
void PLC_IOM_LOCAL_INIT(void)
{
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
static const char plc_diag_err_addr[]    = "Diag location adress must be in 0 or 1!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    uint32_t addr;
    if (PLC_LSZ_X != PLC_APP->l_tab[i]->v_size)
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_iom_err_sz, plc_iom_err_sz_sz );
        return false;
    }

    if( PLC_LT_I != PLC_APP->l_tab[i]->v_type )
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_diag_err_tp, sizeof(plc_diag_err_tp));
        return false;
    }

    if (1 != PLC_APP->l_tab[i]->a_size)
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_diag_err_asz, sizeof(plc_diag_err_asz));
        return false;
    }

    if (PLC_DIAG_ADDR_NUM <= PLC_APP->l_tab[i]->a_data[0])
    {
        plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_diag_err_addr, sizeof(plc_diag_err_addr));
        return false;
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
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
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

uint32_t PLC_IOM_LOCAL_SET(uint16_t lid)
{
    return 0;
}
#undef LOCAL_PROTO
