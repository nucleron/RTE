/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdbool.h>

#include <plc_abi.h>

#include <plc_config.h>

#include <plc_rtc.h>
#include <plc_tick.h>
#include <plc_backup.h>
#include <plc_hw.h>
#include <plc_iom.h>

__attribute__ ((section(".plc_rte_sec"))) plc_rte_abi_t plc_glue_rte =
{
    .get_time  = plc_rtc_time_get,
    .set_timer = plc_tick_setup,

    .check_retain_buf      = plc_backup_check,
    .invalidate_retain_buf = plc_backup_invalidate,
    .validate_retain_buf   = plc_backup_validate,

    .retain = plc_backup_retain,
    .remind = plc_backup_remind,

    .set_dout = plc_set_dout,
    .get_din  = plc_get_din,

    .set_aout = plc_set_aout,
    .get_ain  = plc_get_ain
    //Other unused rigth now
};

extern uint32_t _stack;
extern uint32_t _app_start;
extern uint32_t _app_end;

static bool plc_check_code(uint32_t * addr)
{
    if (addr < &_app_start)
    {
        return false;
    }
    if (addr > &_app_end)
    {
        return false;
    }
    return true;
}

#define PLC_CHECK_CODE(addr) \
    do{ \
        if (!plc_check_code((uint32_t *)addr)) \
        { \
            return false; \
        } \
    }while (0)

bool plc_app_is_valid(void)
{
    uint8_t i;
    bool is_correct = false;

    //Check stack
    ///TODO: Add stack section to app!
    if (PLC_APP->sstart != &_stack)
    {
        return false;
    }
    //Check that header was written
    if (PLC_APP->log_cnt_reset == (void (*)(void))0xffffffff)
    {
        return false;
    }
    //Check that app was written
    for (i=0; i<32; i++)
    {
        uint8_t tmp;
        tmp = PLC_APP->check_id[i];
        if (PLC_APP->id[i] != tmp)
        {
            return false;
        }
        //Did we finish app write?
        if (tmp != 0xff)
        {
            is_correct = true;
        }
    }

    //Check bss section
    if (PLC_APP->bss_end >= &_stack)
    {
        return false;
    }
    //Check data section
    if (PLC_APP->data_end > PLC_APP->bss_end)
    {
        return false;
    }
    if (PLC_APP->data_start > PLC_APP->data_end)
    {
        return false;
    }
    //Check pa section
    PLC_CHECK_CODE(PLC_APP->pa_start);
    PLC_CHECK_CODE(PLC_APP->pa_end);
    if (PLC_APP->pa_end < PLC_APP->pa_start)
    {
        return false;
    }
    //Check ia section
    PLC_CHECK_CODE(PLC_APP->ia_start);
    PLC_CHECK_CODE(PLC_APP->ia_end);
    if (PLC_APP->ia_end < PLC_APP->ia_start)
    {
        return false;
    }
    //Check fia section
    PLC_CHECK_CODE(PLC_APP->fia_start);
    PLC_CHECK_CODE(PLC_APP->fia_end);
    if (PLC_APP->fia_end < PLC_APP->fia_start)
    {
        return false;
    }
    //HW ID
    if (PLC_APP->hw_id != PLC_HW_ID)
    {
        return false;
    }
    //Check RTE version compatibility
    if (PLC_APP->rte_ver_major != PLC_RTE_VER_MAJOR)
    {
        return false;
    }
    if (PLC_APP->rte_ver_minor > PLC_RTE_VER_MINOR)
    {
        return false;
    }
//    if (PLC_APP->rte_ver_patch > PLC_RTE_VER_PATCH)
//    {
//        return false;
//    }
    //Check PLC application ABI
    PLC_CHECK_CODE(PLC_APP->start);
    PLC_CHECK_CODE(PLC_APP->stop);
    PLC_CHECK_CODE(PLC_APP->run);

    PLC_CHECK_CODE(PLC_APP->dbg_resume);
    PLC_CHECK_CODE(PLC_APP->dbg_suspend);

    PLC_CHECK_CODE(PLC_APP->dbg_data_get);
    PLC_CHECK_CODE(PLC_APP->dbg_data_free);

    PLC_CHECK_CODE(PLC_APP->dbg_vars_reset);
    PLC_CHECK_CODE(PLC_APP->dbg_var_register);

    PLC_CHECK_CODE(PLC_APP->log_cnt_get);
    PLC_CHECK_CODE(PLC_APP->log_msg_get);
    PLC_CHECK_CODE(PLC_APP->log_cnt_reset);
    //Check plc IO manager interface
    if (PLC_APP->l_tab < (plc_loc_tbl_t *)PLC_APP->data_start)
    {
        return false;
    }
    if (PLC_APP->l_tab > (plc_loc_tbl_t *)PLC_APP->bss_end)
    {
        return false;
    }

    if (PLC_APP->w_tab < PLC_APP->data_start)
    {
        return false;
    }
    if (PLC_APP->w_tab > PLC_APP->bss_end)
    {
        return false;
    }
    ///TODO: Add with CRC check
    return is_correct;
}

void plc_app_cstratup(void)
{
    volatile uint32_t *src, *dst, *end;
    app_fp_t *func, *func_end;
    //Init .data
    dst = PLC_APP->data_start;
    end = PLC_APP->data_end;
    src = PLC_APP->data_loadaddr;
    while (dst < end)
    {
        *dst++ = *src++;
    }
    //Init .bss
    end = PLC_APP->bss_end;
    while (dst < end)
    {
        *dst++ = 0;
    }
    // Constructors
    // .preinit_array
    func = PLC_APP->pa_start;
    func_end = PLC_APP->pa_end;
    while (func < func_end)
    {
        (*func)();
        func++;
    }
    // .init_array
    func = PLC_APP->ia_start;
    func_end = PLC_APP->ia_end;
    while (func < func_end)
    {
        (*func)();
        func++;
    }
}
