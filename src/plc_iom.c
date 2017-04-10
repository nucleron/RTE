/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_iom.h>

plc_iom_t plc_iom;

#ifndef PLC_APP
#   define PLC_APP plc_curr_app
#endif

/*Insertion sort*/
static void _plc_loc_sort( uint16_t start, uint16_t end )
{
    int32_t       i, j;
    uint32_t      p_weigth;
    plc_loc_tbl_t p_loc;

    for (i=start; i < end; i++)
    {
        p_weigth = PLC_APP->w_tab[i];
        p_loc    = PLC_APP->l_tab[i];

        for (j = i-1; (j >= start) && (PLC_APP->w_tab[j] > p_weigth); j--)
        {
            PLC_APP->w_tab[j+1] = PLC_APP->w_tab[j];
            PLC_APP->l_tab[j+1] = PLC_APP->l_tab[j];
        }
        PLC_APP->w_tab[++j] = p_weigth;
        PLC_APP->l_tab[ j ] = p_loc;
    }
}
//Sort locations with the same protocol used
static uint32_t _plc_proto_sort( uint16_t l_begin, uint16_t l_end )
{
    int32_t i;
    uint16_t proto;
    uint8_t j;

    proto = PLC_APP->l_tab[l_begin]->proto;

    for (i = l_begin+1; i < l_end; i++)
    {
        if (proto != PLC_APP->l_tab[i]->proto)
        {
            l_end = i;
            break;
        }
    }

    _plc_loc_sort(l_begin, l_end);
    //Register location chunk with the same proto
    j = mid_from_pid(proto);
    plc_iom_registry[j].begin(l_begin);
    plc_iom_registry[j].end(l_end);

    return l_end;
}
//Sort locations of the same type
static void _plc_type_sort( uint16_t l_begin, uint16_t l_end )
{
    while(l_begin < l_end)
    {
        l_begin = _plc_proto_sort(l_begin, l_end);
    }
}

void plc_iom_init(void)
{
    int32_t i;

    plc_iom.m_begin = 0;
    plc_iom.m_end   = 0;
    plc_iom.tflg    = false;
    plc_iom.tick    = 0;

    for (i = 0; i < plc_iom_reg_sz; i++)
    {
        plc_iom_registry[i].init();
    }
}

void plc_iom_tick(void)
{
    plc_iom.tflg = true;
    plc_iom.tick++;
}

bool plc_iom_test_hw(void)
{
    uint8_t j;
    for (j = 0; j < plc_iom_reg_sz; j++)
    {
        if (false == plc_iom_registry[j].test_hw())
        {
            return false;
        }
    }
    return true;
}

static char print_buf[128];
static const char print_lt[] = {'I','M','Q'};
static const char print_sz[] = {'X','B','W','D','L'};

void plc_iom_check_print(uint16_t i)
{
    int cnt;
    cnt = sprintf(
              print_buf,
              "Checking: %%%c%c%d",
              print_lt[PLC_APP->l_tab[i]->v_type],
              print_sz[PLC_APP->l_tab[i]->v_size],
              (int)PLC_APP->l_tab[i]->proto
          );
    if (PLC_APP->l_tab[i]->a_size)
    {
        uint16_t j;
        for (j = 0; j < PLC_APP->l_tab[i]->a_size; j++)
        {
            cnt += sprintf( print_buf + cnt, ".%d", (int)PLC_APP->l_tab[i]->a_data[j] );
            //Overflow check
            if (cnt >= 115)
            {
                //Maximum print length is 12bytes
                print_buf[115] = 0;
                cnt = 114;
                break;
            }
        }
    }
    //Must use default app here
    plc_curr_app->log_msg_post(LOG_DEBUG, (char *)print_buf, cnt+1);
    PLC_APP->log_msg_post(LOG_DEBUG, (char *)print_buf, cnt+1);
}

void plc_iom_errno_print(uint16_t errno)
{
    int cnt;
    cnt = sprintf(print_buf, "Error: %d!", errno);
    plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)print_buf, cnt+1);
}

static const char plc_iom_err_proto[]   = "IO protocol is not supported!";

bool plc_iom_check_and_sort(void)
{
    int32_t i, o_end, w;
    uint8_t j;

    o_end   = PLC_APP->l_sz;

    for (i = 0; i < o_end; i++)
    {
        plc_iom_check_print(i);

        j = mid_from_pid( PLC_APP->l_tab[i]->proto );
        //Check protocol
        if (PLC_IOM_MID_ERROR == j)
        {
            PLC_LOG_ERROR(plc_iom_err_proto);
            return false;
        }
        //Weigth location
        w = PLC_APP->l_tab[i]->v_type;
        w <<= 8;
        w |= j;

        PLC_APP->w_tab[i] = w;
    }
    //Sort locations
    _plc_loc_sort(0, o_end);

    //Weigth locations for protocol specific sort
    for (i = 0; i < o_end; i++)
    {
        j = mid_from_pid( PLC_APP->l_tab[i]->proto );
        //Check location, plc_iom_registry[j].check(i) must print all error messages!
        if (false == plc_iom_registry[j].check(i))
        {
            return false;
        }
        PLC_APP->w_tab[i] = plc_iom_registry[j].weigth(i);
    }
    //Set plc_iom.m_begin
    for (i = 0; i < o_end; i++)
    {
        if (PLC_LT_I != PLC_APP->l_tab[i]->v_type)
        {
            plc_iom.m_begin = i;
            break;
        }
    }
    //Set plc_iom.m_end
    for (i = plc_iom.m_begin; i < o_end; i++)
    {
        if (PLC_LT_M != PLC_APP->l_tab[i]->v_type)
        {
            plc_iom.m_end = i;
            break;
        }
    }

    // Protocol specific sort, each type of locations is sorted separately
    _plc_type_sort( 0,               plc_iom.m_begin );//inputs
    _plc_type_sort( plc_iom.m_begin, plc_iom.m_end   );//memory
    _plc_type_sort( plc_iom.m_end,   o_end           );//outputs
    // Clear weigths
    for (i = 0; i < o_end; i++)
    {
        PLC_APP->w_tab[i] = 0;
    }

    return true;
}

void plc_iom_get(void)
{
    int32_t i, m_end;
    uint8_t j;

    m_end = plc_iom.m_end;

    for (i = 0; i < m_end; i++)
    {
        j = mid_from_pid( plc_curr_app->l_tab[i]->proto );
        plc_curr_app->w_tab[i] += plc_iom_registry[j].get(i);
    }
}

void plc_iom_set(void)
{
    int32_t i, m_begin, o_end;
    uint8_t j;

    m_begin = plc_iom.m_begin;
    o_end   = plc_curr_app->l_sz;

    for (i = m_begin; i < o_end; i++)
    {
        j = mid_from_pid( plc_curr_app->l_tab[i]->proto );
        plc_curr_app->w_tab[i] += plc_iom_registry[j].set(i);
    }
}

void plc_iom_start(void)
{
    uint8_t j;
    for (j = 0; j < plc_iom_reg_sz; j++)
    {
        plc_iom_registry[j].start();
    }
}

void plc_iom_poll(void)
{
    int32_t i, o_end, tick;
    uint8_t j;

    PLC_DISABLE_INTERRUPTS();
    tick = plc_iom.tick;

    if (plc_iom.tflg)
    {
        plc_iom.tflg = false;
        PLC_ENABLE_INTERRUPTS();
        /*I/O scheduling is done on tick*/
        o_end = plc_curr_app->l_sz;

        for (i = 0; i < o_end; i++)
        {
            j = mid_from_pid( plc_curr_app->l_tab[i]->proto );
            plc_curr_app->w_tab[i] += plc_iom_registry[j].sched(i, tick);
        }
    }
    else
    {
        PLC_ENABLE_INTERRUPTS();
    }
    /*I/O polling is done always*/
    for (j = 0; j < plc_iom_reg_sz; j++)
    {
        plc_iom_registry[j].poll(tick);
    }
}

void plc_iom_stop(void)
{
    uint8_t j;
    for (j = 0; j < plc_iom_reg_sz; j++)
    {
        plc_iom_registry[j].stop();
    }
}

const char     plc_iom_err_sz[]    = "Wrong variable size!";
const uint32_t plc_iom_err_sz_sz   = sizeof(plc_iom_err_sz);
