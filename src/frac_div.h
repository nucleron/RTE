/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _FRAC_DIV_H_
#define _FRAC_DIV_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint64_t idefault; //!< Default intger part of period
    uint64_t icurrent; //!< Current intger part of period
    uint64_t iaccum;   //!< Integer part accumulator
    uint64_t fdelta;   //!< Fractionsl part delta
    uint64_t faccum;   //!< Fractional part accumulator
    uint64_t base;     //!< Base
}
frac_div_t;//!< Fractional frequency divider

void frac_div_init(frac_div_t * div, uint64_t period, uint64_t base);
void frac_div_set(frac_div_t * div, uint64_t period);
uint64_t frac_div_icalc(frac_div_t * div); //may be used to correct systick reload value during the work
bool frac_div_run(frac_div_t * div);

#endif // _FRAC_DIV_H_
