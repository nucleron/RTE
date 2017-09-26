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

typedef enum
{
    /*Working states*/
    PLC_MBM_ST_RQ_SCHED,
    PLC_MBM_ST_RQ_READ,
    PLC_MBM_ST_RQ_FETCH, /*Fetch register contents before write*/
    PLC_MBM_ST_RQ_WRITE,
    /*Limit of working states for border checking*/
    PLC_MBM_ST_RQ_LIM,
    /*Location checking states*/
    PLC_MBM_ST_INIT,    /*Initial state*/
    PLC_MBM_ST_ERR,     /*Error state*/
    /*Location check states*/
    PLC_MBM_ST_CHK_RQ,  /*Request IB3.[id].[type].[slv_address].[reg_address].[period_ms]*/
    PLC_MBM_ST_CHK_MEM, /*Memory MX3.[id].[reg_id] or MW3.[id].[reg_id]*/
    PLC_MBM_ST_CHK_CFG  /*Configuration QX3.[baud].[mode]*/
} plc_mbm_st_enum;

typedef enum
{
    /*Read*/
    PLC_MBM_RQ_RD_IX, /*Discrete inputs*/
    PLC_MBM_RQ_RD_MX, /*Coils*/
    PLC_MBM_RQ_RD_IW, /*Input regs*/
    PLC_MBM_RQ_RD_MW, /*Holding regs*/
    /*Write*/
    PLC_MBM_RQ_WR_MX, /*Coils*/
    PLC_MBM_RQ_WR_MW, /*Holding regs*/
    /*Limit value for border checking*/
    PLC_MBM_RQ_LIM
} plc_mbm_rq_enum;

typedef enum
{
    PLC_MBM_RST_OK = 0,  /*Request success*/
    PLC_MBM_RST_NODATA,  /*No data for request (initial state)*/
    PLC_MBM_RST_ERR_TO,  /*Request timed out*/
    PLC_MBM_RST_ERR_FN,  /*MB function exception*/
    PLC_MBM_RST_ERR_RCV, /*MB receive exception*/
    PLC_MBM_RST_ERR_DL,  /*MB request deadline not met*/
    PLC_MBM_RST_ERR_FAIL,/*MB stack failed*/
    /*Limit value for border checking*/
    PLC_MBM_RST_LIM
} plc_mbm_rst_enum;

/*MB master state*/
typedef struct
{
    uint8_t state;
} plc_mbm_struct;

/*Read  request function pointer*/
typedef mb_err_enum (*plc_mbm_rd_rq_fp)(mb_inst_struct *inst, UCHAR snd_addr, USHORT reg_addr, USHORT reg_num);
/*Write request function pointer*/
typedef mb_err_enum (*plc_mbm_wr_rq_fp)(mb_inst_struct *inst, UCHAR snd_addr, USHORT reg_addr, USHORT reg_num, USHORT *data_ptr);
/*Alias of both*/
typedef union
{
    plc_mbm_rd_rq_fp rd; /*read*/
    plc_mbm_wr_rq_fp wr; /*write*/
} plc_mbm_rq_fp;

/**TODO: Data load function pointer*/
/**TODO: Data store function pointer*/

/*Transintion function pointer*/
typedef void (*plc_mbm_trans_fp)(void);
/*Transintion function table pointer*/
typedef plc_mbm_trans_fp * plc_mbm_trans_tbl;

plc_mbm_struct plc_mbm =
{
    .state  = PLC_MBM_ST_INIT
};

static void plc_mbm_tf_error(uint8_t err)
{
    (void)err;
    /**TODO:Написать тут общий обработчик перехода
    в состояние "SCHED" c ошибкой*/
}

/*Request function table*/
const plc_mbm_rq_fp plc_mbm_rq_tbl[PLC_MBM_RQ_LIM] =
{
    /*Read*/
    [PLC_MBM_RQ_RD_IX] = {.rd = mb_mstr_rq_read_discrete_inputs   },
    [PLC_MBM_RQ_RD_MX] = {.rd = mb_mstr_rq_read_coils             },
    [PLC_MBM_RQ_RD_IW] = {.rd = mb_mstr_rq_read_inp_reg           },
    [PLC_MBM_RQ_RD_MW] = {.rd = mb_mstr_rq_read_holding_reg       },
    /*Write*/
    [PLC_MBM_RQ_RD_MX] = {.wr = mb_mstr_rq_write_multi_coils      },
    [PLC_MBM_RQ_RD_MW] = {.wr = mb_mstr_rq_write_multi_holding_reg}
};
/*Fetch request function selector*/
/**TODO: Придумать, как лучше написать...*/
static mb_err_enum _default_fetch(mb_inst_struct *inst, UCHAR snd_addr, USHORT reg_addr, USHORT reg_num)
{
    plc_mbm_tf_error(PLC_MBM_RST_ERR_FAIL);
}
/**А что если выполнять реквесты прямо тут?*/
/**Или переместить этот код прямо в обработчик перехода SCHED->FETCH?*/
static inline plc_mbm_rd_rq_fp _plc_mbm_get_fetch_fp(uint8_t rq_type)
{
    switch(rq_type)
    {
    case PLC_MBM_RQ_RD_MX:
        return mb_mstr_rq_read_coils;
    case PLC_MBM_RQ_RD_MW:
        return mb_mstr_rq_read_holding_reg;
    default:
        return (plc_mbm_rd_rq_fp)0;
    }
}

/*Default transition handler*/
static void plc_mbm_tf_default(uint8_t err)
{
    plc_mbm_tf_error(PLC_MBM_RST_ERR_FAIL);
}

static void plc_mbm_tf_rd_sched(void)
{
    /**TODO:Написать обработчик прерхода SCHED->READ*/
}

static void plc_mbm_tf_rd_read(void)
{
    /**TODO:Написать обработчик прерхода READ->SCHED*/
}

const plc_mbm_trans_fp plc_mbm_rd_tbl[PLC_MBM_ST_RQ_LIM] =
{
    [PLC_MBM_ST_RQ_SCHED] = plc_mbm_tf_rd_sched,
    [PLC_MBM_ST_RQ_READ]  = plc_mbm_tf_rd_read,
    [PLC_MBM_ST_RQ_FETCH] = plc_mbm_tf_default,
    [PLC_MBM_ST_RQ_WRITE] = plc_mbm_tf_default
}

static void plc_mbm_tf_wr_sched(void)
{
    /**TODO:Написать обработчик прерхода SCHED->[FETCH,WRITE]*/
}

static void plc_mbm_tf_wr_fetch(void)
{
    /**TODO:Написать обработчик прерхода FETCH->WRITE*/
}

static void plc_mbm_tf_wr_write(void)
{
    /**TODO:Написать обработчик прерхода WRITE->SHCED*/
}

const plc_mbm_trans_fp plc_mbm_wr_tbl[PLC_MBM_ST_RQ_LIM] =
{
    [PLC_MBM_ST_RQ_SCHED] = plc_mbm_tf_wr_sched,
    [PLC_MBM_ST_RQ_READ]  = plc_mbm_tf_default,
    [PLC_MBM_ST_RQ_FETCH] = plc_mbm_tf_wr_fetch,
    [PLC_MBM_ST_RQ_WRITE] = plc_mbm_tf_wr_write
};

const plc_mbm_trans_tbl plc_mbm_tr_tbl[PLC_MBM_RQ_LIM] =
{
    /*Read*/
    [PLC_MBM_RQ_RD_IX] = (plc_mbm_trans_tbl)plc_mbm_rd_tbl,
    [PLC_MBM_RQ_RD_MX] = (plc_mbm_trans_tbl)plc_mbm_rd_tbl,
    [PLC_MBM_RQ_RD_IW] = (plc_mbm_trans_tbl)plc_mbm_rd_tbl,
    [PLC_MBM_RQ_RD_MW] = (plc_mbm_trans_tbl)plc_mbm_rd_tbl,
    /*Write*/
    [PLC_MBM_RQ_RD_MX] = (plc_mbm_trans_tbl)plc_mbm_wr_tbl,
    [PLC_MBM_RQ_RD_MW] = (plc_mbm_trans_tbl)plc_mbm_wr_tbl
};

#define LOCAL_PROTO plc_mbm
void PLC_IOM_LOCAL_INIT(void)
{
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    return true;
}

void PLC_IOM_LOCAL_START(uint16_t i)
{
    (void)i;
}

void PLC_IOM_LOCAL_STOP(void)
{
}

void PLC_IOM_LOCAL_BEGIN(uint16_t i)
{
    (void)i;
}
//Do once!
void PLC_IOM_LOCAL_END(uint16_t i)
{
//    int j, k, l;
    (void)i;
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    (void)lid;
    (void)tick;
    return 0;
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{

}

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param dst_addr destination salve address
 * @param pdu_data_ptr PDU buffer data
 * @param pdu_len PDU buffer length
 *
 */
void mb_mstr_error_timeout_cb(mb_inst_struct *inst, UCHAR dst_addr, const UCHAR* pdu_data_ptr, USHORT pdu_len)
{
    (void)inst;
    (void)dst_addr;
    (void)pdu_data_ptr;
    (void)pdu_len;
}


/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param dst_addr destination salve address
 * @param pdu_data_ptr PDU buffer data
 * @param pdu_len PDU buffer length
 *
 */
void mb_mstr_error_rcv_data_cb(mb_inst_struct *inst, UCHAR dst_addr, const UCHAR* pdu_data_ptr, USHORT pdu_len)
{
    (void)inst;
    (void)dst_addr;
    (void)pdu_data_ptr;
    (void)pdu_len;
}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 * @param dst_addr destination salve address
 * @param pdu_data_ptr PDU buffer data
 * @param pdu_len PDU buffer length
 *
 */
void mb_mstr_error_exec_fn_cb(mb_inst_struct *inst, UCHAR dst_addr, const UCHAR* pdu_data_ptr, USHORT pdu_len)
{
    (void)inst;
    (void)dst_addr;
    (void)pdu_data_ptr;
    (void)pdu_len;
}

/**
 * This is modbus master request_t process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So, for real-time of system.Do not execute too much waiting process.
 *
 */
void mb_mstr_rq_success_cb(mb_inst_struct *inst)
{
}

/**
 * Modbus master input register callback function.
 *
 * @param reg_buff input register buffer
 * @param reg_addr input register address
 * @param reg_num input register number
 *
 * @return result
 */
mb_err_enum mb_mstr_reg_input_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num)
{
    //mb_err_enum    status = MB_ENOERR;
    //uint8_t reg_index;

    (void)inst;

    /* it already plus one in modbus function method. */
    //reg_addr--;

    //while (reg_num > 0)
    //{
    //    reg_index = req_find_reg(reg_addr);

    //    mbm_request.target_value[reg_index] =  (uint16_t)(*reg_buff++)<<8;
    //    mbm_request.target_value[reg_index]|=   *reg_buff++;

    //    reg_addr++;
    //    reg_num--;
    //}

    //return status;
}

/**
 * Modbus master holding register callback function.
 *
 * @param reg_buff holding register buffer
 * @param reg_addr holding register address
 * @param reg_num holding register number
 * @param mode read or write
 *
 * @return result
 */



mb_err_enum mb_mstr_reg_holding_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num)
{
    //mb_err_enum    status = MB_ENOERR;

    //uint8_t reg_index=0;

    //(void)inst;

    //reg_addr--;

    //while (reg_num > 0)
    //{
    //    if (mbm_need_preread)
    //    {
    //        mbm_preread_buffer[reg_index] = (uint16_t)(*reg_buff++)<<8;
    //        mbm_preread_buffer[reg_index]|=   *reg_buff++;
    //        reg_index++;
    //    }
    //    else
    //    {
    //        reg_index = req_find_reg(reg_addr);
    //        if (reg_index!=0xFF)
    //        {
    //            mbm_request.target_value[reg_index] =  (uint16_t)(*reg_buff++)<<8;
    //            mbm_request.target_value[reg_index]|=   *reg_buff++;
    //        }
    //        else
    //        {
    //            reg_buff++;
    //            reg_buff++;
    //            //we got extra register. just ignore it for now
    //        }
    //    }
    //    reg_addr++;
    //    reg_num--;
    //}

//    switch (mode)
//    {
//    /* write values to slave registers*/
//    case MB_REG_WRITE:
//        break;
//        while (reg_num > 0)
//        {
//            reg_index = req_find_reg(reg_addr);
//            if (reg_index!=0xFF)
//            {
//                *reg_buff++ = (UCHAR) (mbm_request.target_value[reg_index] >> 8);
//                *reg_buff++ = (UCHAR) (mbm_request.target_value[reg_index] & 0xFF);
//            }
//            else //means we are trying to write registers we do not have
//            {
//                *reg_buff++=0xDE; //should never end up here
//                *reg_buff++=0xAD; //
//            }
//            reg_addr++;
//            reg_num--;
//        }
//        break;
//    /*Get values from slave registers */
//    case MB_REG_READ:
//        while (reg_num > 0)
//        {
//            if (mbm_need_preread)
//            {
//                mbm_preread_buffer[reg_index] = (uint16_t)(*reg_buff++)<<8;
//                mbm_preread_buffer[reg_index]|=   *reg_buff++;
//                reg_index++;
//            }
//            else
//            {
//                reg_index = req_find_reg(reg_addr);
//                if (reg_index!=0xFF)
//                {
//                    mbm_request.target_value[reg_index] =  (uint16_t)(*reg_buff++)<<8;
//                    mbm_request.target_value[reg_index]|=   *reg_buff++;
//                }
//                else
//                {
//                    reg_buff++;
//                    reg_buff++;
//                    //we got extra register. just ignore it for now
//                }
//            }
//            reg_addr++;
//            reg_num--;
//        }
//        break;
//    }

    //return status;
}

/**
 * Modbus master coils callback function.
 *
 * @param reg_buff coils buffer
 * @param reg_addr coils address
 * @param coil_num coils number
 * @param mode read or write
 *
 * @return result
 */
mb_err_enum mb_mstr_reg_coils_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT coil_num)
{
    //mb_err_enum    status = MB_ENOERR;
    //uint8_t nByte=0;

    (void)inst;
    (void)reg_addr;

    //if (mbm_need_preread)
    //{
    //    nByte = coil_num/8 + ((coil_num%8)>0)?1:0;
    //    memcpy(mbm_preread_buffer, reg_buff, nByte);
// Эта реализация memcpy не работает!!!
//        while(nByte>0)
//        {
//            mbm_preread_buffer[nByte] = *(reg_buff+nByte);
//            nByte--;
//        }
// Эта реализация memcpy не работает!!!
    //}
    //else
    //{
    //coils read callback here
    //}

    //return status;
}

/**
 * Modbus master discrete callback function.
 *
 * @param reg_buff discrete buffer
 * @param reg_addr discrete address
 * @param disc_num discrete number
 *
 * @return result
 */
mb_err_enum mb_mstr_reg_discrete_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT disc_num)
{
    //mb_err_enum    status = MB_ENOERR;

    //uint8_t reg_num =  0;
    //uint8_t reg_index;

    (void)inst;

    /* it already plus one in modbus function method. */
    //reg_addr--;

    /* write current discrete values with new values from the protocol stack. */
    //while (disc_num > 0)
    //{
    //    reg_index = req_find_reg(reg_addr+reg_num);
    //    if (reg_index!=0xFF)
    //    {
    //        mbm_request.target_value[reg_index] = (*(reg_buff+(reg_num/8))) &(1<<(reg_num%8));
    //    }

    //    if ((reg_num%8)==7)
    //    {
    //        reg_buff++;
    //    }
    //    disc_num--;
    //    reg_num++;
    //}

    //return status;
}

