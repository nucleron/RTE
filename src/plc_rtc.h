/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _RTC_H_
#define _RTC_H_

#include <iec_std_lib.h>

extern volatile uint8_t plc_rtc_failure;

//For internal usage
void _plc_rtc_poll(void);

uint32_t plc_rtc_clken_and_check(void);

void  plc_rtc_init(tm* time);
void  plc_rtc_dt_set(tm* time);
void  plc_rtc_dt_get(tm* time);
void  plc_rtc_time_get(IEC_TIME *curent_time);

#endif //_RTC_H_
