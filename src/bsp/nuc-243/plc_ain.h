#ifndef _PLC_AIN_H_
#define _PLC_AIN_H_

// Структура с конфигурацией и результатами для AI
typedef struct
{
    noise_flt_t median;  //Median filter
    uint16_t mbuf[NOISE_FLT_BUFSZ];

    noise_flt_t ave;     //Average filter

    uint16_t threshold_low;                  // нижний порог триггера шмидта (в единицах измерения)
    uint16_t threshold_high;                 // верхний порог триггера шмидта (в единицах измерения)
    uint16_t signal_level;                   // измеренный сигнал в (в единицах измерения)

    uint16_t _10v_coef;     //
    uint16_t _20ma_coef;    //
    uint16_t _100r_coef;   //
    uint16_t _4k_coef;      //

    uint16_t polling_counter;                 // счёт милисекунд периодичности
    uint16_t polling_period;                  // период опроса канала в мс

    uint8_t flag;                            // флаг обработки результатов, на это время низзя изменять данные в прерывании
    uint8_t mode;                            // режим работы порта, см. _plc_ain_cfg()

    bool    signal_schmitt;                  // значение выхода триггера Шмидта в канале
} ai_data_t;

// Структура с конфигурацией и результатами для прочих каналов АЦП
typedef struct
{
    uint32_t  data_in;      // сбор усреднённых данных (питание, температура, VBAT)
    uint32_t  data_out;     // итоговые данные для использования по назначению
    uint16_t  calc;         // пересчитанные в физ.величины: темп. в десятых градуса, напряжения в мВ
    uint8_t   sum;          // количество данных для усреднения (изм. раз в мс)
    uint8_t   counter;      // счёт штук для усреденения
    uint8_t   flag;         //
} adc_data_t;

typedef uint16_t (*calc_func)(uint16_t, ai_data_t *); //указатель на обрабтчик аналогового канала
typedef uint16_t (*other_calc_func)(uint32_t, uint8_t); //указатель на обрабтчик аналогового канала

// *** int ADC для AI ***
extern ai_data_t  analog_input[4];      // четыре комплекта для конфигурации аналоговых входов

extern adc_data_t other_analog[4];      // четыре других аналоговых - напряжения, температура и VBAT


void _plc_ain_init(void);
#define PLC_ADC_CNT_THR         10
void _plc_ain_adc_poll(void);
void _plc_ain_other_data_calc(void);
void _plc_ain_data_calc(void);

#define PLC_AIN_MODE_OFF  0
#define PLC_AIN_MODE_10V  1
#define PLC_AIN_MODE_20MA 2
#define PLC_AIN_MODE_100R 3
#define PLC_AIN_MODE_4K   4
/*  port - номер порта: 0...3,
    mode - режим работы:
    0 - шунты отключены, стабилизаторы тока отключены,
    1 - шунт 21.5 кОм, 0...10 В, порт "b",
    2 - шунт 127 Ом, 4...20 мА, порт "a",
    3 - шунты отключены, измерение сопротивления 0...100R, порт "a",
    4 - шунты отключены, измерение сопротивления 0...4000R, порт "a".
 */
void _plc_ain_cfg(uint32_t port, uint32_t mode);

#endif /* _PLC_AIN_H_ */
