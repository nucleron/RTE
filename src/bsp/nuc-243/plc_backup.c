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

#define PLC_BKP_SIZE 2048 //2k bytes

//Use RTC backup registers for validation
#define PLC_BKP_BANK1_VER1   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER1_OFFSET)
#define PLC_BKP_BANK1_VER2   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER2_OFFSET)

#define PLC_BKP_BANK2_VER1   MMIO32(BACKUP_REGS_BASE + PLC_BKP_BANK2_VER1_OFFSET)
#define PLC_BKP_BANK2_VER2   MMIO32(BACKUP_REGS_BASE + PLC_BKP_BANK2_VER2_OFFSET)

#define PLC_BKP_BANK1_START (uint32_t *)(BKPSRAM_BASE)
#define PLC_BKP_BANK2_START (uint32_t *)(BKPSRAM_BASE+PLC_BKP_SIZE)

#define BANK1_VALID() ((PLC_BKP_BANK1_VER1)==(PLC_BKP_BANK1_VER2))
#define BANK2_VALID() ((PLC_BKP_BANK2_VER1)==(PLC_BKP_BANK2_VER2))

static uint32_t * plc_backup_reg_bank1 = PLC_BKP_BANK1_START;
static uint32_t * plc_backup_reg_bank2 = PLC_BKP_BANK2_START;

void plc_backup_init(void)
{
    uint32_t i;
    rcc_periph_clock_enable(RCC_PWR);
    BACKUP_UNLOCK();
    rcc_periph_clock_enable(RCC_BKPSRAM);
    //RCC_BDCR |= RCC_BDCR_LSEON;
    PWR_CSR |= PWR_CSR_BRE; //enable backup power regulator

    //Wait for backup regulator ready
     for(i=0; i<1000000; i++)
    {
        if( PWR_CSR  &  (1<<3) )
        {
            break;
        }
    }
    BACKUP_LOCK();
}

void plc_backup_invalidate(void)
{
    BACKUP_UNLOCK();
    if(PLC_BKP_BANK1_VER1>=PLC_BKP_BANK2_VER1) //invalidate oldest bank
    {
        PLC_BKP_BANK2_VER1 = PLC_BKP_BANK1_VER1+1;
    }
    else
    {
        PLC_BKP_BANK1_VER1 = PLC_BKP_BANK2_VER1+1;
    }
    BACKUP_LOCK();
}

void plc_backup_invalidate_all(void) //used to reset
{
    BACKUP_UNLOCK();

    PLC_BKP_BANK1_VER1 = 0;
    PLC_BKP_BANK2_VER1 = 0;

    PLC_BKP_BANK1_VER2 = 0xFF;
    PLC_BKP_BANK2_VER2 = 0xFF;

    BACKUP_LOCK();
}

void plc_backup_validate(void)
{
    uint32_t i;

    BACKUP_UNLOCK();

    if(PLC_BKP_BANK1_VER1>=PLC_BKP_BANK2_VER1) //validate latest bank
    {
        PLC_BKP_BANK1_VER2 = PLC_BKP_BANK1_VER1;
    }
    else
    {
        PLC_BKP_BANK2_VER2 = PLC_BKP_BANK2_VER1;
    }

    BACKUP_LOCK();
}

int plc_backup_check(void)
{
    if( BANK1_VALID() || BANK2_VALID() ) //At least one
        return 1; //Success, now may remind

    return 0; //Both banks are invalid, nothing to remind
}



void plc_backup_remind(unsigned int offset, unsigned int count, void *p)
{
    uint32_t* source;
    uint32_t i;
    if( BANK1_VALID() && BANK2_VALID()) //we must choose the latest bank if both are valid
    {
        if(PLC_BKP_BANK2_VER2 > PLC_BKP_BANK1_VER2)
            source = plc_backup_reg_bank2;
        else
            source = plc_backup_reg_bank1;
    }
    else if(BANK2_VALID()) //#2 is the only valid bank
    {
        source = plc_backup_reg_bank2;
    }
    else //should not get here if no banks are valid, so using #1
    {
        source = plc_backup_reg_bank1;
    }

    if(offset + count < PLC_BKP_SIZE)
    {
        memcpy( p, (void *)source + offset, count );
    }
}

void plc_backup_retain(unsigned int offset, unsigned int count, void *p)
{
    uint32_t* storage;
    if(offset + count < PLC_BKP_SIZE)
    {
        if(PLC_BKP_BANK1_VER1>=PLC_BKP_BANK2_VER1) //store to latest bank
        {
            storage = plc_backup_reg_bank1;
        }
        else
        {
            storage = plc_backup_reg_bank2;
        }

        BACKUP_UNLOCK();
        memcpy( (void *)storage + offset, p, count );
        BACKUP_LOCK();
    }
}
