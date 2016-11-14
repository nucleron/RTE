/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdint.h>
#include <stdbool.h>

#include <noise_flt.h>

void noise_flt_init(noise_flt_t * self, uint8_t depth, uint16_t val)
{
    uint8_t i;
    for (i=0; i<NOISE_FLT_BUFSZ; i++)
    {
        self->data[i] = val;
    }
    self->depth = depth;
    self->idx = 0;
}


void noise_flt_write(noise_flt_t * self, uint16_t val)
{
    self->data[self->idx++] = val;
    self->idx %= self->depth;
}

// C.A.R. Hoare's Quick Median
static inline uint8_t quick_median(uint16_t *data, uint8_t n)
{
    uint8_t l = 0;
    uint8_t r = n-1;
    uint8_t k = n/2;

    while (l < r)
    {
        uint16_t x = data[k];
        uint8_t i = l;
        uint8_t j = r;
        //Split dada
        do
        {
            while (data[i] < x)
            {
                i++;
            }
            while (x < data[j])
            {
                j--;
            }
            if (i <= j)
            {
                uint16_t ii = data[i];
                data[i] = data[j];
                data[j] = ii;
                i++;
                j--;
            }
        }
        while (i <= j);

        if (j < k)
        {
            l = i;
        }
        if (k < i)
        {
            r = j;
        }
    }
    return k;
}

uint16_t noise_flt_median(noise_flt_t * self, uint16_t * buf)
{
    uint8_t  i;
    uint8_t n = self->depth;
    //Copy data to buf
    for(i=0; i<n; i++)
    {
        buf[i] = self->data[i];
    }

    i = quick_median(buf, n);

    return buf[i];
}

uint16_t noise_flt_ave(noise_flt_t * self)
{
    uint8_t  i;
    uint8_t n;
    uint32_t s = 0;

    n = self->depth;
    for(i=0; i<n; i++)
    {
        s += self->data[i];
    }
    return (uint16_t)(s/n);
}
