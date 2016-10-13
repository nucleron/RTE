/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#ifndef _PLC_TICK_H_
#define _PLC_TICK_H_

#include <stdbool.h>

extern volatile bool plc_tick_flag;

void plc_tick_setup( unsigned long long tick_next, unsigned long long tick_period );

#endif /* _PLC_TICK_H_ */
