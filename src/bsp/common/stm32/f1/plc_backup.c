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
#define PLC_BKP_VER_1   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER1_OFFSET)
#define PLC_BKP_VER_2   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER2_OFFSET)
#define PLC_BKP_START_1 (uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_REG_OFFSET)
#define PLC_BKP_START_2 (uint32_t *)(BACKUP_REGS_BASE + PLC_BKP_REG_OFFSET + PLC_BKP_REG_NUM)

#define PLC_BKP_SIZE (PLC_BKP_REG_NUM*2ul) //Lower 16bits are used

static uint16_t plc_backup_buff[PLC_BKP_REG_NUM];
static uint32_t * plc_backup_reg_1 = PLC_BKP_START_1;
static uint32_t * plc_backup_reg_2 = PLC_BKP_START_2;

void plc_backup_init(void)
{
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_BKP);
}

void plc_backup_reset(void)
{
    pwr_disable_backup_domain_write_protect();
    PLC_BKP_VER_1=0;
    PLC_BKP_VER_2=0;
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
    //Write bank 1
    pwr_disable_backup_domain_write_protect();
    PLC_BKP_VER_1 &= 0xFFFFFFE; //Invalidate bank

    for(i = 0; i < PLC_BKP_REG_NUM; i++)
    {
        plc_backup_reg_1[i] = (uint32_t)plc_backup_buff[i];
    }

    PLC_BKP_VER_1 += 3;//Validate bank
    pwr_enable_backup_domain_write_protect();

    //Write bank 2
    pwr_disable_backup_domain_write_protect();
    PLC_BKP_VER_2 &= 0xFFFFFFE; //Invalidate bank

    for(i = 0; i < PLC_BKP_REG_NUM; i++)
    {
        plc_backup_reg_2[i] = (uint32_t)plc_backup_buff[i];
    }

    PLC_BKP_VER_2 += 3;//Validate bank
    pwr_enable_backup_domain_write_protect();
}

int plc_backup_check(void)
{
    uint32_t i;
    if ((PLC_BKP_VER_1 > PLC_BKP_VER_2) && (PLC_BKP_VER_1 & 0x1))
    {
        for(i = 0; i < PLC_BKP_REG_NUM; i++)
        {
            plc_backup_buff[i] = (uint16_t)(plc_backup_reg_1[i]&0xffff);
        }
        return 1;//Success!!!
    }

    if (PLC_BKP_VER_2 & 0x1)
    {
        for(i = 0; i < PLC_BKP_REG_NUM; i++)
        {
            plc_backup_buff[i] = (uint16_t)(plc_backup_reg_2[i]&0xffff);
        }
        return 1;//Success!!!
    }

    return 0; //Fail! Use dafaults!
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
