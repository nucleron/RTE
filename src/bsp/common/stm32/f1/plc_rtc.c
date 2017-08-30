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
#include <plc_diag.h>
#include <plc_rtc.h>
#include <plc_hw.h>

#define PLC_RTC_DIV_VAL 0x7FFF

volatile uint8_t plc_rtc_failure = 0; //Not failed by default

uint32_t plc_rtc_clken_and_check(void)
{
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_BKP);

    return rcc_rtc_clock_enabled_flag();
}

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
static uint32_t dt_to_sec(tm *date_time)
{
    uint32_t ret;

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

void plc_rtc_init(tm* time)
{
    uint32_t i;

    start_flg = true;

    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_BKP);

    pwr_disable_backup_domain_write_protect();

    /* LSE oscillator clock used as the RTC clock */

    if (!rcc_rtc_clock_enabled_flag())
    {
        RCC_BDCR |= RCC_BDCR_LSEON;

        i = 0;
        while (0 == (RCC_BDCR & RCC_BDCR_LSERDY))
        {
            if (i++ > 1000000)
            {
                goto lse_error;
            }
        }
        //LSE is OK, may now set the clock
        RCC_BDCR &= ~((1 << 8) | (1 << 9));
        RCC_BDCR |= (1 << 8);

        rcc_enable_rtc_clock();
    }

    i = 0;
    while (0 == (RTC_CRL & RTC_CRL_RTOFF))
    {
        if (i++ > 1000000)
        {
            goto lse_error;
        }
    }
    /* Reset values + enter config mode*/
    RTC_CRL = RTC_CRL_CNF|RTC_CRL_RTOFF;
    RTC_CRH = 0;
    /* We have 32, 768KHz clock, set 0x7fff*/
    RTC_PRLL = PLC_RTC_DIV_VAL     & 0xffff;;
    RTC_PRLH = PLC_RTC_DIV_VAL>>16 & 0x000f;;
    //Get date and time, set counters.
    i = dt_to_sec(time);
    RTC_CNTH = (i >> 16) & 0xffff;
    RTC_CNTL =         i & 0xffff;

    RTC_ALRH = 0xFFFF;
    RTC_ALRL = 0xFFFF;

    RTC_CRL &= ~RTC_CRL_CNF;
    i = 0;
    while (0 == (RTC_CRL & RTC_CRL_RTOFF))
    {
        if (i++ > 1000000)
        {
            goto lse_error;
        }
    }
    /* Normal termination */
    pwr_enable_backup_domain_write_protect();
    return;
    /* LSE error occured */
lse_error:
    plc_diag_status |= PLC_DIAG_ERR_LSE;
    pwr_enable_backup_domain_write_protect();
}

void plc_rtc_dt_set(tm* time)
{
    uint32_t i;

    if (plc_diag_status & PLC_DIAG_ERR_LSE)
    {
        return;
    }

    pwr_disable_backup_domain_write_protect();


    /* Enter config mode */
    i = 0;
    while (0 == (RTC_CRL & RTC_CRL_RTOFF))
    {
        if (i++ > 1000000)
        {
            goto lse_error;
        }
    }
    RTC_CRL |= RTC_CRL_CNF;
    /*Set the clock*/
    if (time->tm_year < 2000)
    {
        time->tm_year += 2000;
    }
    i = dt_to_sec(time);

    PLC_DISABLE_INTERRUPTS();
    RTC_CNTH = (i >> 16) & 0xffff;
    RTC_CNTL =         i & 0xffff;
    start_flg = true;
    PLC_ENABLE_INTERRUPTS();

    /* Exit config mode */
    RTC_CRL &= ~RTC_CRL_CNF;
    i = 0;
    while (0 == (RTC_CRL & RTC_CRL_RTOFF))
    {
        if (i++ > 1000000)
        {
            goto lse_error;
        }
    }
    /* Normal termination */
    pwr_enable_backup_domain_write_protect();
    return;
    /* LSE error occured */
lse_error:
    plc_diag_status |= PLC_DIAG_ERR_LSE;
    pwr_enable_backup_domain_write_protect();
}

#define PLC_BASE_TICK_NS (100000)
#define PLC_RTC_CORR_THR (1200000000)
static int32_t plc_rtc_tick_ns;// Initial tick value is 100'000ns
static int64_t plc_rtc_correction;//Correction to RTC time

static uint32_t plc_rtc_cntr = 0;

//THis function is called every 100us in PLC_WAIT_TMR_ISR
void _plc_rtc_poll(void)
{
    static uint32_t last_sec = 0;

    uint32_t current_sec;

    current_sec = rtc_get_counter_val();

    if (start_flg)
    {
        start_flg = false;
        last_sec = current_sec;

        plc_rtc_cntr = current_sec;
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
            plc_rtc_cntr = current_sec;
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

void plc_rtc_dt_get(tm* time)
{
    sec_to_dt(plc_rtc_cntr, time);
}

void plc_rtc_time_get(IEC_TIME *current_time)
{
    int64_t rtc_corr;

    PLC_DISABLE_INTERRUPTS();

    current_time->tv_sec = plc_rtc_cntr;
    rtc_corr             = plc_rtc_correction;

    PLC_ENABLE_INTERRUPTS();

    current_time->tv_sec += (rtc_corr / 1000000000);
    current_time->tv_nsec = (rtc_corr % 1000000000);
}



