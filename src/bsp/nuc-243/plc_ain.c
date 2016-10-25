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

    self->_10v_coef       = PLC_DEFAULT_COEF_10V;
    self->_20ma_coef      = PLC_DEFAULT_COEF_20MA;
    self->_100r_coef      = PLC_DEFAULT_COEF_100R;
    self->_4k_coef        = PLC_DEFAULT_COEF_4K;

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
    return (uint16_t)(i / ch->_10v_coef);
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
    return (uint16_t)(i / ch->_20ma_coef ); // результат в мкА.
}

// *** ВЫЧИСЛЕНИЕ СОПРОТИВЛЕНИЯ 100 Ом для AI
static uint16_t ai_100r_calc(uint16_t data, ai_data_t * ch)
{
    // Расчётный коэф: 218 мОм на разряд АЦП
    uint32_t i = (data * ch->_100r_coef);
    // сопротивление в 0,1 Ом. См. "Расчёты" в проекте.
    return (uint16_t)(i / 100);
}

// *** ВЫЧИСЛЕНИЕ СОПРОТИВЛЕНИЯ 4 кОм для AI
static uint16_t ai_4k_calc(uint16_t data, ai_data_t * ch)
{
    // Расчётный коэф: 3141 мОм на разряд АЦП
    uint32_t i = (data * ch->_4k_coef);
    // сопротивление в омах. См. "Расчёты" в проекте.
    return (uint16_t)(i / 1000);
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
        if (mode < sizeof(calc)/sizeof(calc_func))
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
        plc_gpio_set  (pwr_ai0 + port);
        plc_gpio_clear(pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_100R:
        plc_gpio_clear(pwr_ai0 + port);
        plc_gpio_set  (pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_20MA:
        plc_gpio_clear(pwr_ai0 + port);
        plc_gpio_clear(pwr_ai1 + port);
        plc_gpio_set  (non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    case PLC_AIN_MODE_10V:
        plc_gpio_clear(pwr_ai0 + port);
        plc_gpio_clear(pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_set  (non1   + port);
        break;
    case PLC_AIN_MODE_OFF:
    default:
        plc_gpio_clear(pwr_ai0 + port);
        plc_gpio_clear(pwr_ai1 + port);
        plc_gpio_clear(non0   + port);
        plc_gpio_clear(non1   + port);
        break;
    }
}

void adc_setup(void)
{
    rcc_periph_clock_enable(RCC_ADC1);

    adc_enable_temperature_sensor();

    // включить делитель для измерения VBAT
    // включить только на время измерения
    // тут только для примера
    // ADC_CCR = ADC_CCR | 0x00400000;

    adc_disable_scan_mode(ADC1);
    adc_set_right_aligned(ADC1);

    //adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28CYC);
    //adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);

    adc_power_on(ADC1);
}

void _plc_ain_init(void)
{
    int i;
        // Питание аналоговых портов
    PLC_GPIO_GR_CFG_OUT(pwr_ai0);
    PLC_GPIO_GR_CLEAR  (pwr_ai0);

    PLC_GPIO_GR_CFG_OUT(pwr_ai1);
    PLC_GPIO_GR_CLEAR  (pwr_ai1);

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

    /*
    rcc_periph_clock_enable(PLC_REF2V5_PERIPH);
    gpio_mode_setup(PLC_REF2V5_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_REF2V5_PIN);

    rcc_periph_clock_enable(PLC_ADC18_PERIPH);
    gpio_mode_setup(PLC_ADC18_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_ADC18_PIN);

    rcc_periph_clock_enable(PLC_AIN0_PERIPH);
    gpio_mode_setup(PLC_AIN0_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_AIN0_PIN);

    rcc_periph_clock_enable(PLC_AIN1_PERIPH);
    gpio_mode_setup(PLC_AIN1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_AIN1_PIN);

    rcc_periph_clock_enable(PLC_AIN2_PERIPH);
    gpio_mode_setup(PLC_AIN2_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_AIN2_PIN);

    rcc_periph_clock_enable(PLC_AIN3_PERIPH);
    gpio_mode_setup(PLC_AIN3_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PLC_AIN3_PIN);
    //*/

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
    // ДЛЯ ОТЛАДКИ: конфигурация АЦП для AI

    analog_input[0].polling_period  = 1;
    analog_input[1].polling_period  = 1;
    analog_input[2].polling_period  = 1;
    analog_input[3].polling_period  = 1;

    other_analog[0].sum = 1;
    other_analog[1].sum = 1;
    other_analog[2].sum = 1;
    other_analog[3].sum = 0;

    analog_input[0].mode = 1;
    analog_input[1].mode = 2;
    analog_input[2].mode = 3;
    analog_input[3].mode = 4;

    /*
     Конфигурация аналоговых портов
     port - номер порта: 0...3,
     mode - режим работы:
     4 - шунты отключены, измерение сопротивления 0...4000R, порт "a",
     3 - шунты отключены, измерение сопротивления 0...100R, порт "a",
     2 - шунт 127 Ом, 4...20 мА, порт "a",
     1 - шунт 21.5 кОм, 0...10 В, порт "b",
     0 - шунты отключены, стабилизаторы тока отключены.
     */

    _plc_ain_cfg(0,analog_input[0].mode);
    _plc_ain_cfg(1,analog_input[1].mode);
    _plc_ain_cfg(2,analog_input[2].mode);
    _plc_ain_cfg(3,analog_input[3].mode);
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
