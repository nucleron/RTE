/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */
#include <stdint.h>
#include <stdbool.h>

#include <noise_flt.h>

#include <plc_config.h>
#include <plc_ain.h>
#include <plc_aout.h>
#include <plc_hmi.h>
#include <iec_std_lib.h>
#include <dbnc_flt.h>

#define MIN(x,y) (((x)>(y))?(y):(x))

extern uint8_t plc_hmi_bri;

extern uint32_t mb_baudrate;
extern uint8_t mb_slave_addr;

extern dbnc_flt_t in_flt[];
uint8_t din_chnl = 0;

uint8_t ain_chnl = 0;

extern plc_hmi_dm_t plc_hmi_sys;

#define PLC_HMI_SYS_PAR_YEAR 0
#define PLC_HMI_SYS_PAR_MMDD 1
#define PLC_HMI_SYS_PAR_HHMM 2

#define PLC_HMI_SYS_PAR_BRI  3

#define PLC_HMI_SYS_PAR_DI   4
#define PLC_HMI_SYS_PAR_DQ   5

#define PLC_HMI_SYS_PAR_DICN 6
#define PLC_HMI_SYS_PAR_ONF  7
#define PLC_HMI_SYS_PAR_OFFF 8

#define PLC_HMI_SYS_PAR_AICN 9   //Analog input sellect
#define PLC_HMI_SYS_PAR_AISL 10  //Analog input signal level
#define PLC_HMI_SYS_PAR_AIMD 11  //Analog input mode
#define PLC_HMI_SYS_PAR_AIMF 12  //Analog input median filter depth
#define PLC_HMI_SYS_PAR_AIAF 13  //Analog input average filter depth
#define PLC_HMI_SYS_PAR_AISP 14  //Analog input sampling period

#define PLC_HMI_SYS_PAR_AO0 15   //Analog output 0
#define PLC_HMI_SYS_PAR_AO1 16   //Analog output 0

#define PLC_HMI_SYS_PAR_MBSS 20
#define PLC_HMI_SYS_PAR_MBSA 21

const plc_hmi_par_t plc_hmi_sys_ptype[] =
{
    PLC_HMI_UINT, //YYYY
    PLC_HMI_MMDD, //MMDD
    PLC_HMI_HHMM, //HHMM
    PLC_HMI_UINT, //Bri

    PLC_HMI_EMPTY,//Discrete inputs
    PLC_HMI_EMPTY,//Discrete outputs

    PLC_HMI_UINT, //DI chanel number
    PLC_HMI_RO_UINT, //DI  on filter
    PLC_HMI_RO_UINT, //DI off filter

    PLC_HMI_UINT,    //AI chanel
    PLC_HMI_RO_UINT, //AI sig level
    PLC_HMI_RO_UINT, //AI mode
    PLC_HMI_RO_UINT, //AI median depth
    PLC_HMI_RO_UINT, //AI average depth
    PLC_HMI_RO_UINT, //AI period

    PLC_HMI_RO_UINT, //AO 0 val
    PLC_HMI_RO_UINT, //AO 1 val

    PLC_HMI_NOT_USED,PLC_HMI_NOT_USED,PLC_HMI_NOT_USED,

    PLC_HMI_RO_UINT, //MBS speed
    PLC_HMI_RO_UINT, //MBS addr

    PLC_HMI_NOT_USED
};
const uint8_t plc_hmi_sys_psize = sizeof(plc_hmi_sys_ptype)/sizeof(plc_hmi_par_t);

uint16_t hmi_sys_get(uint8_t par)
{
    uint32_t i;
    tm now;

    if (par <= PLC_HMI_SYS_PAR_HHMM)
    {
        plc_rtc_dt_get(&now);
    }

    switch(par)
    {
    case PLC_HMI_SYS_PAR_YEAR: //YYYY
        return now.tm_year;
    case PLC_HMI_SYS_PAR_MMDD: //MM.DD
        return now.tm_day+now.tm_mon*100;
    case PLC_HMI_SYS_PAR_HHMM: //HH.MM
        return now.tm_min + now.tm_hour*100;

    case PLC_HMI_SYS_PAR_BRI: //hmi display plc_hmi_bri
        return plc_hmi_bri;

    case PLC_HMI_SYS_PAR_DI:
        for(i=0; i<PLC_DI_NUM; i++)
        {
            if (plc_get_din(i))
            {
                plc_hmi_sys.leds |= (1<<i);
            }
            else
            {
                plc_hmi_sys.leds &= ~(1<<i);
            }
        }
        return 0;
    case PLC_HMI_SYS_PAR_DQ:
        for(i=0; i<PLC_DO_NUM; i++)
        {
            if (plc_get_dout(i))
            {
                plc_hmi_sys.leds |= (1<<i);
            }
            else
            {
                plc_hmi_sys.leds &= ~(1<<i);
            }
        }
        return 0;

    case PLC_HMI_SYS_PAR_DICN:
        return din_chnl;
    case PLC_HMI_SYS_PAR_ONF:
        return in_flt[din_chnl].thr_on;
    case PLC_HMI_SYS_PAR_OFFF:
        return in_flt[din_chnl].thr_off;

    case PLC_HMI_SYS_PAR_AICN:
        return ain_chnl;
    case PLC_HMI_SYS_PAR_AISL:
        return analog_input[ain_chnl].signal_level;
    case PLC_HMI_SYS_PAR_AIMD:
        return analog_input[ain_chnl].mode;
    case PLC_HMI_SYS_PAR_AIMF:
        return analog_input[ain_chnl].median.depth;
    case PLC_HMI_SYS_PAR_AIAF:
        return analog_input[ain_chnl].ave.depth;
    case PLC_HMI_SYS_PAR_AISP:
        return analog_input[ain_chnl].polling_period/10; //0.01sec

    case PLC_HMI_SYS_PAR_AO0:
        return plc_aout_dataA;
    case PLC_HMI_SYS_PAR_AO1:
        return plc_aout_dataB;

    case PLC_HMI_SYS_PAR_MBSS:
        return mb_baudrate/100;
    case PLC_HMI_SYS_PAR_MBSA:
        return mb_slave_addr;

    default:
        return 0; //dummy
    }
}

//Спасибо Волкову Мише!!!
static const uint8_t month_table[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};// Количество дней в месяце. количество дней = month_table[ номер месяца ]
static uint8_t leap_year_chk( uint8_t month, uint8_t year )// Проверка на 29 февраля
{
    if( (month == 2) && (year % 4 == 0) ) return 1;
    return 0;
}

static void date_check_day(tm *now)
{
    uint8_t max;
    max = month_table[now->tm_mon] + leap_year_chk(now->tm_mon, now->tm_year);
    if( now->tm_day > max )
    {
        now->tm_day = 1;
    }
}


uint16_t hmi_sys_chk(uint8_t par, uint16_t val)
{
    tm now;
    if (par <= PLC_HMI_SYS_PAR_HHMM)
    {
        plc_rtc_dt_get(&now);
    }

    switch(par)
    {
    case PLC_HMI_SYS_PAR_YEAR: //YYYY
        if (val>2099)
        {
            return 2000;
        }
        if (val<2000)
        {
            return 2099;
        }
        break;
    case PLC_HMI_SYS_PAR_MMDD: //MM.DD
        now.tm_day = val%100;
        now.tm_mon = val/100;

        if (now.tm_mon > 12)
        {
            now.tm_mon = 1;
        }

        date_check_day(&now);
        val = (uint16_t)now.tm_day + 100*(uint16_t)now.tm_mon;
        break;

    case PLC_HMI_SYS_PAR_HHMM: //HH.MM
        now.tm_min = val%100;
        now.tm_hour = val/100;

        if (now.tm_min > 59)
        {
            now.tm_min = 1;
        }
        if (now.tm_hour > 23)
        {
            now.tm_hour = 1;
        }

        val = (uint16_t)now.tm_min + 100*(uint16_t)now.tm_hour;
        break;

    case PLC_HMI_SYS_PAR_BRI: //hmi display plc_hmi_bri
        if (val > PLC_HMI_BRI_LIM)
        {
            return 1;
        }
        break;

    case PLC_HMI_SYS_PAR_DICN:
        if (val >= PLC_DI_NUM )
        {
            return 0;
        }
        break;

    case PLC_HMI_SYS_PAR_AICN:
        if (val >= PLC_AI_NUM )
        {
            return 0;
        }
        break;
    }
    return val;
}
void     hmi_sys_set(uint8_t par, uint16_t val)
{
    tm now;
    if (par <= PLC_HMI_SYS_PAR_HHMM)
    {
        plc_rtc_dt_get(&now);
    }

    switch(par)
    {
    case PLC_HMI_SYS_PAR_YEAR: //YYYY
        now.tm_year = val;
        plc_rtc_dt_set(&now);
        break;
    case PLC_HMI_SYS_PAR_MMDD: //MM.DD
        now.tm_day = val%100;
        now.tm_mon = val/100;
        plc_rtc_dt_set(&now);
        break;
    case PLC_HMI_SYS_PAR_HHMM: //HH.MM
        now.tm_min = val%100;
        now.tm_hour = val/100;
        plc_rtc_dt_set(&now);
        break;

    case PLC_HMI_SYS_PAR_BRI: //hmi display plc_hmi_bri
        plc_hmi_bri = MIN(val,PLC_HMI_BRI_LIM);
        plc_backup_save_brightness(plc_hmi_bri);
        break;

    case PLC_HMI_SYS_PAR_DICN:
        din_chnl = MIN(val,(PLC_DI_NUM-1));
        break;

    case PLC_HMI_SYS_PAR_AICN:
        ain_chnl = MIN(val,(PLC_AI_NUM-1));
        break;
    }
}
