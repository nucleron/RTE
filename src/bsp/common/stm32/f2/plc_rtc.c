/*
 * This file is based on the libopencm3 project.
 *
 * Copyright (C) 2016 Nucleron R&D LLC (main@nucleron.ru)
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>

#include <plc_config.h>
#include <plc_backup.h>
#include <plc_rtc.h>
#include <plc_hw.h>


#define PLC_RTC_DIV_VAL 0x7FFF
#define PLC_BKP_RTC_IS_OK   MMIO32(RTC_BKP_BASE + PLC_BKP_RTC_IS_OK_OFFSET)


#define BACKUP_UNLOCK() PWR_CR |= PWR_CR_DBP
#define BACKUP_LOCK() PWR_CR &= ~PWR_CR_DBP

volatile uint8_t plc_rtc_failure = 0; //Not failed by default

/*
 *  Julian Day Number computation
 *  See http://en.wikipedia.org/wiki/Julian_day
 */
static uint32_t jdn(tm *date_time)
{
    uint8_t a, m;
    uint16_t y;
    uint32_t ret;

    /* Compute helper factors */
    a = (14 - (uint8_t)date_time->tm_mon)/12;
    m = (uint8_t)date_time->tm_mon + 12*a - 3;
    y = (uint16_t)date_time->tm_year + 4800 - a;

    /* Julian day number computation */
    ret = (uint32_t)date_time->tm_day;
    ret += (153ul * (uint32_t)m + 2ul)/ 5ul;
    ret += 365ul * (uint32_t)y;
    ret += (uint32_t)(y/4  );
    ret -= (uint32_t)(y/100);
    ret += (uint32_t)(y/400);
    ret -= 32045ul;

    return ret;
}

/*
 * Compute unix-time seconds
 */
static uint64_t dt_to_sec(tm *date_time)
{
    uint64_t ret;

    /*Count days from 01.01.1970*/
    ret = jdn(date_time);
    ret -= 2440588ul;

    /*Convert day number to seconds*/
    ret *= 24ul;
    ret += date_time->tm_hour;
    ret *= 60ul;
    ret += date_time->tm_min;
    ret *= 60ul;
    ret += date_time->tm_sec;

    return ret;
}

static void  jdn_to_dt (uint32_t jdn, tm *date_time)
{
    uint32_t a, b, c, d, e, m;

    a = jdn + 32044;
    b = (4 * a + 3) /146097;
    c = a - (146097 * b) /4;
    d = (4 * c + 3) /1461;
    e = c - (1461 * d) /4;
    m = (5 * e + 2) /153;

    date_time->tm_year = 100*b + d - 4800 + (m/10);
    date_time->tm_mon = m + 3 - 12 * (m /10);
    date_time->tm_day = e - (153 * m + 2) /5 + 1;
}

void  sec_to_dt (uint32_t utime, tm *date_time)
{
    date_time->tm_sec = utime%60;
    utime /= 60;
    date_time->tm_min = utime%60;
    utime /= 60;
    date_time->tm_hour = utime%24;
    utime /= 24;        //Unix day number
    utime += 2440588ul; //JDN

    jdn_to_dt(utime, date_time);
}


static bool start_flg = true;
#define PLC_BASE_TICK_NS (100000)
#define PLC_RTC_CORR_THR (1200000000)
static int32_t plc_rtc_tick_ns;// Initial tick value is 100'000ns
static int64_t plc_rtc_correction;//Correction to RTC time

//static uint32_t plc_rtc_dr = 0;
//static uint32_t plc_rtc_tr = 0;

static uint64_t plc_rtc_sec = 0;

uint32_t plc_rtc_clken_and_check(void)
{
    uint32_t ret;
    rcc_periph_clock_enable(RCC_PWR);
    ret = PLC_BKP_RTC_IS_OK;

    if (ret)
    {
        /*Sunchronize with HW RTC on start*/
        static tm rtc_time;
        uint32_t tmp;

        tmp = RTC_TR;
        rtc_time.tm_sec   = (unsigned char)(  tmp & 0x0000000F);
        rtc_time.tm_sec  += (unsigned char)(((tmp & 0x000000F0) >> 4) * 10);
        rtc_time.tm_min   = (unsigned char)(( tmp & 0x00000F00) >> 8);
        rtc_time.tm_min  += (unsigned char)(((tmp & 0x0000F000) >> 12) * 10);
        rtc_time.tm_hour  = (unsigned char)(( tmp & 0x000F0000) >> 16);
        rtc_time.tm_hour += (unsigned char)(((tmp & 0x00F00000) >> 20) * 10);

        tmp = RTC_DR;
        rtc_time.tm_day   = (unsigned char)(  tmp & 0x0000000F);
        rtc_time.tm_day  += (unsigned char)(((tmp & 0x00000030) >> 4) * 10);
        rtc_time.tm_mon   = (unsigned char)(( tmp & 0x00000F00) >> 8);
        rtc_time.tm_mon  += (unsigned char)(((tmp & 0x00001000) >> 12) * 10);
        rtc_time.tm_year  = (unsigned char)(( tmp & 0x000F0000) >> 16);
        rtc_time.tm_year += (unsigned char)(((tmp & 0x00F00000) >> 20) * 10);
        rtc_time.tm_year += 2000; // 16 is actually 2016

        plc_rtc_sec = dt_to_sec(&rtc_time);
        start_flg   = true;
    }

    return ret ;
}


void _plc_rtc_dt_set(tm* time)
{
    uint32_t i;
    uint32_t tmp;

    if (plc_rtc_failure)
    {
        return;
    }

    PLC_BKP_RTC_IS_OK = 0;
    rtc_unlock();

    RTC_ISR |= RTC_ISR_INIT; //Init mode

    for(i=0; i<1000000; i++)
    {
        if (RTC_ISR & RTC_ISR_INITF)
        {
            break;
        }
    }

    if (!(RTC_ISR & RTC_ISR_INITF))
    {
        goto lse_error;
    }

    tmp = 0;
    tmp |= (time->tm_sec%10)<< RTC_TR_SU_SHIFT;
    tmp |= (time->tm_sec/10)<< RTC_TR_ST_SHIFT;

    tmp |= (time->tm_min%10)<< RTC_TR_MNU_SHIFT;
    tmp |= (time->tm_min/10)<< RTC_TR_MNT_SHIFT;

    tmp |= (time->tm_hour%10)<< RTC_TR_HU_SHIFT;
    tmp |= (time->tm_hour/10)<< RTC_TR_HT_SHIFT;

    RTC_TR = tmp;
    //set date
    tmp=0;
    tmp = 1<<RTC_DR_WDU_SHIFT;
    tmp |= (time->tm_day%10) << RTC_DR_DU_SHIFT;
    tmp |= (time->tm_day/10) << RTC_DR_DT_SHIFT;

    tmp |= (time->tm_mon%10) << RTC_DR_MU_SHIFT;
    tmp |= (time->tm_mon/10) << RTC_DR_MT_SHIFT;

    tmp |= (time->tm_year%10) << RTC_DR_YU_SHIFT;
    tmp |= ((time->tm_year%100)/10) << RTC_DR_YT_SHIFT;

    RTC_DR = tmp;

    RTC_ISR &= ~RTC_ISR_INIT;

    /* Normal termination */
    rtc_lock();
    PLC_BKP_RTC_IS_OK = 1;
    plc_rtc_sec       = dt_to_sec(time);
    start_flg         = true;
    return;
    /* LSE error occured */
lse_error:
    //plc_diag_status |= PLC_DIAG_ERR_LSE;
    rtc_lock();
    plc_rtc_failure = 1;
}

void plc_rtc_init(tm* time)
{
    uint32_t i;

    rcc_periph_clock_enable(RCC_PWR);

    BACKUP_UNLOCK(); //Disable backup domain write protect
    /* LSE oscillator clock used as the RTC clock */
    RCC_BDCR |= (1<<8);
    RCC_BDCR |= RCC_BDCR_LSEON;
    rcc_periph_clock_enable(RCC_RTC);

    //Wait for LSE oscillator start
    for(i=0; i<1000000; i++)
    {
        if (RCC_BDCR  &  RCC_BDCR_LSERDY)
        {
            break;
        }
    }

    if (!(RCC_BDCR  & RCC_BDCR_LSERDY))
    {
        //plc_diag_status |= PLC_DIAG_ERR_LSE;
        plc_rtc_failure = 1;
        BACKUP_LOCK();
        return;
    }

    _plc_rtc_dt_set(time);
    BACKUP_LOCK();
}

void plc_rtc_dt_set(tm* time)
{

    BACKUP_UNLOCK();
    PLC_DISABLE_INTERRUPTS();

    _plc_rtc_dt_set(time);

    PLC_ENABLE_INTERRUPTS();
    BACKUP_LOCK();
}

void plc_rtc_dt_get(tm* time)
{
    uint32_t utime;
    IEC_TIME current_time;

    plc_rtc_time_get(&current_time);
    utime = current_time.tv_sec;
    sec_to_dt(utime, time);
}

//THis function is called every 100us in PLC_WAIT_TMR_ISR
void _plc_rtc_poll(void)
{
    static uint32_t last_sec = 0;

    uint32_t current_sec;

    current_sec = RTC_TR & 0x0000000F;

    if (start_flg)
    {
        start_flg = false;
        last_sec = current_sec;

        //plc_rtc_dr = RTC_DR;
        //plc_rtc_tr = RTC_TR;
        // Initial tick value is 100'000ns or 100us
        plc_rtc_tick_ns = PLC_BASE_TICK_NS;
        plc_rtc_correction = 0;
    }

    // Count ticks from last RTC second update
    plc_rtc_correction += plc_rtc_tick_ns;
    //Check if LSE is working
    if ((plc_rtc_correction < PLC_RTC_CORR_THR) &&  (plc_rtc_correction >- PLC_RTC_CORR_THR))
    {
        //Check if second has passed
        if (0 != current_sec - last_sec)
        {
            last_sec = current_sec;
            //Update buffered date and time registers
            //plc_rtc_dr = RTC_DR;
            //plc_rtc_tr = RTC_TR;
            plc_rtc_sec++;
            //This second is in plc_rtc_dr now!
            plc_rtc_correction -= 1000000000;
            //Update plc_rtc_tick_ns
            plc_rtc_tick_ns = PLC_BASE_TICK_NS - (plc_rtc_correction/20000);
        }
    }
    else
    {
        plc_rtc_failure = 1;
    }
}

void plc_rtc_time_get(IEC_TIME *current_time)
{
    int64_t rtc_sec;
    int64_t rtc_corr;

    //Read buffered register values
    PLC_DISABLE_INTERRUPTS();
    rtc_sec  = plc_rtc_sec;
    rtc_corr = plc_rtc_correction;
    PLC_ENABLE_INTERRUPTS();

    /* Convert current date/time to unix time seconds */
    current_time->tv_sec  = rtc_sec;
    current_time->tv_sec += (rtc_corr / 1000000000);
    current_time->tv_nsec = (rtc_corr % 1000000000);
}



