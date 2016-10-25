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

static inline void _plc_aout_init(void)
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

        if ((plc_aout_dataA >> (0x000F-(((plc_aout_clk&0x001F)>>1)&0x000F)))&0x0001) ///Black magic!
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

static const char plc_aout_err_asz[]     = "Analog output adress must be one number!";
static const char plc_aout_err_tp[]      = "Analog output does not support input locations!";
static const char plc_aout_err_addr[]    = "Analog output adress must be in 0 or 1!";

bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    if (PLC_LSZ_W != PLC_APP->l_tab[i]->v_size)
    {
        PLC_LOG_ERR_SZ();
        return false;
    }

    if (PLC_LT_Q != PLC_APP->l_tab[i]->v_type)
    {
        PLC_LOG_ERROR(plc_aout_err_tp);
        return false;
    }

    if (1 != PLC_APP->l_tab[i]->a_size)
    {
        PLC_LOG_ERROR(plc_aout_err_asz);
        return false;
    }

    if (2 <= PLC_APP->l_tab[i]->a_data[0])
    {
        PLC_LOG_ERROR(plc_aout_err_addr);
        return false;
    }
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
    plc_aout_dataA = 0;
    plc_aout_dataB = 0;
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}

uint32_t PLC_IOM_LOCAL_GET(uint16_t lid)
{
    return 0;
}

///TODO: add calibration!!!
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    switch(plc_curr_app->l_tab[i]->a_data[0])
    {
    case 0:
        plc_aout_dataA = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
    case 1:
        plc_aout_dataB = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
