/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_wait_tmr.h>
#include <plc_hw.h>
#include <plc_iom.h>

PLC_IOM_METH_DECLS(plc_dio);
PLC_IOM_METH_DECLS(plc_mb);
PLC_IOM_METH_DECLS(plc_diag);
PLC_IOM_METH_DECLS(plc_hmi);
PLC_IOM_METH_DECLS(plc_ain);
PLC_IOM_METH_DECLS(plc_aout);

const plc_io_metods_t plc_iom_registry[] =
{
    PLC_IOM_RECORD(plc_diag),
    PLC_IOM_RECORD(plc_dio),
    PLC_IOM_RECORD(plc_mb),
    PLC_IOM_RECORD(plc_hmi),
    PLC_IOM_RECORD(plc_aout),
    PLC_IOM_RECORD(plc_ain)
};
//Must be declared after plc_iom_registry
PLC_IOM_REG_SZ_DECL;

uint8_t mid_from_pid(uint16_t proto)
{
    switch (proto)
    {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 4:
        return 3;
    case 5:
        return 4;
    case 6:
        return 5;
    default:
        return PLC_IOM_MID_ERROR;
    }
    return PLC_IOM_MID_ERROR;
}
