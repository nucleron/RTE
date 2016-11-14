/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <stdint.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>

#include <plc_config.h>

void plc_diag_reset(void)
{
    uint32_t i;

    rcc_periph_clock_enable(RCC_PWR);
//    rcc_periph_clock_enable(RCC_BKP);

    rtc_unlock();
    for (i=0; i<6; i++)
    {
        PLC_DIAG_IRQS[i] = 0;
    }
    rtc_lock();
}

static uint32_t read_irq(void)
{
    uint32_t ret=0;
    __asm__ __volatile__ ("mrs %0, ipsr\n\t" : "=r" (ret) );
    return(ret & 0x1FFul);
}

//If unhandled interrupt happens, then we must remember the vector and reset the system!
void plc_irq_stub(void)
{
    uint32_t irq,i;

    irq = read_irq();
    if (irq > PLC_DIAG_INUM)
    {
        irq = PLC_DIAG_INUM;
    }

    i = irq/16;
    irq %= 16;

    rcc_periph_clock_enable(RCC_PWR);
    //rcc_periph_clock_enable(RCC_BKP);

    rtc_unlock();

    PLC_DIAG_IRQS[i] |= (1ul<<irq);

    rtc_lock();

    scb_reset_system();
}

#define PLC_ISR_STUB(f) void f() __attribute__ ((alias ("plc_irq_stub")))

//PLC_ISR_STUB( nmi_handler        ); USED!!!
PLC_ISR_STUB( hard_fault_handler );
PLC_ISR_STUB( sv_call_handler    );
PLC_ISR_STUB( pend_sv_handler    );
//PLC_ISR_STUB( sys_tick_handler   ); USED!!!

/* Those are defined only on CM3 or CM4 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
PLC_ISR_STUB( mem_manage_handler    );
PLC_ISR_STUB( bus_fault_handler     );
PLC_ISR_STUB( usage_fault_handler   );
//PLC_ISR_STUB( debug_monitor_handler ); DO I NEED THIS???
#endif

PLC_ISR_STUB(nvic_wwdg_isr         );
PLC_ISR_STUB(pvd_isr               );
PLC_ISR_STUB(tamp_stamp_isr        );
PLC_ISR_STUB(rtc_wkup_isr          );
PLC_ISR_STUB(flash_isr             );
PLC_ISR_STUB(rcc_isr               );
PLC_ISR_STUB(exti0_isr             );
PLC_ISR_STUB(exti1_isr             );
PLC_ISR_STUB(exti2_isr             );
PLC_ISR_STUB(exti3_isr             );
PLC_ISR_STUB(exti4_isr             );
PLC_ISR_STUB(dma1_stream0_isr      );
PLC_ISR_STUB(dma1_stream1_isr      );
PLC_ISR_STUB(dma1_stream2_isr      );
PLC_ISR_STUB(dma1_stream3_isr      );
PLC_ISR_STUB(dma1_stream4_isr      );
PLC_ISR_STUB(dma1_stream5_isr      );
PLC_ISR_STUB(dma1_stream6_isr      );
PLC_ISR_STUB(adc_isr               );
PLC_ISR_STUB(can1_tx_isr           );
PLC_ISR_STUB(can1_rx0_isr          );
PLC_ISR_STUB(can1_rx1_isr          );
PLC_ISR_STUB(can1_sce_isr          );
PLC_ISR_STUB(exti9_5_isr           );
PLC_ISR_STUB(tim1_brk_tim9_isr     );
PLC_ISR_STUB(tim1_up_tim10_isr     );
PLC_ISR_STUB(tim1_trg_com_tim11_isr);
PLC_ISR_STUB(tim1_cc_isr           );
PLC_ISR_STUB(tim2_isr              );
PLC_ISR_STUB(tim3_isr              );
PLC_ISR_STUB(tim4_isr              );
PLC_ISR_STUB(i2c1_ev_isr           );
PLC_ISR_STUB(i2c1_er_isr           );
PLC_ISR_STUB(i2c2_ev_isr           );
PLC_ISR_STUB(i2c2_er_isr           );
PLC_ISR_STUB(spi1_isr              );
PLC_ISR_STUB(spi2_isr              );
//PLC_ISR_STUB(usart1_isr            ); USED!!!
//PLC_ISR_STUB(usart2_isr            ); USED!!!
//PLC_ISR_STUB(usart3_isr            ); USED!!!
PLC_ISR_STUB(exti15_10_isr         );
PLC_ISR_STUB(rtc_alarm_isr         );
PLC_ISR_STUB(usb_fs_wkup_isr       );
PLC_ISR_STUB(tim8_brk_tim12_isr    );
PLC_ISR_STUB(tim8_up_tim13_isr     );
PLC_ISR_STUB(tim8_trg_com_tim14_isr);
PLC_ISR_STUB(tim8_cc_isr           );
PLC_ISR_STUB(dma1_stream7_isr      );
PLC_ISR_STUB(fsmc_isr              );
PLC_ISR_STUB(sdio_isr              );
PLC_ISR_STUB(tim5_isr              );
PLC_ISR_STUB(spi3_isr              );
PLC_ISR_STUB(uart4_isr             );
PLC_ISR_STUB(uart5_isr             );
//PLC_ISR_STUB(tim6_dac_isr          ); USED!!!
//PLC_ISR_STUB(tim7_isr              ); USED!!!
PLC_ISR_STUB(dma2_stream0_isr      );
PLC_ISR_STUB(dma2_stream1_isr      );
PLC_ISR_STUB(dma2_stream2_isr      );
PLC_ISR_STUB(dma2_stream3_isr      );
PLC_ISR_STUB(dma2_stream4_isr      );
PLC_ISR_STUB(eth_isr               );
PLC_ISR_STUB(eth_wkup_isr          );
PLC_ISR_STUB(can2_tx_isr           );
PLC_ISR_STUB(can2_rx0_isr          );
PLC_ISR_STUB(can2_rx1_isr          );
PLC_ISR_STUB(can2_sce_isr          );
PLC_ISR_STUB(otg_fs_isr            );
PLC_ISR_STUB(dma2_stream5_isr      );
PLC_ISR_STUB(dma2_stream6_isr      );
PLC_ISR_STUB(dma2_stream7_isr      );
PLC_ISR_STUB(usart6_isr            );
PLC_ISR_STUB(i2c3_ev_isr           );
PLC_ISR_STUB(i2c3_er_isr           );
PLC_ISR_STUB(otg_hs_ep1_out_isr    );
PLC_ISR_STUB(otg_hs_ep1_in_isr     );
PLC_ISR_STUB(otg_hs_wkup_isr       );
PLC_ISR_STUB(otg_hs_isr            );
PLC_ISR_STUB(dcmi_isr              );
PLC_ISR_STUB(cryp_isr              );
PLC_ISR_STUB(hash_rng_isr          );
