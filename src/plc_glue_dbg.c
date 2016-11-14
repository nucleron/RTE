/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include "plc_glue_dbg.h"

#ifndef PLC_MD5
#define PLC_MD5 "375254725509a3f2b9f0867e11074526Unknown#Unnamed#Unnamed#"
#endif

const char plc_md5[] = PLC_MD5;

extern int startPLC(int argc,char **argv);
extern int stopPLC();
extern void runPLC(void);

extern void resumeDebug(void);
extern void suspendDebug(int disable);

extern void FreeDebugData(void);
extern int GetDebugData(unsigned long *tick, unsigned long *size, void **buffer);

extern void ResetDebugVariables(void);
extern void RegisterDebugVariable(int idx, void* force);

extern void ResetLogCount(void);
extern uint32_t GetLogCount(uint8_t level);
extern uint32_t GetLogMessage(uint8_t level, uint32_t msgidx, char* buf, uint32_t max_size, uint32_t* tick, uint32_t* tv_sec, uint32_t* tv_nsec);

const plc_app_abi_t plc_glue_app =
{
    .id   = plc_md5,

    .start = startPLC,
    .stop  = stopPLC,
    .run   = runPLC,

    .dbg_resume    = resumeDebug,
    .dbg_suspend   = suspendDebug,

    .dbg_data_get  = GetDebugData,
    .dbg_data_free = FreeDebugData,

    .dbg_vars_reset   = ResetDebugVariables,
    .dbg_var_register = RegisterDebugVariable,

    .log_cnt_get   = GetLogCount,
    .log_msg_get   = GetLogMessage,
    .log_cnt_reset = ResetLogCount,
    .log_msg_post  = LogMessage
};

#include <plc_rtc.h>
#include <plc_tick.h>
#include <plc_backup.h>

const plc_rte_abi_t plc_glue_rte =
{
    .get_time  = plc_rtc_time_get,
    .set_timer = plc_tick_setup,

    .check_retain_buf      = plc_backup_check,
    .invalidate_retain_buf = plc_backup_invalidate,
    .validate_retain_buf   = plc_backup_validate,

    .retain = plc_backup_retain,
    .remind = plc_backup_remind
    //Other unused rigth now
};


bool plc_app_is_valid(void)
{
    return true;
}
void plc_app_cstratup(void)
{
    return;
}
