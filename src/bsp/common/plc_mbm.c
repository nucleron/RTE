/*
 * FreeModbus Libary: user callback functions and buffer define in master mode
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: user_mbm_app_m.c, v 1.60 2013/11/23 11:49:05 Armink $
 */
#include <stdbool.h>
#include <stdint.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_diag.h>
#include <plc_rtc.h>
#include <plc_hw.h>
#include <plc_iom.h>

#include <mb.h>

#include "user_mb_app.h"

#define MBM_REQUEST_MAX_ADDR 250
#define INVALID_UID 0xFFFF

enum req_type_enum
{
    RT_NONE,
    RT_DISCRETE,
    RT_INPUTREG,
    RT_HOLDING_RD,
    RT_HOLDING_WR,
    RT_COILS,
};

enum req_result_enum
{
    RR_SUCCESS=0,
    RR_TIMEOUT,
    RR_DATA_ERR,
    RR_INT_ERR,
    RR_LOADING,
    RR_PENDING,
    RR_NEXTSTEP,
    RR_FINISHED,
};

typedef enum req_type_enum req_type_t;
typedef enum req_result_enum req_result_t;

struct request_struct
{
    uint16_t uid;
    uint32_t period;
    req_type_t type;
    req_result_t result;
    uint8_t slave_addr;
    uint16_t target_addr[MBM_REQUEST_MAX_ADDR];
    uint16_t target_value[MBM_REQUEST_MAX_ADDR];
    uint8_t registers;
};

typedef struct request_struct request_t;

static request_t mbm_request;

//static uint8_t mbm_preread_buffer[(MBM_REQUEST_MAX_ADDR>>3)+1];
static uint16_t mbm_preread_buffer[MBM_REQUEST_MAX_ADDR];


static uint32_t mbm_nearest_time=0xFFFFFFFF;
static uint16_t mbm_nearest_num;

static uint8_t mbm_busy = 0xFF;
static bool mbm_need_preread = false;
static bool mbm_preread_finished = false;

static mb_instance MBMaster;
static mb_trans_union MBMTransport;

static bool mbm_init_flg = false;
static bool mbm_start = false;

static bool mbm_gotinit = false;
static bool mbm_enabled = false;
static bool mbm_ascii = false;
uint32_t mbm_baudrate = 9600;


#define LOCAL_PROTO plc_mbm
void PLC_IOM_LOCAL_INIT(void)
{
    int i;

    mbm_request.type=RT_NONE;
    mbm_request.registers=0;
    mbm_request.result=RR_FINISHED;
    mbm_request.slave_addr=0;
    mbm_request.uid=INVALID_UID;
    for (i=0; i<MBM_REQUEST_MAX_ADDR; i++)
    {
        mbm_request.target_addr[i]=0;
        mbm_request.target_value[i]=0;
    }

}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
/*
static const char plc_mbm_err_asz[]     = "Modbus local register adress must be one number!";
static const char plc_mbm_err_tp[]      = "Modbus local supports only memory locations for registers!";
static const char plc_mbm_err_addr[]    = "Modbus local register adress must be in 0...31!";
static const char plc_mbm_err_init[]    = "Modbus wrong init value format. Must be QX2.[baudrate].[mode]";
static const char plc_mbm_err_tp_init[] = "Modbus local supports only output location for initialization!";

static const char plc_mbm_err_reglimit[]  = "Registers number limit reached for one modbus master request_t";
static const char plc_mbm_err_badtype[]  = "Modbus register type does not match request_t type";
static const char plc_mbm_err_requid[]  = "Modbus request_t UID out of range";
static const char plc_mbm_err_extrainit[] = "Only one modbus init variable allowed!";
*/

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
//    int j;
//    uint32_t addr;
//    uint32_t temp;
//    req_type_t rtype;

    switch (PLC_APP->l_tab[i]->a_size)
    {
    case 2:
        if (PLC_APP->l_tab[i]->v_type == PLC_LT_Q) //init location
        {
            if (PLC_APP->l_tab[i]->v_size != PLC_LSZ_X)
            {
                //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mbm_err_tp_init, sizeof(plc_mbm_err_tp_init));
                plc_iom_errno_print(PLC_ERRNO_MBM_TP_INIT);
                return false;
            }
            if (mbm_gotinit)
            {
                //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mbm_err_extrainit, sizeof(plc_mbm_err_extrainit));
                plc_iom_errno_print(PLC_ERRNO_MBM_EX_INIT);
                return false;
            }

            mbm_gotinit = true;
            mbm_baudrate = (int)PLC_APP->l_tab[i]->a_data[0];
            mbm_ascii = ((int)PLC_APP->l_tab[i]->a_data[1]!=0);
            return true;
        }
        else if (PLC_APP->l_tab[i]->v_type == PLC_LT_M)//requested registers
        {
            if ((PLC_APP->l_tab[i]->v_size != PLC_LSZ_X) && (PLC_APP->l_tab[i]->v_size != PLC_LSZ_W)) //bool size only for coils and discrete inputs
            {
                //word size for holding & input regs
                //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mbm_err_badtype, sizeof(plc_mbm_err_badtype));
                plc_iom_errno_print(PLC_ERRNO_MBM_SZ);
                return false;
            }

        }
        else
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mbm_err_tp, sizeof(plc_mbm_err_tp));
            plc_iom_errno_print(PLC_ERRNO_MBM_TP);
            return false;
        }
        break;

    case 4: //Request location
        if (PLC_APP->l_tab[i]->v_type == PLC_LT_I)
        {
            if (PLC_APP->l_tab[i]->v_size != PLC_LSZ_B)
            {
                //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mbm_err_badtype, sizeof(plc_mbm_err_badtype));
                plc_iom_errno_print(PLC_ERRNO_MBM_TP);
                return false;
            }
        }
        break;
    default:
        plc_iom_errno_print(PLC_ERRNO_MBM_ASZ);
        return false;
        break;
    }
    return true;
}

void PLC_IOM_LOCAL_START(uint16_t i)
{
}

void PLC_IOM_LOCAL_STOP(void)
{
}

void PLC_IOM_LOCAL_BEGIN(uint16_t i)
{
}

bool is_continious()
{
    if ((mbm_request.target_addr[mbm_request.registers-1]-mbm_request.target_addr[0])==(mbm_request.registers-1))
    {
        return true;
    }
    return false;
}

uint8_t get_full_length()
{
    return mbm_request.target_addr[mbm_request.registers-1] - mbm_request.target_addr[0]+1;
}


//Do once!
void PLC_IOM_LOCAL_END(uint16_t i)
{
//    int j, k, l;
    if (!mbm_init_flg)
    {
        mbm_init_flg = true;
        //If no init location specified then start with default params.
        if (!mbm_gotinit)
        {
            mbm_ascii = MB_DEFAULT_TRANSPORT;
            mbm_baudrate = MB_DEFAULT_BAUDRATE;
            mbm_enabled = true;
            mbm_start = true;
        }
        //eMBMasterInitRTU(&MBMaster, &MBMTransport, (mb_port_base *)&mbm_inst_usart, mbm_baudrate, MB_PAR_NONE);
        eMBInit(&MBMaster, &MBMTransport, (mbm_ascii)?MB_ASCII:MB_RTU, TRUE, 0, (mb_port_base *)&mbm_inst_usart, mbm_baudrate, MB_PAR_NONE);

    }
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void execute_request(uint32_t tick)
{
    int i;
    int j=0;
    uint8_t reg_index;

    mbm_busy=0x00;
    mbm_need_preread     = false;
    PLC_APP->w_tab[mbm_nearest_num] = tick;

    switch (mbm_request.type)
    {
    case RT_HOLDING_RD:
        eMBMasterReqReadHoldingRegister(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), 0);
        break;
    case RT_HOLDING_WR:
        if (is_continious())
            eMBMasterReqWriteMultipleHoldingRegister(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], mbm_request.registers, mbm_request.target_value, 0);
        else
        {
            if (mbm_preread_finished) //after we got current register values, change what we have to write and send a request_t to send everything to slave
            {
                mbm_preread_finished = false;
                for (j=0; j<get_full_length(); j++)
                {
                    reg_index = mbm_request.target_addr[j]-mbm_request.target_addr[0];
                    mbm_preread_buffer[reg_index] = mbm_request.target_value[j];
                }
                //eMBMasterReqWriteHoldingRegister
                eMBMasterReqWriteMultipleHoldingRegister(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), mbm_preread_buffer, 0);
            }
            else //we need to send a request_t to first read full registers range
            {
                eMBMasterReqReadHoldingRegister(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), 0);
                mbm_need_preread = true;
            }
        }
        break;
    case RT_DISCRETE:
        eMBMasterReqReadDiscreteInputs(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), 0);
        break;
    case RT_INPUTREG:
        eMBMasterReqReadInputRegister(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), 0);
        break;
    case RT_COILS:
        //pack bits
        for (i=0; i<get_full_length(); i++)
        {
            xMBUtilSetBits((UCHAR *)mbm_preread_buffer, i, 1, (mbm_request.target_value[i]==0)?0:1);
        }
        //send request_t
        eMBMasterReqWriteMultipleCoils(&MBMaster, mbm_request.slave_addr, mbm_request.target_addr[0], get_full_length(), (UCHAR *)mbm_preread_buffer, 0);
        break;
    default:
        break;
    }
}

void read_request_info(uint16_t i)
{
    mbm_request.uid = PLC_APP->l_tab[i]->a_data[0];
    mbm_request.slave_addr = PLC_APP->l_tab[i]->a_data[2];
    mbm_request.period = PLC_APP->l_tab[i]->a_data[3];
    mbm_request.result = RR_LOADING;
    mbm_request.registers=0;
    switch (PLC_APP->l_tab[i]->a_data[1])
    {
    case 0:
        mbm_request.type = RT_INPUTREG;
        break;
    case 2:
        mbm_request.type = RT_HOLDING_WR;
        break;
    case 3:
        mbm_request.type = RT_COILS;
        break;
    case 4:
        mbm_request.type = RT_DISCRETE;
        break;

    case 1:
    default:
        mbm_request.type = RT_HOLDING_RD;
        break;
    }
}

bool request_completed(void)
{
    return ((mbm_request.result!=RR_LOADING) && (mbm_request.result!= RR_PENDING));
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
//    int i, j;
//    uint8_t max_weight_num;
//    uint8_t max_weight=0;

    if (!mbm_init_flg) return;

    //Do once!
    if (mbm_start)
    {
        mbm_start = false;
        if (!mbm_enabled)
        {
            eMBDisable(&MBMaster);
            return;
        }
        eMBEnable(&MBMaster);
    }

    if (eMBPoll(&MBMaster)==MB_EILLSTATE)
    {
        return;
    }

    if (mbm_preread_finished)
    {
        execute_request(tick);
        return;
    }

    if (mbm_request.result==RR_NEXTSTEP)
    {
        mbm_request.result = RR_PENDING;
    }

    if (mbm_request.result==RR_LOADING)
    {
        if ((mbm_request.type==RT_COILS) || (mbm_request.type==RT_HOLDING_WR)) //write request_t, set values to write first
        {
            mbm_request.result=RR_NEXTSTEP;
        }
        else
        {
            mbm_request.result=RR_PENDING;
        }
    }


    if (mbm_busy!=0xFF)
    {
        return;
    }

    if (mbm_request.result==RR_PENDING)
    {
        if ((PLC_APP->w_tab[mbm_nearest_num]+mbm_request.period)<tick)
        {
            execute_request(tick);
        }
    }

    //check if it is time to execute request_t


}
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    return PLC_APP->l_tab[i]->a_data[0];
}

uint8_t req_find_reg(uint16_t reg_addr)
{
    int i;
    for (i=0; i<mbm_request.registers; i++)
    {
        if (mbm_request.target_addr[i]==reg_addr)
        {
            return i;
        }
        if (mbm_request.target_addr[i]>reg_addr)
        {
            return 0xFF;
        }

    }
    return 0xFF;
}



uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    uint32_t last_called;
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_I:
        if (mbm_request.result==RR_FINISHED)
        {
            last_called = (PLC_APP->l_tab[i]->a_data[3]+PLC_APP->w_tab[i]);
            if (last_called<mbm_nearest_time)
            {
                mbm_nearest_time = last_called;
                mbm_nearest_num = i;
            }
        }
        else if (mbm_request.uid==plc_curr_app->l_tab[i]->a_data[0])
        {
            if (request_completed())
            {
                *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf) = mbm_request.result;
            }

        }
        break;
    case PLC_LT_M:
        mbm_nearest_time=0xFFFFFFFF;
        if (mbm_request.result==RR_FINISHED)
        {
            if (mbm_request.uid!=plc_curr_app->l_tab[mbm_nearest_num]->a_data[0])
            {
                read_request_info(mbm_nearest_num);
            }
        }

        if (mbm_request.uid==plc_curr_app->l_tab[i]->a_data[0])
        {

            if (mbm_request.result==RR_LOADING)
            {
                mbm_request.target_addr[mbm_request.registers] = PLC_APP->l_tab[i]->a_data[1];
                mbm_request.registers++;
            }
            else if (request_completed())
            {
                switch (PLC_APP->l_tab[i]->v_size)
                {
                case PLC_LSZ_W:
                    *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf) = mbm_request.target_value[req_find_reg(plc_curr_app->l_tab[i]->a_data[1])];
                    break;
                case PLC_LSZ_X:
                    *(bool *)(plc_curr_app->l_tab[i]->v_buf) = mbm_request.target_value[req_find_reg(plc_curr_app->l_tab[i]->a_data[1])];
                    break;
                }

            }

        }

        break;
    case PLC_LT_Q://Write only access to outputs
    default:
        break;
    }
    return 0;
}

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    if (request_completed())
    {
        mbm_request.result = RR_FINISHED;
    }

    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_M:
        if (mbm_request.uid==plc_curr_app->l_tab[i]->a_data[0])
        {
            switch (PLC_APP->l_tab[i]->v_size)
            {
            case PLC_LSZ_W:
                mbm_request.target_value[req_find_reg(plc_curr_app->l_tab[i]->a_data[1])] = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
                break;
            case PLC_LSZ_X:
                mbm_request.target_value[req_find_reg(plc_curr_app->l_tab[i]->a_data[1])] = *(bool*)(plc_curr_app->l_tab[i]->v_buf);
                break;
            }
        }
        break;
    case PLC_LT_Q: //for init location.
        if (mbm_gotinit)
        {
            mbm_enabled = *(bool *)(PLC_APP->l_tab[i]->v_buf);
            mbm_start = true; //Now we can start
            mbm_gotinit = false; //Do not check locationy any more
        }
        break;

    case PLC_LT_I:
    default:
        break;
    }
    return 0;
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBRespondTimeout(mb_instance* inst, UCHAR ucDestAddress, const UCHAR* pucPDUData, USHORT ucPDULength)
{
    mbm_request.result=RR_TIMEOUT;
    mbm_busy=0xFF;
}


/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBReceiveData(mb_instance* inst, UCHAR ucDestAddress, const UCHAR* pucPDUData, USHORT ucPDULength)
{
    mbm_request.result=RR_DATA_ERR;
    mbm_busy=0xFF;
}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBExecuteFunction(mb_instance* inst, UCHAR ucDestAddress, const UCHAR* pucPDUData, USHORT ucPDULength)
{
    mbm_request.result=RR_INT_ERR;
    mbm_busy=0xFF;
}

/**
 * This is modbus master request_t process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterCBRequestSuccess(mb_instance* inst)
{
    if (mbm_need_preread)
    {
        mbm_need_preread     = false;
        mbm_preread_finished = true;
    }
    else
    {
        mbm_request.result=RR_SUCCESS;
        mbm_busy=0xFF;
    }

}

/**
 * Modbus master input register callback function.
 *
 * @param pucRegBuffer input register buffer
 * @param usAddress input register address
 * @param usNRegs input register number
 *
 * @return result
 */
eMBErrorCode eMBMasterRegInputCB(mb_instance* inst, UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    uint8_t reg_index;

    /* it already plus one in modbus function method. */
    usAddress--;

    while (usNRegs > 0)
    {
        reg_index = req_find_reg(usAddress);

        mbm_request.target_value[reg_index] =  (uint16_t)(*pucRegBuffer++)<<8;
        mbm_request.target_value[reg_index]|=   *pucRegBuffer++;

        usAddress++;
        usNRegs--;
    }

    return eStatus;
}

/**
 * Modbus master holding register callback function.
 *
 * @param pucRegBuffer holding register buffer
 * @param usAddress holding register address
 * @param usNRegs holding register number
 * @param eMode read or write
 *
 * @return result
 */



eMBErrorCode eMBMasterRegHoldingCB(mb_instance* inst, UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    uint8_t reg_index=0;

    usAddress--;

    while (usNRegs > 0)
    {
        if (mbm_need_preread)
        {
            mbm_preread_buffer[reg_index] = (uint16_t)(*pucRegBuffer++)<<8;
            mbm_preread_buffer[reg_index]|=   *pucRegBuffer++;
            reg_index++;
        }
        else
        {
            reg_index = req_find_reg(usAddress);
            if (reg_index!=0xFF)
            {
                mbm_request.target_value[reg_index] =  (uint16_t)(*pucRegBuffer++)<<8;
                mbm_request.target_value[reg_index]|=   *pucRegBuffer++;
            }
            else
            {
                pucRegBuffer++;
                pucRegBuffer++;
                //we got extra register. just ignore it for now
            }
        }
        usAddress++;
        usNRegs--;
    }

//    switch (eMode)
//    {
//    /* write values to slave registers*/
//    case MB_REG_WRITE:
//        break;
//        while (usNRegs > 0)
//        {
//            reg_index = req_find_reg(usAddress);
//            if (reg_index!=0xFF)
//            {
//                *pucRegBuffer++ = (UCHAR) (mbm_request.target_value[reg_index] >> 8);
//                *pucRegBuffer++ = (UCHAR) (mbm_request.target_value[reg_index] & 0xFF);
//            }
//            else //means we are trying to write registers we do not have
//            {
//                *pucRegBuffer++=0xDE; //should never end up here
//                *pucRegBuffer++=0xAD; //
//            }
//            usAddress++;
//            usNRegs--;
//        }
//        break;
//    /*Get values from slave registers */
//    case MB_REG_READ:
//        while (usNRegs > 0)
//        {
//            if (mbm_need_preread)
//            {
//                mbm_preread_buffer[reg_index] = (uint16_t)(*pucRegBuffer++)<<8;
//                mbm_preread_buffer[reg_index]|=   *pucRegBuffer++;
//                reg_index++;
//            }
//            else
//            {
//                reg_index = req_find_reg(usAddress);
//                if (reg_index!=0xFF)
//                {
//                    mbm_request.target_value[reg_index] =  (uint16_t)(*pucRegBuffer++)<<8;
//                    mbm_request.target_value[reg_index]|=   *pucRegBuffer++;
//                }
//                else
//                {
//                    pucRegBuffer++;
//                    pucRegBuffer++;
//                    //we got extra register. just ignore it for now
//                }
//            }
//            usAddress++;
//            usNRegs--;
//        }
//        break;
//    }

    return eStatus;
}

/**
 * Modbus master coils callback function.
 *
 * @param pucRegBuffer coils buffer
 * @param usAddress coils address
 * @param usNCoils coils number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBMasterRegCoilsCB(mb_instance* inst, UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    return eStatus;
}

/**
 * Modbus master discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode eMBMasterRegDiscreteCB(mb_instance* inst, UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    uint8_t iNReg =  0;
    uint8_t reg_index;

    /* it already plus one in modbus function method. */
    usAddress--;

    /* write current discrete values with new values from the protocol stack. */
    while (usNDiscrete > 0)
    {
        reg_index = req_find_reg(usAddress+iNReg);
        if (reg_index!=0xFF)
        {
            mbm_request.target_value[reg_index] = (*(pucRegBuffer+(iNReg/8))) &(1<<(iNReg%8));
        }

        if ((iNReg%8)==7)
        {
            pucRegBuffer++;
        }
        usNDiscrete--;
        iNReg++;
    }

    return eStatus;
}

