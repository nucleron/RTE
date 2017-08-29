/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
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

#define MB_REG_AI00  14
#define MB_REG_AI01  15

#define MB_REG_AI10  16
#define MB_REG_AI11  17

#define MB_REG_AI20  18
#define MB_REG_AI21  19

#define MB_REG_AI30  20
#define MB_REG_AI31  21

#define MB_REG_AO00  22
#define MB_REG_AO01  23

#define MB_REG_AO10  24
#define MB_REG_AO11  25

#define MB_REG_YEAR  26
#define MB_REG_MONTH 27
#define MB_REG_DAY   28
#define MB_REG_HOUR  29
#define MB_REG_MIN   30
#define MB_REG_SEC   31

#define REG_HOLDING_START 1
#define REG_HOLDING_NREGS 64

#define S_DISCRETE_INPUT_START        0
#define S_DISCRETE_INPUT_NDISCRETES   16
#define S_COIL_START                  0
#define S_COIL_NCOILS                 16

UCHAR    ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8+1];
//Master mode:Coils variables
UCHAR    ucSCoilBuf[S_COIL_NCOILS/8+1];

/* ----------------------- Static variables ---------------------------------*/

static mb_inst_struct MBSlave;
static mb_trans_union MBTransport;

static tm mbtime;
static bool mbt_sflg = false;

static bool mb_init_flg = false;
static bool mb_start = false;

static bool mb_gotinit = false;
static bool mb_enabled = false;
static bool mb_ascii = false;
uint32_t mb_baudrate = 9600;
uint8_t mb_slave_addr = 1;

static unsigned short usRegHoldingStart = REG_HOLDING_START;
static unsigned short usRegHoldingBuf[REG_HOLDING_NREGS-MB_REG_SEC-1];



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
    case MB_REG_AI00:
    case MB_REG_AI01:
    case MB_REG_AI10:
    case MB_REG_AI11:
    case MB_REG_AI20:
    case MB_REG_AI21:
    case MB_REG_AI30:
    case MB_REG_AI31:
    case MB_REG_AO00:
    case MB_REG_AO01:
    case MB_REG_AO10:
    case MB_REG_AO11:
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
        return usRegHoldingBuf[reg-MB_REG_SEC-1];
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
    case MB_REG_AI00:
    case MB_REG_AI01:
        /* */
    case MB_REG_AI10:
    case MB_REG_AI11:
        /* */
    case MB_REG_AI20:
    case MB_REG_AI21:
        /* */
    case MB_REG_AI30:
    case MB_REG_AI31:
        /* */
    case MB_REG_AO00:
    case MB_REG_AO01:
        /* */
    case MB_REG_AO10:
    case MB_REG_AO11:
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
        usRegHoldingBuf[reg-MB_REG_SEC-1] = val;
        break;
    }
}

#define LOCAL_PROTO plc_mb
void PLC_IOM_LOCAL_INIT(void)
{
    int i;

    for(i = 0; i < REG_HOLDING_NREGS-MB_REG_SEC-1; i++)
    {
        usRegHoldingBuf[i] = 0;
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
    uint32_t addr;
    switch (PLC_APP->l_tab[i]->v_size)
    {
    case PLC_LSZ_X:
        if (PLC_LT_Q != PLC_APP->l_tab[i]->v_type)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_tp_init, sizeof(plc_mb_err_tp_init));
            plc_iom_errno_print(PLC_ERRNO_MBS_TP_INIT);
            return false;
        }
        if (3 != PLC_APP->l_tab[i]->a_size)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_init, sizeof(plc_mb_err_init));
            plc_iom_errno_print(PLC_ERRNO_MBS_INIT);
            return false;
        }
        if (mb_gotinit)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_mb_err_extrainit, sizeof(plc_mb_err_extrainit));
            plc_iom_errno_print(PLC_ERRNO_MBS_EX_INIT);
            return false;
        }

        mb_gotinit = true;
        mb_baudrate = (int)PLC_APP->l_tab[i]->a_data[1];
        mb_slave_addr = (int)PLC_APP->l_tab[i]->a_data[0];
        mb_ascii = ((int)PLC_APP->l_tab[i]->a_data[2]!=0);
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

//Do once!
void PLC_IOM_LOCAL_END(uint16_t i)
{
    if (!mb_init_flg)
    {
        mb_init_flg = true;
        //If no init location specified then start with default params.
        if (!mb_gotinit)
        {
            mb_ascii = MB_DEFAULT_TRANSPORT;
            mb_slave_addr = MB_DEFAULT_ADDRESS;
            mb_baudrate = MB_DEFAULT_BAUDRATE;
            mb_enabled = true;
            mb_start = true;
        }
        mb_init(&MBSlave, &MBTransport, (mb_ascii)?MB_ASCII:MB_RTU, FALSE, mb_slave_addr, (mb_port_base_struct *)&mbs_inst_usart, mb_baudrate, MB_PAR_NONE);
    }
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    //Do once!
    if (!mb_init_flg) return;

    if (mb_start)
    {
        mb_start = false;
        if (!mb_enabled)
        {
            mb_disable(&MBSlave);
            return;
        }
        mb_enable(&MBSlave);
    }

    plc_rtc_dt_get(&mbtime);
    mb_poll(&MBSlave);
    if (mbt_sflg)
    {
        mbt_sflg = false;
        plc_rtc_dt_set(&mbtime);
    }
}
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    return PLC_APP->l_tab[i]->a_data[0];
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    switch (plc_curr_app->l_tab[i]->v_type)
    {
    case PLC_LT_M:
        *(IEC_UINT *)(plc_curr_app->l_tab[i]->v_buf) = usRegHoldingBuf[plc_curr_app->l_tab[i]->a_data[0]];
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
        usRegHoldingBuf[plc_curr_app->l_tab[i]->a_data[0]] = *(IEC_UINT *)(plc_curr_app->l_tab[i]->v_buf);
        break;
    case PLC_LT_Q:
        if (mb_gotinit)
        {
            mb_enabled = *(bool *)(PLC_APP->l_tab[i]->v_buf);
            mb_start = true; //Now we can start
            mb_gotinit = false; //Do not check locationy any more
        }
        break;
    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO


mb_err_enum
mb_reg_holding_cb(UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num,
                 mb_reg_mode_enum mode)
{
    mb_err_enum    status = MB_ENOERR;
    int             iRegIndex;

    if ((reg_addr >= REG_HOLDING_START) &&
            (reg_addr + reg_num <= REG_HOLDING_START + REG_HOLDING_NREGS))
    {
        iRegIndex = (int)(reg_addr - usRegHoldingStart);
        switch (mode)
        {
            /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while (reg_num > 0)
            {
                USHORT tmp;
                tmp = mb_hr_get(iRegIndex);
                *reg_buff++ = (unsigned char)(tmp >> 8);
                *reg_buff++ = (unsigned char)(tmp & 0xFF);
                iRegIndex++;
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
                mb_hr_set(iRegIndex, tmp);
                iRegIndex++;
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
mb_reg_coils_cb(UCHAR *reg_buff, USHORT reg_addr, USHORT coil_num,
               mb_reg_mode_enum mode)
{
     mb_err_enum    status = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    USHORT          COIL_START;
    USHORT          COIL_NCOILS;
    USHORT          usCoilStart;
    iNReg =  coil_num / 8 + 1;

    COIL_START = S_COIL_START;
    COIL_NCOILS = S_COIL_NCOILS;
    usCoilStart = S_COIL_START;

    /* it already plus one in modbus function method. */
    reg_addr--;

    if ((reg_addr >= COIL_START)
            && (reg_addr + coil_num <= COIL_START + COIL_NCOILS))
    {
        iRegIndex = (USHORT) (reg_addr - usCoilStart) / 8;
        iRegBitIndex = (USHORT) (reg_addr - usCoilStart) % 8;
        switch (mode)
        {
         /* read current coil values from the protocol stack. */
        case MB_REG_READ:
            while (iNReg > 0)
            {
                *reg_buff++ = mb_util_get_bits(&ucSCoilBuf[iRegIndex++],
                        iRegBitIndex, 8);
                iNReg--;
            }
            reg_buff--;
            /* last coils */
            coil_num = coil_num % 8;
            /* filling zero to high bit */
            *reg_buff = *reg_buff << (8 - coil_num);
            *reg_buff = *reg_buff >> (8 - coil_num);
            break;

        /* write current coil values with new values from the protocol stack. */
        case MB_REG_WRITE:
            while (iNReg > 1)
            {
                mb_util_set_bits(&ucSCoilBuf[iRegIndex++], iRegBitIndex, 8,
                        *reg_buff++);
                iNReg--;
            }
            /* last coils */
            coil_num = coil_num % 8;
            /* mb_util_set_bits has bug when ucNBits is zero */
            if (coil_num != 0)
            {
                mb_util_set_bits(&ucSCoilBuf[iRegIndex++], iRegBitIndex, coil_num,
                        *reg_buff++);
            }
            break;
        }
    }
    else
    {
        status = MB_ENOREG;
    }
    return status;
}

mb_err_enum
mb_reg_discrete_cb(UCHAR *reg_buff, USHORT reg_addr, USHORT disc_num)
{
    mb_err_enum    status = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    iNReg =  disc_num / 8 + 1;

    /* it already plus one in modbus function method. */
    reg_addr--;

    if ((reg_addr >= S_DISCRETE_INPUT_START)
            && (reg_addr + disc_num    <= S_DISCRETE_INPUT_START + S_DISCRETE_INPUT_NDISCRETES))
    {
        iRegIndex = (USHORT) (reg_addr - S_DISCRETE_INPUT_START) / 8;
        iRegBitIndex = (USHORT) (reg_addr - S_DISCRETE_INPUT_START) % 8;

         while (iNReg > 0)
        {
            *reg_buff++ = mb_util_get_bits(&ucSCoilBuf[iRegIndex++],
                    iRegBitIndex, 8);
            iNReg--;
        }
        reg_buff--;
        /* last coils */
        disc_num = disc_num % 8;
        /* filling zero to high bit */
        *reg_buff = *reg_buff << (8 - disc_num);
        *reg_buff = *reg_buff >> (8 - disc_num);

    }
    else
    {
        status = MB_ENOREG;
    }

    return status;
}

mb_err_enum
mb_reg_input_cb(UCHAR *reg_buff, USHORT reg_addr, USHORT reg_num)
{
    mb_err_enum    status = MB_ENOERR;
    int             iRegIndex;

    if ((reg_addr >= 0)
        && (reg_addr + reg_num <= REG_HOLDING_NREGS))
    {
        iRegIndex = (int)(reg_addr - S_DISCRETE_INPUT_START);
        while (reg_num > 0)
        {
            *reg_buff++ = (unsigned char)(usRegHoldingBuf[iRegIndex] >> 8);
            *reg_buff++ = (unsigned char)(usRegHoldingBuf[iRegIndex] & 0xFF);
            iRegIndex++;
            reg_num--;
        }
    }
    else
    {
        status = MB_ENOREG;
    }

    return status;
}

