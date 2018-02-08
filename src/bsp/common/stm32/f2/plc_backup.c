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
#include <libopencm3/stm32/f1/bkp.h>

#include <plc_config.h>
#include <plc_backup.h>
#include <plc_hw.h>

#define PLC_BKP_SIZE 2048 //2k bytes
//Use RTC backup registers for validation
#define PLC_BKP_VER_1   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER1_OFFSET)
#define PLC_BKP_VER_2   MMIO32(BACKUP_REGS_BASE + PLC_BKP_VER2_OFFSET)

#define PLC_BKP_BRIGHT       MMIO32(BACKUP_REGS_BASE + PLC_BKP_BRIGHT_OFFSET)

#define PLC_BKP_BANK1_START (uint32_t *)(BKPSRAM_BASE)
#define PLC_BKP_BANK2_START (uint32_t *)(BKPSRAM_BASE+PLC_BKP_SIZE)

#define BACKUP_UNLOCK() PWR_CR |= PWR_CR_DBP
#define BACKUP_LOCK() PWR_CR &= ~PWR_CR_DBP

#define PLC_BKP_VER_MSK (0xFFFFFFFE)
#define PLC_BKP_VLD_MSK (0x01)

#define  PLC_BKP_GET_VER(ver) ((ver) & PLC_BKP_VER_MSK)
#define PLC_BKP_IS_VALID(ver) ((ver) & PLC_BKP_VLD_MSK)

/*If true, then read from v1-bank, write to v2-bank, else do vice versa.*/
#define  PLC_BKP_TEST_VER(v1, v2) (PLC_BKP_GET_VER(v1) != PLC_BKP_GET_VER(v2))

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
        if (PWR_CSR  &  (1<<3))
        {
            break;
        }
    }
    BACKUP_LOCK();
}

void plc_backup_save_brightness(uint8_t data)
{
    BACKUP_UNLOCK();
    PLC_BKP_BRIGHT = data;
    BACKUP_LOCK();
}

uint8_t plc_backup_load_brightness(void)
{
    return PLC_BKP_BRIGHT;
}

void plc_backup_invalidate(void)
{
    BACKUP_UNLOCK();
    if (PLC_BKP_TEST_VER(PLC_BKP_VER_1, PLC_BKP_VER_2))
    {
        /*Will write BANK_2*/
        PLC_BKP_VER_2 &= PLC_BKP_VER_MSK;
    }
    else
    {
        /*Will write BANK_1*/
        PLC_BKP_VER_1 &= PLC_BKP_VER_MSK;
    }
    BACKUP_LOCK();
}

void plc_backup_reset(void) //used to reset
{
    BACKUP_UNLOCK();

    PLC_BKP_VER_1 = 0;
    PLC_BKP_VER_2 = 0;

    BACKUP_LOCK();
}

void plc_backup_validate(void)
{
//    uint32_t i;

    BACKUP_UNLOCK();

    if (PLC_BKP_TEST_VER(PLC_BKP_VER_1, PLC_BKP_VER_2))
    {
        /*BANK_2 was written.*/
        PLC_BKP_VER_2 += 3;
    }
    else
    {
        /*BANK_1 was written.*/
        PLC_BKP_VER_1 += 3;
    }

    BACKUP_LOCK();
}

int plc_backup_check(void)
{
    if (PLC_BKP_IS_VALID(PLC_BKP_VER_1))
    {
        /*BANK_1 is valid*/
        return 1;
    }

    if (PLC_BKP_IS_VALID(PLC_BKP_VER_2))
    {
        /*BANK_2 is valid*/
        return 1;
    }

    return 0; //Both banks are invalid, nothing to remind
}



void plc_backup_remind(unsigned int offset, unsigned int count, void *p)
{
    uint32_t* source = PLC_BKP_BANK1_START;
    /*We have at least one valid bank here*/
    if (PLC_BKP_TEST_VER(PLC_BKP_VER_1, PLC_BKP_VER_2))
    {
        //Try to remind from bank 1 as it was written later
        if (PLC_BKP_IS_VALID(PLC_BKP_VER_1))
        {
            source = PLC_BKP_BANK1_START;
        }
    }
    else
    {
        //Else - try to remind from bank 2
        if (PLC_BKP_IS_VALID(PLC_BKP_VER_2))
        {
            source = PLC_BKP_BANK2_START;
        }
    }

    if (offset + count < PLC_BKP_SIZE)
    {
        memcpy(p, (void *)source + offset, count);
    }
}

void plc_backup_retain(unsigned int offset, unsigned int count, void *p)
{
    uint32_t* storage;
    if (offset + count < PLC_BKP_SIZE)
    {
        if (PLC_BKP_TEST_VER(PLC_BKP_VER_1, PLC_BKP_VER_2))
        {
            /*Will write to bank 2 as bank 1 has later version of data.*/
            storage = PLC_BKP_BANK2_START;
        }
        else
        {
            /*Will write to bank 1 as it has older data version*/
            storage = PLC_BKP_BANK1_START;
        }

        BACKUP_UNLOCK();
        memcpy((void *)storage + offset, p, count);
        BACKUP_LOCK();
    }
}
