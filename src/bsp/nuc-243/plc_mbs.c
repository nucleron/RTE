/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>

#include <noise_flt.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_ain.h>
#include <plc_aout.h>
#include <plc_diag.h>
#include <plc_rtc.h>
#include <plc_hw.h>
#include <plc_iom.h>

#include <mb.h>
/* ----------------------- Defines ------------------------------------------*/
#define MB_REG_HWST  0
/*Unhandled interrupts*/
#define MB_REG_DIAG1  1
#define MB_REG_DIAG2  2
#define MB_REG_DIAG3  3
#define MB_REG_DIAG4  4
#define MB_REG_DIAG5  5
#define MB_REG_DIAG6  6
/*Reserved values*/
#define MB_REG_RS07  7
#define MB_REG_RS08  8
#define MB_REG_RS09  9
#define MB_REG_RS10  10
#define MB_REG_RS11  11

#define MB_REG_DIN   12
#define MB_REG_DOUT  13

#define MB_REG_AI0   14
#define MB_REG_AI1   15

#define MB_REG_AI2   16
#define MB_REG_AI3   17

#define MB_REG_RS18  18
#define MB_REG_RS19  19

#define MB_REG_RS20  20
#define MB_REG_RS21  21

#define MB_REG_AO0   22
#define MB_REG_AO1   23

#define MB_REG_RS24  24
#define MB_REG_RS25  25

#define MB_REG_YEAR  26
#define MB_REG_MONTH 27
#define MB_REG_DAY   28
#define MB_REG_HOUR  29
#define MB_REG_MIN   30
#define MB_REG_SEC   31

#define REG_HOLDING_START 1
#define REG_HOLDING_NREGS 64

/* ----------------------- Static variables ---------------------------------*/
static mb_inst_struct mb_slave;
static mb_trans_union mb_slave_transport;

static tm mbtime;
static bool mbt_sflg = false;

/*static bool mb_init_flg = false;*/
static bool mb_start = false;

/*static bool mb_gotinit = false;*/
/*static bool mb_waitinit = false;*/
static bool mb_enabled = true;
static bool mb_ascii = MB_DEFAULT_TRANSPORT;
static IEC_BYTE* mb_ascii_ptr = NULL;
uint32_t mb_baudrate = MB_DEFAULT_BAUDRATE;
static IEC_BYTE* mb_baudrate_ptr = NULL;
uint8_t mb_slave_addr = MB_DEFAULT_ADDRESS;
static IEC_BYTE* mb_slave_addr_ptr = NULL;

static unsigned short reg_holding_start = REG_HOLDING_START;
static unsigned short reg_holding_buf[REG_HOLDING_NREGS-MB_REG_SEC-1];

static uint16_t mb_hr_get(uint16_t reg)
{
    uint16_t tmp, i;
    switch (reg)
    {
    case MB_REG_HWST:
        return (uint16_t)plc_diag_status;

    case MB_REG_DIN:
        tmp = 0;
        for (i=0; i<PLC_DI_NUM; i++)
        {
            if (plc_get_din(i))
            {
                tmp |= 1<<i;
            }
        }
        return tmp;

    case MB_REG_DOUT:
        tmp = 0;
        for (i=0; i<PLC_DO_NUM; i++)
        {
            if (plc_get_dout(i))
            {
                tmp |= 1<<i;
            }
        }
        return tmp;
    case MB_REG_DIAG1:
    case MB_REG_DIAG2:
    case MB_REG_DIAG3:
    case MB_REG_DIAG4:
    case MB_REG_DIAG5:
    case MB_REG_DIAG6:
        return (uint16_t)(PLC_DIAG_IRQS[reg]&0xFFFF);

    case MB_REG_RS07:
    case MB_REG_RS08:
    case MB_REG_RS09:
    case MB_REG_RS10:
    case MB_REG_RS11:
        /* */
        break;
    case MB_REG_AI0:
    case MB_REG_AI1:
    case MB_REG_AI2:
    case MB_REG_AI3:
    {
        reg -= MB_REG_AI0;
        return analog_input[reg].signal_level;
    }
    case MB_REG_AO0:
        return plc_aout_dataA;
    case MB_REG_AO1:
        return plc_aout_dataB;
    case MB_REG_RS18:
    case MB_REG_RS19:
    case MB_REG_RS20:
    case MB_REG_RS21:
    case MB_REG_RS24:
    case MB_REG_RS25:
        break;
    case MB_REG_YEAR:
        return (uint16_t)mbtime.tm_year;

    case MB_REG_MONTH:
        return (uint16_t)mbtime.tm_mon;

    case MB_REG_DAY:
        return (uint16_t)mbtime.tm_day;

    case MB_REG_HOUR:
        return (uint16_t)mbtime.tm_hour;

    case MB_REG_MIN:
        return (uint16_t)mbtime.tm_min;

    case MB_REG_SEC:
        return (uint16_t)mbtime.tm_sec;

    default:
        return reg_holding_buf[reg-MB_REG_SEC-1];
    }
    return 0;
}

static void mb_hr_set(uint16_t reg, uint16_t val)
{
    switch (reg)
    {
        /* */
    case MB_REG_HWST:
        /* */
    case MB_REG_DIAG1:
    case MB_REG_DIAG2:
    case MB_REG_DIAG3:
    case MB_REG_DIAG4:
    case MB_REG_DIAG5:
    case MB_REG_DIAG6:
        /* */
    case MB_REG_RS07:
    case MB_REG_RS08:
    case MB_REG_RS09:
    case MB_REG_RS10:
    case MB_REG_RS11:
        /* */
    case MB_REG_DIN:
        /* */
    case MB_REG_DOUT:
        /* */
    case MB_REG_AI0:
    case MB_REG_AI1:
        /* */
    case MB_REG_AI2:
    case MB_REG_AI3:
        /* */
    case MB_REG_RS18:
    case MB_REG_RS19:
        /* */
    case MB_REG_RS20:
    case MB_REG_RS21:
        /* */
    case MB_REG_AO0:
    case MB_REG_AO1:
        /* */
    case MB_REG_RS24:
    case MB_REG_RS25:
        break;

    case MB_REG_YEAR:
        mbtime.tm_year = val;
        mbt_sflg = true;
        break;

    case MB_REG_MONTH:
        mbtime.tm_mon = val;
        mbt_sflg = true;
        break;

    case MB_REG_DAY:
        mbtime.tm_day = val;
        mbt_sflg = true;
        break;

    case MB_REG_HOUR:
        mbtime.tm_hour = val;
        mbt_sflg = true;
        break;

    case MB_REG_MIN:
        mbtime.tm_min = val;
        mbt_sflg = true;
        break;

    case MB_REG_SEC:
        mbtime.tm_sec = val;
        mbt_sflg = true;
        break;

    default:
        reg_holding_buf[reg-MB_REG_SEC-1] = val;
        break;
    }
}

#define LOCAL_PROTO plc_mb
void PLC_IOM_LOCAL_INIT(void)
{
    int i;

    for(i = 0; i < REG_HOLDING_NREGS-MB_REG_SEC-1; i++)
    {
        reg_holding_buf[i] = 0;
    }
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
/*
static const char plc_mb_err_asz[]     = "Modbus local register adress must be one number!";
static const char plc_mb_err_tp[]      = "Modbus local supports only memory locations for registers!";
static const char plc_mb_err_addr[]    = "Modbus local register adress must be in 0...31!";
static const char plc_mb_err_init[]    = "Modbus wrong init value format. Must be QX2.[baudrate].[mode]";
static const char plc_mb_err_tp_init[] = "Modbus local supports only output location for initialization!";
static const char plc_mb_err_extrainit[] = "Only one modbus init variable allowed!";
*/
bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
//    uint32_t addr;
    switch (PLC_APP->l_tab[i]->v_size)
    {
    case PLC_LSZ_B: //configuration variables:
        if(PLC_LT_M != PLC_APP->l_tab[i]->v_type)
        {
            plc_iom_errno_print(PLC_ERRNO_MBS_INIT);
            return false;
        }
        if (1 != PLC_APP->l_tab[i]->a_size)
        {
            plc_iom_errno_print(PLC_ERRNO_MBS_INIT);
            return false;
        }
        /*
        if(0 != PLC_APP->l_tab[i]->a_data[0])
        {
            plc_iom_errno_print(PLC_ERRNO_MBS_INIT);
            return false;
        }
        */

        switch((int)PLC_APP->l_tab[i]->a_data[0])
        {
        case 0: // slave address
            mb_slave_addr_ptr = (IEC_BYTE*)(PLC_APP->l_tab[i]->v_buf);
            break;
        case 1: //baudrate
            mb_baudrate_ptr = (IEC_BYTE*)(PLC_APP->l_tab[i]->v_buf);
            break;
        case 2://mode
            mb_ascii_ptr = (IEC_BYTE*)(PLC_APP->l_tab[i]->v_buf);
            break;
        }
        return true;

    case PLC_LSZ_W:
        if (PLC_LT_M != PLC_APP->l_tab[i]->v_type)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_tp, sizeof(plc_mb_err_tp));
            plc_iom_errno_print(PLC_ERRNO_MBS_TP);
            return false;
        }
        if (1 != PLC_APP->l_tab[i]->a_size)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_asz, sizeof(plc_mb_err_asz));
            plc_iom_errno_print(PLC_ERRNO_MBS_ASZ);
            return false;
        }
        if (32 <= PLC_APP->l_tab[i]->a_data[0])
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_addr, sizeof(plc_mb_err_addr));
            plc_iom_errno_print(PLC_ERRNO_MBS_ADDR);
            return false;
        }
        return true;

    default:
        PLC_LOG_ERR_SZ();//plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_iom_err_sz, plc_iom_err_sz_sz);
        return false;
    }
    return false;
}

void PLC_IOM_LOCAL_START(uint16_t i)
{
    (void)i;

    if(mb_baudrate_ptr)
    {
        switch(*(IEC_BYTE*)(mb_baudrate_ptr))
        {
        case 0:
            mb_baudrate=0;
            break;
        case 1:
            mb_baudrate=1200;
            break;
        case 2:
            mb_baudrate=2400;
            break;
        case 3:
            mb_baudrate=4800;
            break;

        default:
        case 4:
            mb_baudrate=9600;
            break;

        case 5:
            mb_baudrate=19200;
            break;
        case 6:
            mb_baudrate=38400;
            break;
        case 7:
            mb_baudrate=57600;
            break;
        case 8:
            mb_baudrate=115200;
            break;
        }
    }

    if(mb_ascii_ptr)
    {
        mb_ascii = *(IEC_BYTE*)(mb_ascii_ptr);
    }

    if(mb_slave_addr_ptr)
    {
        mb_slave_addr = *(IEC_BYTE*)(mb_slave_addr_ptr);
    }

    if(mb_slave_addr>244) mb_slave_addr = 244;

    if(0 == mb_baudrate || 0 == mb_slave_addr)
        mb_enabled = false;

    mb_init(&mb_slave, &mb_slave_transport, (mb_ascii)?MB_ASCII:MB_RTU, FALSE, mb_slave_addr, (mb_port_base_struct *)&mbs_inst_usart, mb_baudrate, MB_PAR_NONE);
    mb_start = true;

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
    (void)tick;
    //Do once!

    if (mb_start)
    {
        mb_start = false;
        if (!mb_enabled)
        {
            mb_disable(&mb_slave);
            return;
        }
        mb_enable(&mb_slave);
    }

    plc_rtc_dt_get(&mbtime);
    mb_poll(&mb_slave);
    if (mbt_sflg)
    {
        mbt_sflg = false;
        plc_rtc_dt_set(&mbtime);
    }
}
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    return (PLC_APP->l_tab[i]->a_data[0])<<(PLC_APP->l_tab[i]->v_size*8);
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_M:
        if (PLC_LSZ_W == PLC_APP->l_tab[i]->v_size)
        {
            *(IEC_UINT *)(plc_curr_app->l_tab[i]->v_buf) = reg_holding_buf[plc_curr_app->l_tab[i]->a_data[0]];
        }
        break;
    case PLC_LT_Q://Write only access to init location
    default:
        break;
    }
    return 0;
}

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_M:
        if (PLC_LSZ_W == PLC_APP->l_tab[i]->v_size)
        {
            reg_holding_buf[plc_curr_app->l_tab[i]->a_data[0]] = *(IEC_UINT *)(plc_curr_app->l_tab[i]->v_buf);
        }
        break;

    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO

mb_err_enum
mb_reg_input_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num)
{
    (void)inst;
    (void)reg_buff;
    (void)reg_addr;
    (void)reg_num;
    return MB_ENOREG;
}

mb_err_enum
mb_reg_holding_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num,
                  mb_reg_mode_enum mode)
{
    mb_err_enum    status = MB_ENOERR;
    int             reg_index;

    (void)inst;

    if ((reg_addr >= REG_HOLDING_START) &&
            (reg_addr + reg_num <= REG_HOLDING_START + REG_HOLDING_NREGS))
    {
        reg_index = (int)(reg_addr - reg_holding_start);
        switch (mode)
        {
        /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while (reg_num > 0)
            {
                USHORT tmp;
                tmp = mb_hr_get(reg_index);
                *reg_buff++ = (unsigned char)(tmp >> 8);
                *reg_buff++ = (unsigned char)(tmp & 0xFF);
                reg_index++;
                reg_num--;
            }
            break;

        /* Update current register values with new values from the
         * protocol stack. */
        case MB_REG_WRITE:
            while (reg_num > 0)
            {
                USHORT tmp;
                tmp  = *reg_buff++ << 8;
                tmp |= *reg_buff++;
                mb_hr_set(reg_index, tmp);
                reg_index++;
                reg_num--;
            }
        }
    }
    else
    {
        status = MB_ENOREG;
    }
    return status;
}


mb_err_enum
mb_reg_coils_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT coil_num,
                mb_reg_mode_enum mode)
{
    (void)inst;
    (void)reg_buff;
    (void)reg_addr;
    (void)coil_num;
    (void)mode;
    return MB_ENOREG;
}

mb_err_enum
mb_reg_discrete_cb(mb_inst_struct *inst, UCHAR *reg_buff, USHORT reg_addr, USHORT disc_num)
{
    (void)inst;
    (void)reg_buff;
    (void)reg_addr;
    (void)disc_num;
    return MB_ENOREG;
}

