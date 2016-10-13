/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <plc_config.h>
#include <plc_tick.h>
#include <frac_div.h>
#include <plc_hw.h>

static bool systick_set_period(uint32_t period, uint32_t ahb, uint8_t clk_source)
{
    if (period >= (STK_RVR_RELOAD/ahb))
    {
        return false;
    }
    else
    {
        systick_set_clocksource( clk_source );
        systick_set_reload( ahb * period - 1 );
        return true;
    }
}

//Минимальный период в нС.
#define TICK_MIN_PER    100000ULL
//Пороговый период
#define TICK_THR_PER 500000000ULL
//Частота в МГц
#define RCC_AHB_FREQ    64UL

//Контроллер системного таймера.
static frac_div_t systick_ctrl;

//Состояние системного таймера
#define TICK_STATE_HIGH 0
#define TICK_STATE_MID  1
#define TICK_STATE_LOW  2
static uint32_t tick_state = TICK_STATE_HIGH;

volatile bool plc_tick_flag = false;

extern void plc_irq_stub(void);

extern bool plc_dbg_mode;
extern volatile uint32_t plc_hw_status;
void sys_tick_handler(void)
{
    //Soft watchdog
    if (plc_tick_flag)
    {
        if (plc_dbg_mode)
        {
            plc_hw_status |= PLC_HW_ERR_DEADLINE;
        }
        else
        {
            plc_irq_stub();
        }
    }

    switch (tick_state)
    {
    case TICK_STATE_MID:
    case TICK_STATE_HIGH:
    default:
        systick_set_reload(RCC_AHB_FREQ * (uint32_t)frac_div_icalc( &systick_ctrl ) - 1);
        plc_tick_flag = true;
        break;

    case TICK_STATE_LOW:
        plc_tick_flag = frac_div_run(&systick_ctrl);
        break;
    }
}

//Tick period in ns
void plc_tick_setup( uint64_t tick_next, uint64_t tick_period )
{
    (void)tick_next;//disable warning

    if (TICK_MIN_PER > tick_period)
    {
        tick_period = TICK_MIN_PER;
    }

    if (TICK_THR_PER > tick_period)
    {
        //Переменная частота работы системного таймера.
        if (systick_set_period((uint32_t)(tick_period/1000ULL), RCC_AHB_FREQ, STK_CSR_CLKSOURCE_AHB))
        {
            //Примерно до 0.1с
            tick_state = TICK_STATE_HIGH;
            frac_div_init(&systick_ctrl, tick_period, 1000ULL); //Коррекция периода
        }
        else
        {
            //До 0.5с
            tick_state = TICK_STATE_MID;
            systick_set_period((uint32_t)(tick_period/8000ULL), RCC_AHB_FREQ, STK_CSR_CLKSOURCE_AHB_DIV8);
            frac_div_init(&systick_ctrl, tick_period, 8000ULL); //Коррекция периода
        }
    }
    else
    {
        //Постоянная частота работы системного таймера.
        tick_state = TICK_STATE_LOW;
        systick_set_period(500, RCC_AHB_FREQ, STK_CSR_CLKSOURCE_AHB);//500мкс
        frac_div_init(&systick_ctrl, tick_period, 500000ULL);
    }

    systick_counter_enable();
    systick_interrupt_enable();
}
