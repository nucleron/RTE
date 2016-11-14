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

const plc_io_metods_t plc_iom_registry[] =
{
    PLC_IOM_RECORD(plc_diag),
    PLC_IOM_RECORD(plc_dio),
    PLC_IOM_RECORD(plc_mb),
    PLC_IOM_RECORD(plc_hmi)
};
//Must be declared after plc_iom_registry
PLC_IOM_REG_SZ_DECL;

uint8_t mid_from_pid( uint16_t proto )
{
    switch(proto)
    {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 4: //lost 3 somewhere?
        return 3;
    default:
        return PLC_IOM_MID_ERROR;
    }
    return PLC_IOM_MID_ERROR;
}
