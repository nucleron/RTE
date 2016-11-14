/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>

#include <plc_backup.h>
#include <plc_hw.h>

#include <plc_config.h>

//#define PLC_BKP_SIZE 4096

#define PLC_UVP(a) ((void *)(a))
//Use RTC backup registers for validation
#define PLC_BKP_VER1   MMIO32(RTC_BKP_BASE + PLC_BKP_VER1_OFFSET)
#define PLC_BKP_VER2   MMIO32(RTC_BKP_BASE + PLC_BKP_VER2_OFFSET)
//Use BackupSRAM to store retain vars
//#define PLC_BKP_START PLC_UVP(BKPSRAM_BASE)
#define PLC_BKP_START (uint32_t *)(RTC_BKP_BASE + PLC_BKP_REG_OFFSET)


//watches RTC_BKP_BASE 0x40002850
//  PLC_BKP_START 0x40024000

#define PLC_BKP_REG_NUM 4
#define PLC_BKP_SIZE (PLC_BKP_REG_NUM*4ul)
static uint32_t plc_backup_buff[PLC_BKP_REG_NUM];
static uint32_t * plc_backup_reg = PLC_BKP_START;

void plc_backup_init(void)
{
    uint32_t i;
    rcc_periph_clock_enable( RCC_PWR );
    pwr_disable_backup_domain_write_protect();

    PWR_CSR |= PWR_CSR_BRE;             // Backup regulator enable
    while( 0 == ( PWR_CSR & PWR_CSR_BRR ) );// Wait for backup regulator ready

    pwr_enable_backup_domain_write_protect();

    //rcc_periph_clock_enable( RCC_BKPSRAM );
    for(i = 0; i < PLC_BKP_REG_NUM; i++)
    {
        plc_backup_buff[i] = plc_backup_reg[i];
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
        plc_backup_reg[i] = plc_backup_buff[i];
    }

    PLC_BKP_VER2 = PLC_BKP_VER1;
    pwr_enable_backup_domain_write_protect();
}

int plc_backup_check(void)
{
    if( PLC_BKP_VER1 != PLC_BKP_VER2 )
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
