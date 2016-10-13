/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/f1/bkp.h>

#include <plc_config.h>
#include <plc_backup.h>
#include <plc_hw.h>

//Use RTC backup registers for validation
#define PLC_BKP_VER1   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER1_OFFSET)
#define PLC_BKP_VER2   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER2_OFFSET)
#define PLC_BKP_START (uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_REG_OFFSET)

#define PLC_BKP_SIZE (PLC_BKP_REG_NUM*2ul) //Lowe 16bits are used

static uint16_t plc_backup_buff[PLC_BKP_REG_NUM];
static uint32_t * plc_backup_reg = PLC_BKP_START;

void plc_backup_init(void)
{
    uint32_t i;
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_BKP);

    for(i = 0; i < PLC_BKP_REG_NUM; i++)
    {
        plc_backup_buff[i] = (uint16_t)(plc_backup_reg[i]&0xffff);
    }
}

void plc_backup_reset(void)
{
    pwr_disable_backup_domain_write_protect();
    PLC_BKP_VER1++;
    pwr_enable_backup_domain_write_protect();
}

void plc_backup_invalidate(void)
{
    //pwr_disable_backup_domain_write_protect();
    //PLC_BKP_VER1++;
    //pwr_enable_backup_domain_write_protect();
}

void plc_backup_validate(void)
{
    uint32_t i;

    pwr_disable_backup_domain_write_protect();
    PLC_BKP_VER1++;

    for(i = 0; i < PLC_BKP_REG_NUM; i++)
    {
        plc_backup_reg[i] = (uint32_t)plc_backup_buff[i];
    }

    PLC_BKP_VER2 = PLC_BKP_VER1;
    pwr_enable_backup_domain_write_protect();
}

int plc_backup_check(void)
{
    if (PLC_BKP_VER1 != PLC_BKP_VER2)
    {
        return 0;
    }
    else
    {
        return 1; //Success, now may remind
    }
}



void plc_backup_remind(unsigned int offset, unsigned int count, void *p)
{
    if(offset + count < PLC_BKP_SIZE)
    {
        memcpy( p, (void *)plc_backup_buff + offset, count );
    }
}

void plc_backup_retain(unsigned int offset, unsigned int count, void *p)
{
    if(offset + count < PLC_BKP_SIZE)
    {
        //pwr_disable_backup_domain_write_protect();
        memcpy( (void *)plc_backup_buff + offset, p, count );
        //pwr_enable_backup_domain_write_protect();
    }
}
