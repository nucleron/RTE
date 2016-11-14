/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */
#ifndef _NOISE_FLT_H_
#define _NOISE_FLT_H_

#ifndef NOISE_FLT_BUFSZ
#define NOISE_FLT_BUFSZ 8
#endif//NOISE_FLT_BUFSZ

typedef struct
{
    volatile uint16_t data[NOISE_FLT_BUFSZ];
    uint8_t  depth;
    volatile uint8_t  idx;
}
noise_flt_t;

void noise_flt_init(noise_flt_t * self, uint8_t depth, uint16_t val);

void noise_flt_write(noise_flt_t * self, uint16_t val);

uint16_t noise_flt_median(noise_flt_t * self, uint16_t * buf);
uint16_t noise_flt_ave(noise_flt_t * self);

#endif /* _NOISE_FLT_H_ */
