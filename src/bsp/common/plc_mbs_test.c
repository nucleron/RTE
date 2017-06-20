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

#include <serial_port.h>
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

static MBInstance MBSlave;
static MBASCIIInstance MBTransport;

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



static uint16_t mb_hr_get( uint16_t reg )
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

static void mb_hr_set( uint16_t reg, uint16_t val )
{
    switch(reg)
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

    for( i = 0; i < REG_HOLDING_NREGS-MB_REG_SEC-1; i++ )
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
            //plc_curr_app->log_msg_post(LOG_CRITICAL,(char *)plc_mb_err_init,sizeof(plc_mb_err_init));
            plc_iom_errno_print(PLC_ERRNO_MBS_INIT);
            return false;
        }
        if (mb_gotinit)
        {
            //plc_curr_app->log_msg_post(LOG_CRITICAL,(char *)plc_mb_err_extrainit,sizeof(plc_mb_err_extrainit));
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
        PLC_LOG_ERR_SZ();//plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)plc_iom_err_sz, plc_iom_err_sz_sz );
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
        eMBInitASCII(&MBSlave,&MBTransport, mb_slave_addr, MBS_USART, mb_baudrate, MB_PAR_NONE);
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
        if(!mb_enabled)
        {
            eMBDisable(&MBSlave);
            return;
        }
        eMBEnable(&MBSlave);
    }

    plc_rtc_dt_get( &mbtime );
    eMBPoll(&MBSlave);
    if (mbt_sflg)
    {
        mbt_sflg = false;
        plc_rtc_dt_set( &mbtime );
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
        *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf) = usRegHoldingBuf[plc_curr_app->l_tab[i]->a_data[0]];
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
        usRegHoldingBuf[plc_curr_app->l_tab[i]->a_data[0]] = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
        break;
    case PLC_LT_Q:
        if(mb_gotinit)
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


eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_HOLDING_START ) &&
            ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch ( eMode )
        {
            /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                USHORT tmp;
                tmp = mb_hr_get(iRegIndex);
                *pucRegBuffer++ = ( unsigned char )( tmp >> 8 );
                *pucRegBuffer++ = ( unsigned char )( tmp & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

            /* Update current register values with new values from the
             * protocol stack. */
        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                USHORT tmp;
                tmp  = *pucRegBuffer++ << 8;
                tmp |= *pucRegBuffer++;
                mb_hr_set(iRegIndex, tmp);
                iRegIndex++;
                usNRegs--;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
     eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    USHORT          COIL_START;
    USHORT          COIL_NCOILS;
    USHORT          usCoilStart;
    iNReg =  usNCoils / 8 + 1;

    COIL_START = S_COIL_START;
    COIL_NCOILS = S_COIL_NCOILS;
    usCoilStart = S_COIL_START;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= COIL_START)
            && (usAddress + usNCoils <= COIL_START + COIL_NCOILS))
    {
        iRegIndex = (USHORT) (usAddress - usCoilStart) / 8;
        iRegBitIndex = (USHORT) (usAddress - usCoilStart) % 8;
        switch (eMode)
        {
         /* read current coil values from the protocol stack. */
        case MB_REG_READ:
            while (iNReg > 0)
            {
                *pucRegBuffer++ = xMBUtilGetBits(&ucSCoilBuf[iRegIndex++],
                        iRegBitIndex, 8);
                iNReg--;
            }
            pucRegBuffer--;
            /* last coils */
            usNCoils = usNCoils % 8;
            /* filling zero to high bit */
            *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
            *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
            break;

        /* write current coil values with new values from the protocol stack. */
        case MB_REG_WRITE:
            while (iNReg > 1)
            {
                xMBUtilSetBits(&ucSCoilBuf[iRegIndex++], iRegBitIndex, 8,
                        *pucRegBuffer++);
                iNReg--;
            }
            /* last coils */
            usNCoils = usNCoils % 8;
            /* xMBUtilSetBits has bug when ucNBits is zero */
            if (usNCoils != 0)
            {
                xMBUtilSetBits(&ucSCoilBuf[iRegIndex++], iRegBitIndex, usNCoils,
                        *pucRegBuffer++);
            }
            break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    iNReg =  usNDiscrete / 8 + 1;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= S_DISCRETE_INPUT_START)
            && (usAddress + usNDiscrete    <= S_DISCRETE_INPUT_START + S_DISCRETE_INPUT_NDISCRETES))
    {
        iRegIndex = (USHORT) (usAddress - S_DISCRETE_INPUT_START) / 8;
        iRegBitIndex = (USHORT) (usAddress - S_DISCRETE_INPUT_START) % 8;

         while (iNReg > 0)
        {
            *pucRegBuffer++ = xMBUtilGetBits(&ucSCoilBuf[iRegIndex++],
                    iRegBitIndex, 8);
            iNReg--;
        }
        pucRegBuffer--;
        /* last coils */
        usNDiscrete = usNDiscrete % 8;
        /* filling zero to high bit */
        *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);

    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= 0 )
        && ( usAddress + usNRegs <= REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - S_DISCRETE_INPUT_START );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

