#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_aout.h>
#include <plc_gpio.h>
#include <plc_iom.h>


// Управление ЦАП
static const plc_gpio_t dac[] =
{
    PLC_GPIO_REC(DAC_CLK),
    PLC_GPIO_REC(DAC_DIN),
    PLC_GPIO_REC(DAC_SYN0),
    PLC_GPIO_REC(DAC_SYN1)
};

void _plc_aout_init(void)
{
    // Управление ЦАП
    PLC_GPIO_GR_CFG_OUT(dac);

    gpio_clear(PLC_DAC_CLK_PORT,  PLC_DAC_CLK_PIN);
    gpio_clear(PLC_DAC_DIN_PORT,  PLC_DAC_DIN_PIN);
    gpio_set  (PLC_DAC_SYN0_PORT, PLC_DAC_SYN0_PIN);
    gpio_set  (PLC_DAC_SYN1_PORT, PLC_DAC_SYN1_PIN);
}

//Буфер аналогового ввода

// *** ext DAC для AO ***
uint16_t plc_aout_dataA  = 0;  // данные для DAC первого канала, 0...4095
uint16_t plc_aout_dataB  = 0;  // данные для DAC второго канала, 0...4095
uint16_t plc_aout_clk   = 0;  // счётчик прерываний для генерации последовательности сигналов

// DAC аналоговых выходов
void _plc_aout_dac_poll(void)
{
    plc_aout_clk++;

    // изготовление синхронизации
    if (plc_aout_clk & 0x0001)
    {
        gpio_clear(PLC_DAC_CLK_PORT, PLC_DAC_CLK_PIN);
    }
    else
    {
        gpio_set(PLC_DAC_CLK_PORT, PLC_DAC_CLK_PIN);
    }

    // изготовление сигнала SYNC и DIN для A
    if ((plc_aout_clk&PLC_DAC_SYNC_MSK)==0x0000)
    {
        gpio_clear(PLC_DAC_SYN0_PORT, PLC_DAC_SYN0_PIN);

        if ( (plc_aout_dataA >> (0x000F-(((plc_aout_clk&0x001F)>>1)&0x000F)))&0x0001) ///Black magic!
        {
            gpio_set(PLC_DAC_DIN_PORT, PLC_DAC_DIN_PIN);
        }
        else
        {
            gpio_clear(PLC_DAC_DIN_PORT, PLC_DAC_DIN_PIN);
        }
    }
    else
    {
        gpio_set(PLC_DAC_SYN0_PORT, PLC_DAC_SYN0_PIN);
    }

    // изготовление сигнала SYNC и DIN для B
    if ((plc_aout_clk&PLC_DAC_SYNC_MSK)==0x0040)
    {
        gpio_clear(PLC_DAC_SYN1_PORT, PLC_DAC_SYN1_PIN);

        if ( (plc_aout_dataB >> (0x000F-(((plc_aout_clk&0x001F)>>1)&0x000F)))& 0x0001)
        {
            gpio_set(PLC_DAC_DIN_PORT, PLC_DAC_DIN_PIN);
        }
        else
        {
            gpio_clear(PLC_DAC_DIN_PORT, PLC_DAC_DIN_PIN);
        }
    }
    else
    {
        gpio_set(PLC_DAC_SYN1_PORT, PLC_DAC_SYN1_PIN);
    }
}


#define LOCAL_PROTO plc_aout
void PLC_IOM_LOCAL_INIT(void)
{
    _plc_aout_init();
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
bool PLC_IOM_LOCAL_CHECK(uint16_t lid)
{
    return true;
}
void PLC_IOM_LOCAL_BEGIN(uint16_t lid)
{
}

void PLC_IOM_LOCAL_END(uint16_t lid)
{
}

void PLC_IOM_LOCAL_START(void)
{
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
}

void PLC_IOM_LOCAL_STOP(void)
{
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t lid)
{
    return 0;
}

uint32_t PLC_IOM_LOCAL_SET(uint16_t lid)
{
    return 0;
}
#undef LOCAL_PROTO
