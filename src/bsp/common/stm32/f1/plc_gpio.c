#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <plc_gpio.h>

//#pragma GCC optimize ("O0")

void plc_gpio_cfg_in(const plc_gpio_t * self)
{
    rcc_periph_clock_enable(self->periph);
    gpio_set_mode          (self->port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, self->pin);
    gpio_set               (self->port,                                              self->pin);
}

void plc_gpio_cfg_out(const plc_gpio_t * self)
{
    rcc_periph_clock_enable(self->periph );
    gpio_set_mode          (self->port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, self->pin);
    gpio_clear             (self->port,                                                    self->pin);
}

void plc_gpio_gr_cfg_in(const plc_gpio_t * group, uint8_t n)
{
    if (n==0)
    {
       return;
    }
    while (n--)
    {
        plc_gpio_cfg_in(group++);
    }
}

void plc_gpio_gr_cfg_out(const plc_gpio_t * group, uint8_t n)
{
    if (n==0)
    {
        return;
    }
    while (n--)
    {
        plc_gpio_cfg_out(group++);
    }
}

void plc_gpio_gr_set(const plc_gpio_t * group, uint8_t n)
{
    if (n==0)
    {
       return;
    }
    while (n--)
    {
        plc_gpio_set(group++);
    }
}

void plc_gpio_gr_clear(const plc_gpio_t * group, uint8_t n)
{
    if (n==0)
    {
       return;
    }
    while (n--)
    {
        plc_gpio_clear(group++);
    }
}

