/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include "frac_div.h"

void frac_div_set(frac_div_t * div, uint64_t period)
{
    div->idefault = period / div->base;
    div->fdelta = period % div->base;
}

void frac_div_init(frac_div_t * div, uint64_t period, uint64_t base)
{
    div->base = base;

    frac_div_set(div, period);

    div->icurrent = div->idefault;
    div->iaccum = 0;
    div->faccum = div->fdelta;
}

uint64_t frac_div_icalc(frac_div_t * div)
{
    div->faccum += div->fdelta;
    if (div->base > div->faccum)
    {
        return div->idefault;
    }
    else
    {
        div->faccum %= div->base;
        return div->idefault + 1;
    }
}

bool frac_div_run(frac_div_t * div)
{
    if (div->iaccum < div->icurrent)
    {
        div->iaccum++;
        return false;
    }
    else
    {
        div->iaccum = 1;
        div->icurrent = frac_div_icalc(div);
        return true;
    }
}
