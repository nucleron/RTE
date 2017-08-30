/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portevent.c, v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

#include <mb.h>

#define evt_in_queue inst->evt_in_queue
#define queued_evt inst->queued_evt

/* ----------------------- Start implementation -----------------------------*/
BOOL
mb_port_ser_evt_init(mb_port_ser_struct* inst)
{
    evt_in_queue = FALSE;
    return TRUE;
}

BOOL
mb_port_ser_evt_post( mb_port_ser_struct* inst, mb_event_enum event)
{
    evt_in_queue = TRUE;
    queued_evt = event;
    return TRUE;
}

BOOL
mb_port_ser_evt_get(mb_port_ser_struct* inst, void* caller, mb_event_enum * event)
{
    BOOL            xEventHappened = FALSE;

    (void)caller;

    if (evt_in_queue)
    {
        *event = queued_evt;
        evt_in_queue = FALSE;
        xEventHappened = TRUE;
    }
    return xEventHappened;
}


/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */
mb_err_enum eMBRTUMasterWaitRequestFinish(void) {

	/*
	mb_err_enum    eErrStatus = MB_ENOERR;
   // rt_uint32_t recvedEvent;
     //waiting for OS event
    rt_event_recv(&xMasterOsEvent,
            EV_MASTER_PROCESS_SUCESS | EV_MASTER_ERROR_RESPOND_TIMEOUT
                    | EV_MASTER_ERROR_RECEIVE_DATA
                    | EV_MASTER_ERROR_EXECUTE_FUNCTION,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER,
            &recvedEvent);
    switch (recvedEvent)
    {
    case EV_MASTER_PROCESS_SUCESS:
        break;
    case EV_MASTER_ERROR_RESPOND_TIMEOUT:
    {
        eErrStatus = MB_ETIMEDOUT;
        break;
    }
    case EV_MASTER_ERROR_RECEIVE_DATA:
    {
        eErrStatus = MB_ERCV;
        break;
    }
    case EV_MASTER_ERROR_EXECUTE_FUNCTION:
    {
        eErrStatus = MB_MRE_EXE_FUN;
        break;
    }
    }

    */
    return 0;//eErrStatus;
}
