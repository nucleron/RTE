/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _PLC_CLOCK_H_
#define _PLC_CLOCK_H_

extern volatile uint8_t plc_clock_hse_failure;
void plc_clock_setup(void);

#endif /* _PLC_CLOCK_H_ */
