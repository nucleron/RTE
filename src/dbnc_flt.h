/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _DBNC_FLT_H_
#define _DBNC_FLT_H_

#include "stdbool.h"
#include "stdint.h"

typedef struct _dbnc_flt_t dbnc_flt_t;
struct _dbnc_flt_t
{
    uint32_t thr_on;
    uint32_t tim_on;
    uint32_t thr_off;
    uint32_t tim_off;
    bool    flg;
};

/* Default threasholds */
#define DBNC_FLT_THR_ON  2
#define DBNC_FLT_THR_OFF 10

void dbnc_flt_init(dbnc_flt_t * self);
void dbnc_flt_poll(dbnc_flt_t * self, uint32_t tick, bool in);
bool dbnc_flt_get(dbnc_flt_t * self);

#endif // _DBNC_FLT_H_
