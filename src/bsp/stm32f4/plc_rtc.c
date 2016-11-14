/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>

#include <plc_rtc.h>
#include <plc_hw.h>
#include <plc_config.h>

#define PLC_BKP_RTC_IS_OK   MMIO32(RTC_BKP_BASE + PLC_BKP_RTC_IS_OK_OFFSET)

uint32_t plc_rtc_is_ok(void)
{
    return PLC_BKP_RTC_IS_OK;
}

void plc_rtc_init( tm* time )
{
    uint32_t tmp=0;
    uint32_t year;
    uint32_t i;

    rcc_periph_clock_enable( RCC_PWR );

    pwr_disable_backup_domain_write_protect();

    PLC_BKP_RTC_IS_OK = 0;

    /* LSE oscillator clock used as the RTC clock */
    RCC_BDCR |= 0x00000100;
    RCC_BDCR |= RCC_BDCR_LSEON;

    rcc_periph_clock_enable( RCC_RTC );

    rtc_unlock();

    RTC_ISR |= RTC_ISR_INIT;
    for(i=0; i<1000000; i++)
    {
        if( RTC_ISR & RTC_ISR_INITF )
        {
            break;
        }
    }

    if( !(RTC_ISR & RTC_ISR_INITF) )
    {
        plc_hw_status |= PLC_HW_ERR_LSE;

        RTC_ISR &= ~RTC_ISR_INIT;
        pwr_enable_backup_domain_write_protect();
        return;
    }

    rtc_set_prescaler( 0x1FF, 0x3F );

    tmp  =  (unsigned long)(time->tm_sec%10 );
    tmp |= ((unsigned long)(time->tm_sec/10 )) <<  4;
    tmp |= ((unsigned long)(time->tm_min%10 )) <<  8;
    tmp |= ((unsigned long)(time->tm_min/10 )) << 12;
    tmp |= ((unsigned long)(time->tm_hour%10)) << 16;
    tmp |= ((unsigned long)(time->tm_hour/10)) << 20;
    RTC_TR = tmp;

    /* Only 2 digits used!!! *time may be const, so use year var. */
    year = time->tm_year % 100;

    tmp  =  (unsigned long)(time->tm_day%10 );
    tmp |= ((unsigned long)(time->tm_day/10 )) <<  4;
    tmp |= ((unsigned long)(time->tm_mon%10 )) <<  8;
    tmp |= ((unsigned long)(time->tm_mon/10 )) << 12;
    tmp |= ((unsigned long)(year%10)) << 16;
    tmp |= ((unsigned long)(year/10)) << 20;
    RTC_DR = tmp;

    /* exit from initialization mode */
    RTC_ISR &= ~RTC_ISR_INIT;

    PLC_BKP_RTC_IS_OK = 1;

    pwr_enable_backup_domain_write_protect();
}

/*
 *  Julian Day Number computation
 *  See http://en.wikipedia.org/wiki/Julian_day
 */
static uint32_t jdn( tm *date_time )
{
    uint8_t a,m;
    uint16_t y;
    uint32_t ret;

    /* Compute helper factors */
    a = (14 - (uint8_t)date_time->tm_mon)/12;
    m = (uint8_t)date_time->tm_mon + 12*a - 3;
    y = (uint16_t)date_time->tm_year + 4800 - a;

    /* Julian day number computation */
    ret = (uint32_t)date_time->tm_day;
    ret += ( 153ul * (uint32_t)m + 2ul )/ 5ul;
    ret += 365ul * (uint32_t)y;
    ret += (uint32_t)( y/4   );
    ret -= (uint32_t)( y/100 );
    ret += (uint32_t)( y/400 );
    ret -= 32045ul;

    return ret;
}

/*
 * Compute unix-time seconds
 */
static uint32_t dt_to_sec( tm *date_time )
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

///WARNING, this function is not reentrant!!!
void plc_rtc_time_get( IEC_TIME *current_time )
{
    static tm curr;
    static uint32_t rtc_ssr, rtc_tr, rtc_dr, rtc_prediv_s;

    /* Read regs AQAP */
    rtc_ssr = RTC_SSR;
    rtc_tr  = RTC_TR;
    rtc_dr  = RTC_DR;

    curr.tm_sec   = (unsigned char)(  rtc_tr & 0x0000000F);
    curr.tm_sec  += (unsigned char)(((rtc_tr & 0x000000F0) >> 4 ) * 10 );
    curr.tm_min   = (unsigned char)( (rtc_tr & 0x00000F00) >> 8);
    curr.tm_min  += (unsigned char)(((rtc_tr & 0x0000F000) >> 12 ) * 10 );
    curr.tm_hour  = (unsigned char)( (rtc_tr & 0x000F0000) >> 16);
    curr.tm_hour += (unsigned char)(((rtc_tr & 0x00F00000) >> 20 ) * 10 );

    curr.tm_day   = (unsigned char)(  rtc_dr & 0x0000000F);
    curr.tm_day  += (unsigned char)(((rtc_dr & 0x00000030) >> 4 ) * 10 );
    curr.tm_mon   = (unsigned char)( (rtc_dr & 0x00000F00) >> 8);
    curr.tm_mon  += (unsigned char)(((rtc_dr & 0x00001000) >> 12 ) * 10 );
    curr.tm_year  = (unsigned char)( (rtc_dr & 0x000F0000) >> 16);
    curr.tm_year += (unsigned char)(((rtc_dr & 0x00F00000) >> 20 ) * 10 );
    curr.tm_year += 2000; // 16 is actually 2016

    /* Convert current date/time to unix time seconds */
    current_time->tv_sec = dt_to_sec( &curr );

    /* Add subseconds! */
    rtc_prediv_s = RTC_PRER & 0x7FFF;
    /* Check for time shift... */
    if( rtc_ssr > rtc_prediv_s )
    {
        /* This is NOT normal RTC operation!!! */
        current_time->tv_sec--;
        current_time->tv_nsec = 0;
        return;
    }
    else
    {
        current_time->tv_nsec = (((rtc_prediv_s - rtc_ssr)*10000ul)/(rtc_prediv_s + 1))*100000ul;
    }
}



