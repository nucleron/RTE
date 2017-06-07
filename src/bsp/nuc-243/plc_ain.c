/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <ADC/adc.h>
#include <noise_flt.h>

#include <plc_config.h>
#include <plc_abi.h>
#include <plc_ain.h>
#include <plc_gpio.h>
#include <plc_iom.h>
#include <plc_tick.h>

// *** int ADC для AI ***
ai_data_t  analog_input[4];      // четыре комплекта для конфигурации аналоговых входов

adc_data_t other_analog[4];      // четыре других аналоговых - напряжения, температура и VBAT

uint8_t plc_adc_clk   = 0;   // счётчик прерываний для обеспечения процесса
uint8_t k,m;                  // счётчик для чего то там (циклы для переменных в исходное)

// *** ПРОВЕРКА КОРРЕКТНОСТИ КОНФИГ. ДАННЫХ ДЛЯ АЦП ***
static void _plc_ain_cfg_chk(ai_data_t * self)
{
    // проверка корректности указания глубины медианного фильтра
    if (NOISE_FLT_BUFSZ < self->median.depth)
    {
        self->median.depth = NOISE_FLT_BUFSZ;
    }
    if (PLC_MIN_MEDIAN_DEPH > self->median.depth)
    {
        self->median.depth = PLC_MIN_MEDIAN_DEPH;
    }

    // проверка корректности указания глубины усредняющего фильтра
    if (NOISE_FLT_BUFSZ < self->ave.depth)
    {
        self->ave.depth = NOISE_FLT_BUFSZ;
    }
    if (PLC_MIN_AVE_DEPH > self->ave.depth)
    {
        self->ave.depth = PLC_MIN_AVE_DEPH;
    }
}
//===============================================================================================
static void _ai_setup(ai_data_t * self)
{
    // AI
    noise_flt_init(&self->median, PLC_DEFAULT_MEDIAN_DEPH, PLC_DEFAULT_MEDIAN_VAL);
    noise_flt_init(&self->ave, PLC_DEFAULT_AVE_DEPH, PLC_DEFAULT_AVE_VAL);

    self->polling_counter = PLC_DEFAULT_POLL_CNT;
    self->polling_period  = PLC_DEFAULT_POLL_PERIOD;
    self->threshold_low   = PLC_DEFAULT_THR_LOW;
    self->threshold_high  = PLC_DEFAULT_THR_HIGH;
    self->signal_level    = PLC_DEFAULT_ADC_VAL;
    self->signal_schmitt  = PLC_DEFAULT_CMP_VAL;
    self->flag            = PLC_DEFAULT_ADC_FLG;
    self->mode            = PLC_DEFAULT_ADC_MODE;

    self->clb.coef._10v       = PLC_DEFAULT_COEF_10V;
    self->clb.coef._20ma      = PLC_DEFAULT_COEF_20MA;
    self->clb.coef._100r      = PLC_DEFAULT_COEF_100R;
    self->clb.coef._4k        = PLC_DEFAULT_COEF_4K;

    _plc_ain_cfg_chk(self);
}

static void _other_data_init(adc_data_t * self)
{
    self->counter     = PLC_DEFAULT_CNTR;
    self->sum         = PLC_DEFAULT_SUM;
    self->data_in     = PLC_DEFAULT_DATA_IN;
    self->data_out    = PLC_DEFAULT_OUT;
    self->calc        = PLC_DEFAULT_CALC;
    self->flag        = PLC_DEFAULT_ADC_FLG;
}

static void ai_setup(void)
{
    uint8_t k;

    // Исходные конфигурационные данные для аналоговых входов
    for (k=0; k<4; k++)
    {
        _ai_setup(analog_input + k);
        // прочие аналоговые
        _other_data_init(other_analog + k);
    }
}

// *** ВЫЧИСЛЕНИЕ НАПРЯЖЕНИЯ ПИТАНИЯ CPU ***
static uint16_t vcc_3v3_calc(uint32_t data, uint8_t sum)
// data - сумма данных за sum измерений,
// на выходе напряжение питания в мВ
{
    return ((U_REF_2V5 * 4095) / (data / sum));
}

// *** ВЫЧИСЛЕНИЕ НАПРЯЖЕНИЯ ПИТАНИЯ 18v ***
static uint16_t vcc_18v_calc(uint32_t data, uint8_t sum)
// data * 32 = 4095 * 32 - максимум!
{
    return ((data/sum) * other_analog[0].calc * 8) / 3043;
}

// *** ВЫЧИСЛЕНИЕ ТЕМПЕРАТУРЫ ***
static uint16_t temp_calc(uint32_t data, uint8_t sum)
// 113 суммирования data максимум!
{
    uint32_t u_t_sens = (data * other_analog[0].calc * 10) / (4095 * sum);
    uint16_t out;

    if (u_t_sens < (U_SENS_AT_0))
    {
        // температура отрицательная, не знаю что с этим делать
        out = 0;
    }
    else
    {
        // температура положительная, можно посчитать
        out = (u_t_sens - U_SENS_AT_0) / U_SENS_MV_C;
    }
    // просто градусы
    return out;
}

// *** ВЫЧИСЛЕНИЕ VBAT ***
static uint16_t vbat_calc(uint32_t data,uint8_t sum)
{
    // не забудьте включить VBATE
    return (data * other_analog[0].calc) / (2047 *sum);
}

// *** ВЫЧИСЛЕНИЕ НАПРЯЖЕНИЯ 0...10 В для AI
static uint16_t ai_10v_calc(uint16_t data, ai_data_t * ch)
{
    uint32_t i = (data * other_analog[0].calc * 34);
    // напряжение в мВ
    return (uint16_t)(i / ch->clb.coef._10v);
}

// *** ВЫЧИСЛЕНИЕ ТОКА 4...20 мА для AI
static uint16_t ai_20ma_calc(uint16_t data, ai_data_t * ch)
{
    /*
                             Напр.пит. CPU * N (отсчёты) * 20 000 мкА
    Ток 4-20, мкА =  ----------------------------------------------------------,
                      4095 * 2550 (напряжение в мВ на 127,5 Ом при токе 20 мА)

    где: N (отсчёты) = data, напр.пит. CPU = other_analog[0].calc,
    сокращаем 20 000 / (4095 * 2550) = 80 / 41769.

    Множитель 80 оставляем в формуле, 41769 отправляем в переменную структуры для канала.
    */
    uint32_t i = (data * other_analog[0].calc * 80);  // толстое число, в 32 бита только входит
    return (uint16_t)(i / ch->clb.coef._20ma ); // результат в мкА.
}

static inline uint16_t ai_R_calc(uint16_t data, uint16_t calib)
{
#   define PLC_MAX_ADC 4095ul
    return (uint16_t)(((uint32_t)calib * (uint32_t)data)/(PLC_MAX_ADC - (uint32_t)data));
}

// *** ВЫЧИСЛЕНИЕ СОПРОТИВЛЕНИЯ 100 Ом для AI
static uint16_t ai_100r_calc(uint16_t data, ai_data_t * ch)
{
//    // Расчётный коэф: 218 мОм на разряд АЦП
//    uint32_t i = (data * ch->clb.coef._100r);
//    // сопротивление в 0,1 Ом. См. "Расчёты" в проекте.
//    return (uint16_t)(i / 100);
    return ai_R_calc(data, ch->clb.coef._100r);
}

// *** ВЫЧИСЛЕНИЕ СОПРОТИВЛЕНИЯ 4 кОм для AI
static uint16_t ai_4k_calc(uint16_t data, ai_data_t * ch)
{
//    // Расчётный коэф: 3141 мОм на разряд АЦП
//    uint32_t i = (data * ch->clb.coef._4k);
//    // сопротивление в омах. См. "Расчёты" в проекте.
//    return (uint16_t)(i / 1000);
    return ai_R_calc(data, ch->clb.coef._4k);
}

static uint16_t ai_no_calc(uint16_t data, ai_data_t * ch)
{
    (void)ch;
    return data;
}

static void __plc_ain_other_data_calc(adc_data_t * self, other_calc_func func)
{
    if (self->flag)
    {
        self->calc = func(self->data_out, self->sum);
        self->flag = 0;
    }
}

void _plc_ain_other_data_calc(void)
{
    uint8_t i;
    //Обработчики измерений
    static const other_calc_func calc[4] =
    {
        vcc_3v3_calc,
        vcc_18v_calc,
        temp_calc,
        vbat_calc
    };

    for (i=0; i<4; i++)
    {
        __plc_ain_other_data_calc(other_analog + i, calc[i]);
    }
}

static void __plc_ain_data_calc(ai_data_t * self)
{
    if (self->flag)
    {
        uint8_t mode;
        calc_func func;
        //Обработчики измерений
        static const calc_func calc[] =
        {
            ai_no_calc,    // 0 - шунты отключены, стабилизаторы тока отключены
            ai_10v_calc,   // 1 - шунт 21.5 кОм, 0...10 В, порт "b"
            ai_20ma_calc,  // 2 - шунт 127 Ом, 4...20 мА, порт "a"
            ai_100r_calc,  // 3 - шунты отключены, измерение сопротивления 0...100R, порт "a"
            ai_4k_calc     // 4 - шунты отключены, измерение сопротивления 0...4000R, порт "a"
        };
        //gpio_set(TxLED_PORT,TxLED_PIN);

        // внесенние данных в массив усредняющего фильтра
        noise_flt_write(&self->ave, noise_flt_median(&self->median, self->mbuf));

        // пересчёт результатов АЦП в физ. величины
        mode = self->mode;
        if (mode < (sizeof(calc)/sizeof(calc_func)))
        {
            func = calc[mode];
        }
        else
        {
            func = ai_no_calc; //Nothing to convert
        }
        self->signal_level = func(noise_flt_ave(&self->ave), self);

        // псевдо цифровой вход
        if (self->signal_schmitt)
        {
            if (self->signal_level < self->threshold_low)
            {
                self->signal_schmitt = false;
            }
        }
        else
        {
            if (self->signal_level > self->threshold_high)
            {
                self->signal_schmitt = true;
            }
        }

        self->flag = 0;
    }
}

void _plc_ain_data_calc(void)
{
    uint8_t i;
    for (i=0; i<4; i++)
    {
        __plc_ain_data_calc(analog_input + i);
    }
}

static void ai_read_begin(ai_data_t * self)
{
    if (self->polling_counter == (self->polling_period - 1))
    {
        noise_flt_write(&self->median, adc_read_regular(ADC1));
    }
}

static void _ai_read_end(ai_data_t * self)
{
    self->polling_counter++;
    if (self->polling_counter >= self->polling_period)
    {
        self->flag = 1;
        self->polling_counter = 0;
    }
}

static inline bool ai_sched_adc(ai_data_t * self)
{
    return (bool)(self->polling_counter == (self->polling_period - 1));
}

static void _other_read_begin(adc_data_t * self)
{
    if (self->sum)
    {
        self->data_in = self->data_in + adc_read_regular(ADC1);
    }
}

static inline bool _other_sched_adc(adc_data_t * self)
{
    return (bool)(self->sum);
}

static void _other_read_end(adc_data_t * self)
{
    // проверка разрешена ли обработка
    if (self->sum)
    {
        self->counter++;
        if (self->counter >= self->sum)
        {
            // gpio_set(RxLED_PORT,RxLED_PIN);
            self->counter   = 0;
            self->flag      = 1;
            self->data_out  = self->data_in;
            self->data_in   = 0;
            // gpio_clear(RxLED_PORT,RxLED_PIN);
        }
    }
}


void _plc_ain_adc_poll(void)
{
    uint8_t channel_array[1];
    uint8_t i = 0;
    bool start_convertion = false;
    // I. ИЗВЛЕЧЕНИЕ ДАННЫХ, ОБРАБОТКА КАЖДУЮ МИЛЛИСЕКУНДУ
    switch (plc_adc_clk)
    {
    case 0: // сбор данных для каналов измерения напряжений и температуры
    case 1:
    case 2:
    case 3:
        _other_read_begin(other_analog + plc_adc_clk);
        break;
    case 4: // обработка данных раз в миллисекунду
        for (i=0; i<4; i++)
        {
            _other_read_end(other_analog + i);
        }
        break;
    case 5: // сбор данных для AI
    case 6:
    case 7:
    case 8:
        i = plc_adc_clk - 5;
        ai_read_begin(analog_input + i);
        break;
    case 9: // обработка данных AI раз в миллисекунду
        for (i=0; i<4; i++)
        {
            //gpio_set(RxLED_PORT,RxLED_PIN);
            _ai_read_end(analog_input + i);
            //gpio_clear(RxLED_PORT,RxLED_PIN);
        }
        break;
    }
    // II. ПЕРЕКЛЮЧЕНИЕ НА ДРУГОЙ КАНАЛ
    plc_adc_clk++;
    plc_adc_clk %= PLC_ADC_CNT_THR;
    // III. НАЧАЛО КОНВЕРСИИ
    switch (plc_adc_clk)
    {
    case 0: // сбор данных для каналов измерения напряжений и температуры
    case 1:
    case 2:
    case 3:
        start_convertion = _other_sched_adc(other_analog + plc_adc_clk);
        break;
    case 5: // сбор данных для AI
    case 6:
    case 7:
    case 8:
        i = plc_adc_clk - 5;
        start_convertion = ai_sched_adc(analog_input + i);
        break;
    default:
        break;
    }
    // Отправка запроса
    if (start_convertion)
    {
        // выбор канала измерения
        static const uint8_t adc_chan[] =
        {
            ADC_CHANNEL_REF2v5,
            ADC_CHANNEL_18v,
            ADC_CHANNEL_TEMP_F40,
            ADC_CHANNEL_VBAT,
            0,
            ADC_CHANNEL_AIN0,
            ADC_CHANNEL_AIN1,
            ADC_CHANNEL_AIN2,
            ADC_CHANNEL_AIN3
        };
        channel_array[0] = adc_chan[plc_adc_clk];

        adc_set_regular_sequence(ADC1, 1, channel_array);
        adc_start_conversion_regular(ADC1);

        start_convertion = false;
    }
}

//Питание аналоговых входов
static const plc_gpio_t pwr_ai0[] =
{
    PLC_GPIO_REC(PWR_AI00),
    PLC_GPIO_REC(PWR_AI01),
    PLC_GPIO_REC(PWR_AI02),
    PLC_GPIO_REC(PWR_AI03)
};

static const plc_gpio_t pwr_ai1[] =
{
    PLC_GPIO_REC(PWR_AI10),
    PLC_GPIO_REC(PWR_AI11),
    PLC_GPIO_REC(PWR_AI12),
    PLC_GPIO_REC(PWR_AI13)
};
// Управление шунтами аналоговых входов
static const plc_gpio_t non0[] =
{
    PLC_GPIO_REC(NON00),
    PLC_GPIO_REC(NON01),
    PLC_GPIO_REC(NON02),
    PLC_GPIO_REC(NON03)
};

static const plc_gpio_t non1[] =
{
    PLC_GPIO_REC(NON10),
    PLC_GPIO_REC(NON11),
    PLC_GPIO_REC(NON12),
    PLC_GPIO_REC(NON13)
};



void _plc_ain_cfg(uint32_t port, uint32_t mode)
{
    switch (mode)
    {
    case PLC_AIN_MODE_4K:
        plc_gpio_clear(pwr_ai0 + port);
        plc_gpio_set  (pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_100R:
        plc_gpio_set  (pwr_ai0 + port);
        plc_gpio_clear(pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_20MA:
        plc_gpio_set  (pwr_ai0 + port);
        plc_gpio_set  (pwr_ai1 + port);
        plc_gpio_set  (non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_10V:
        plc_gpio_set  (pwr_ai0 + port);
        plc_gpio_set  (pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_set  (non1   + port);
        break;
    case PLC_AIN_MODE_OFF:
    default:
        plc_gpio_set  (pwr_ai0 + port);
        plc_gpio_set  (pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    }
}

void adc_setup(void)
{
    rcc_periph_clock_enable(RCC_ADC1);

    adc_power_off(ADC1);
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY8);
    adc_enable_temperature_sensor();
    // включить делитель для измерения VBAT
    // включить только на время измерения
    // тут только для примера
    // ADC_CCR = ADC_CCR | 0x00400000;

    adc_disable_scan_mode(ADC1);
    adc_set_right_aligned(ADC1);
    //adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);
    //adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);

    adc_power_on(ADC1);
}

void _plc_ain_init(void)
{
    int i;
    // Питание аналоговых портов
    PLC_GPIO_GR_CFG_OUT(pwr_ai0);
    PLC_GPIO_GR_SET    (pwr_ai0);

    PLC_GPIO_GR_CFG_OUT(pwr_ai1);
    PLC_GPIO_GR_SET    (pwr_ai1);

    // Управление шунтами аналоговых входов
    PLC_GPIO_GR_CFG_OUT(non0);
    PLC_GPIO_GR_CLEAR  (non0);

    PLC_GPIO_GR_CFG_OUT(non1);
    PLC_GPIO_GR_CLEAR  (non1);

    // Аналоговые порты
    static const plc_gpio_t ain_pin[] =
    {
        PLC_GPIO_REC(REF2V5),
        PLC_GPIO_REC(ADC18),
        PLC_GPIO_REC(AIN0),
        PLC_GPIO_REC(AIN1),
        PLC_GPIO_REC(AIN2),
        PLC_GPIO_REC(AIN3)
    };

    for(i=0; i<sizeof(ain_pin)/sizeof(plc_gpio_t); i++)
    {
        rcc_periph_clock_enable(ain_pin[i].periph);
        gpio_mode_setup(ain_pin[i].port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ain_pin[i].pin);
    }

    adc_setup();
    ai_setup();
}


#define LOCAL_PROTO plc_ain
void PLC_IOM_LOCAL_INIT(void)
{
    _plc_ain_init();
}
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    return true;
}
/*
static const char plc_ain_err_asz[]    = "Analog output adress must be two numbers!";
static const char plc_ain_err_mem[]    = "Analog input does not support memory locations!";
static const char plc_ain_err_chn[]    = "Analog input chanel number must be in 0..3!";

static const char plc_ain_err_ix[]     = "Analog input: this location must be input!";
static const char plc_ain_err_ix9[]    = "Analog input: this location must have 8 at address end!";

static const char plc_ain_err_qb[]     = "Analog input: this location must be output!";
static const char plc_ain_err_qbx[]    = "Analog input: this location must have 1,2 or 3 at address end!";

static const char plc_ain_err_qwx[]    = "Analog input: this location must have 4,5 or 6 at address end!";
static const char plc_ain_err_iw7[]    = "Analog input: this location must have 7 at address end!";
*/
bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    if (PLC_LT_M == PLC_APP->l_tab[i]->v_type)
    {
        //PLC_LOG_ERROR(plc_ain_err_mem);
        plc_iom_errno_print(PLC_ERRNO_AIN_MEM);
        return false;
    }

    if (2 != PLC_APP->l_tab[i]->a_size)
    {
        //PLC_LOG_ERROR(plc_ain_err_asz);
        plc_iom_errno_print(PLC_ERRNO_AIN_ASZ);
        return false;
    }

    if (5 <= PLC_APP->l_tab[i]->a_data[0])
    {
        //PLC_LOG_ERROR(plc_ain_err_chn);
        plc_iom_errno_print(PLC_ERRNO_AIN_CHN);
        return false;
    }

    switch (PLC_APP->l_tab[i]->v_size)
    {
    case PLC_LSZ_X:
        if (PLC_LT_I != PLC_APP->l_tab[i]->v_type)
        {
            //PLC_LOG_ERROR(plc_ain_err_ix);
            plc_iom_errno_print(PLC_ERRNO_AIN_IX);
            return false;
        }
        if (8 != PLC_APP->l_tab[i]->a_data[1])
        {
            //PLC_LOG_ERROR(plc_ain_err_ix9);
            plc_iom_errno_print(PLC_ERRNO_AIN_IX9);
            return false;
        }
        break;
    case PLC_LSZ_B:
        if (PLC_LT_Q != PLC_APP->l_tab[i]->v_type)
        {
            //PLC_LOG_ERROR(plc_ain_err_qb);
            plc_iom_errno_print(PLC_ERRNO_AIN_QB);
            return false;
        }
        if (3 < PLC_APP->l_tab[i]->a_data[1])
        {
            //PLC_LOG_ERROR(plc_ain_err_qbx);
            plc_iom_errno_print(PLC_ERRNO_AIN_QBX);
            return false;
        }
        break;
    case PLC_LSZ_W:
        if (PLC_LT_Q == PLC_APP->l_tab[i]->v_type)
        {
            //%QW6.chn.4..6
            if (6 < PLC_APP->l_tab[i]->a_data[1])
            {
                //PLC_LOG_ERROR(plc_ain_err_qwx);
                plc_iom_errno_print(PLC_ERRNO_AIN_QWX);
                return false;
            }

            if (4 > PLC_APP->l_tab[i]->a_data[1])
            {
                //PLC_LOG_ERROR(plc_ain_err_qwx);
                plc_iom_errno_print(PLC_ERRNO_AIN_QWX);
                return false;
            }
        }
        else
        {
            //%IW.chn.7
            if (7 != PLC_APP->l_tab[i]->a_data[1])
            {
                //PLC_LOG_ERROR(plc_ain_err_iw7);
                plc_iom_errno_print(PLC_ERRNO_AIN_IW7);
                return false;
            }
        }
        break;
    default:
        PLC_LOG_ERR_SZ();
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

static ai_clb_t clb_data;

static const ai_clb_t clb_data_min =
{
    .coef._10v  = PLC_DEFAULT_COEF_10V  - PLC_COEF_DELTA_10V,
    .coef._20ma = PLC_DEFAULT_COEF_20MA - PLC_COEF_DELTA_20MA,
    .coef._100r = PLC_DEFAULT_COEF_100R - PLC_COEF_DELTA_100R,
    .coef._4k   = PLC_DEFAULT_COEF_4K   - PLC_COEF_DELTA_4K,
};

static const ai_clb_t clb_data_max =
{
    .coef._10v  = PLC_DEFAULT_COEF_10V  + PLC_COEF_DELTA_10V,
    .coef._20ma = PLC_DEFAULT_COEF_20MA + PLC_COEF_DELTA_20MA,
    .coef._100r = PLC_DEFAULT_COEF_100R + PLC_COEF_DELTA_100R,
    .coef._4k   = PLC_DEFAULT_COEF_4K   + PLC_COEF_DELTA_4K,
};


void PLC_IOM_LOCAL_START(void)
{
    int i;
    for (i=0; i<4; i++)
    {
        int j;
        static const other_analog_cfg[4] = {8,8,8,0};
        other_analog[i].sum             = other_analog_cfg[i];

        analog_input[i].polling_period  = (plc_tick_time/1000000);
        analog_input[i].mode            = PLC_AIN_MODE_OFF;
        _plc_ain_cfg(i,PLC_AIN_MODE_OFF);
        //Calibration data must be valid
        if (0 == PLC_CLB_VER & 0x1)
        {
            continue;
        }
        //Read calib data only uint32_t acces is supported by backup regs
        clb_data.reg[0] = PLC_CLB_REGS[2*i];
        clb_data.reg[1] = PLC_CLB_REGS[2*i+1];
        //Check coef values, defaults are in place
        for (j=0; j<4; j++)
        {
            if ((clb_data_min.cval[j] < clb_data.cval[j]) && \
                (clb_data_max.cval[j] > clb_data.cval[j]))
            {
                analog_input[i].clb.cval[j] = clb_data.cval[j];
            }
        }
    }
}

uint32_t PLC_IOM_LOCAL_SCHED(uint16_t lid, uint32_t tick)
{
    return 0;
}

void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    _plc_ain_other_data_calc();  // пересчёт данных АЦП в напряжения и температуру раз в sum мс
    _plc_ain_data_calc();     // раз в необходимо кол-во мс фильтрация, пересчёт данных с АЦП аналоговых входов
}

void PLC_IOM_LOCAL_STOP(void)
{
    int i;
    for (i=0; i<4; i++)
    {
        analog_input[i].mode = PLC_AIN_MODE_OFF;
        _plc_ain_cfg(i,PLC_AIN_MODE_OFF);
    }
}

uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t lid)
{
    return PLC_APP->l_tab[lid]->a_data[0];
}

///TODO: Add calibration!!!

uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    uint8_t chn;
    //Chanel number
    chn = PLC_APP->l_tab[i]->a_data[0];

    switch (PLC_APP->l_tab[i]->a_data[1])
    {
    case 7:
    {
        *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf) = analog_input[chn].signal_level;
        break;
    }
    case 8:
    {
        *(bool *)(plc_curr_app->l_tab[i]->v_buf) = analog_input[chn].signal_schmitt;
        break;
    }
    default:
        break;
    }
    return 0;
}

uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    uint8_t chn;
    //Chanel number
    chn = PLC_APP->l_tab[i]->a_data[0];

    switch (PLC_APP->l_tab[i]->a_data[1])
    {
    case 1:
    {
        uint8_t tmp;
        tmp = *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        if( 0 == analog_input[chn].mode - tmp )
        {
            break;
        }
        if (PLC_AIN_MODE_4K < tmp)
        {
            tmp = PLC_AIN_MODE_4K;
        }
        //Set chanel processing mode
        analog_input[chn].mode = tmp;
        //Configure pins
        _plc_ain_cfg(chn,tmp);
        break;
    }
    case 2:
    {
        uint8_t tmp;
        tmp = *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        if (3 > tmp)
        {
            tmp = 3;
        }
        if (NOISE_FLT_BUFSZ < tmp)
        {
            tmp = NOISE_FLT_BUFSZ;
        }
        //Set chanel processing mode
        analog_input[chn].median.depth = tmp;
        break;
    }
    case 3:
    {
        uint8_t tmp;
        tmp = *(uint8_t *)(plc_curr_app->l_tab[i]->v_buf);
        if (3 > tmp)
        {
            tmp = 3;
        }
        if (NOISE_FLT_BUFSZ < tmp)
        {
            tmp = NOISE_FLT_BUFSZ;
        }
        //Set chanel processing mode
        analog_input[chn].ave.depth = tmp;
        break;
    }
    case 4:
    {
        uint16_t tmp;
        tmp = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
        if (1 > tmp)
        {
            tmp = 1;
        }
        // Do we realy need this limitation?
//        if (tmp < (plc_tick_time/1000000))
//        {
//            tmp = plc_tick_time/1000000;
//        }
        //Set chanel processing mode
        analog_input[chn].polling_period = tmp;
        break;
    }
    case 5:
    {
        uint16_t tmp;
        tmp = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
        if (1 > tmp)
        {
            tmp = 1;
        }
        if (analog_input[chn].threshold_high <= tmp)
        {
            tmp = analog_input[chn].threshold_high - 1;
        }
        //Smidt trig thr low
        analog_input[chn].threshold_low = tmp;
        break;
    }
    case 6:
    {
        uint16_t tmp;
        tmp = *(uint16_t *)(plc_curr_app->l_tab[i]->v_buf);
        if (1 > tmp)
        {
            tmp = 1;
        }
        if (analog_input[chn].threshold_low >= tmp)
        {
            tmp = analog_input[chn].threshold_low + 1;
        }
        //Smidt trig thr high
        analog_input[chn].threshold_high = tmp;
        break;
    }
    default:
        break;
    }
    return 0;
}
#undef LOCAL_PROTO
