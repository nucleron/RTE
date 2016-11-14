/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "xprintf.h"
/* Portions of standart library to run Linker with -nostdlib -lgcc */
void *memset(void *dest, int c, size_t count)
{
    char * d;

    c &= 0xff;
    d = (char *)dest;

    while (count--)
    {
        *d++ = (char)c;
    }
    return (void *)d;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    char *d, *s;

    d = (char *)dest;
    s = (char *)src;

    while (count--)
    {
        *d++ = *s++;
    }
    return (void *)d;
}

static char * out_ptr;
static uint8_t out_cnt = 0;
static void out_stub(unsigned char c)
{
    out_ptr[out_cnt++] = c;
}


int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;

    out_ptr = buf;
    out_cnt = 0;
    xdev_out(out_stub);

    va_start(args, fmt);
    xvprintf(fmt, args);
    va_end(args);

    xdev_out(0);
    buf[out_cnt] = 0;//Terminate string.

    return out_cnt;
}

