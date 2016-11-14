/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include "dbnc_flt.h"

void dbnc_flt_init(dbnc_flt_t * self)
{
    self->thr_on  = DBNC_FLT_THR_ON;
    self->tim_on  = 0;
    self->thr_off = DBNC_FLT_THR_OFF;
    self->tim_off = 0;
    self->flg = false;
}

void dbnc_flt_poll(dbnc_flt_t * self, uint32_t tick, bool in)
{
    if (self->flg)
    {
        if (in)
        {
            self->tim_off = tick;
        }
        else
        {
            if ((tick - self->tim_off) > self->thr_off)
            {
                self->flg = false;
                self->tim_on = tick;
            }
        }
    }
    else
    {
        if (in)
        {
            if ((tick - self->tim_on) > self->thr_on)
            {
                self->flg = true;
                self->tim_off = tick;
            }
        }
        else
        {
            self->tim_on = tick;
        }
    }
}

bool dbnc_flt_get(dbnc_flt_t * self)
{
    return self->flg;
}
