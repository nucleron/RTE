#ifndef _PLC_GPIO_H_
#define _PLC_GPIO_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    enum rcc_periph_clken periph;
    uint32_t port;
    uint16_t pin;
} plc_gpio_t;

#define PLC_GPIO_CONCAT(a, b) a##b
#define PLC_GPIO_CONCAT2(a, b) PLC_GPIO_CONCAT(a, b)

#define PLC_GPIO_THING(n, name) (PLC_GPIO_CONCAT2(PLC_GPIO_CONCAT(PLC_, n), name))

#define PLC_GPIO_PERIPH(n) PLC_GPIO_THING(n, _PERIPH)
#define PLC_GPIO_PORT(n)   PLC_GPIO_THING(n, _PORT)
#define PLC_GPIO_PIN(n)    PLC_GPIO_THING(n, _PIN)

#define PLC_GPIO_REC(n) {PLC_GPIO_PERIPH(n), PLC_GPIO_PORT(n), PLC_GPIO_PIN(n)}

void plc_gpio_cfg_in (const plc_gpio_t * self);
void plc_gpio_cfg_out(const plc_gpio_t * self);

static inline void plc_gpio_set(const plc_gpio_t * self)
{
    gpio_set  (self->port, self->pin);
}

static inline void plc_gpio_clear(const plc_gpio_t * self)
{
    gpio_clear(self->port, self->pin);
}

static inline bool plc_gpio_get(const plc_gpio_t * self)
{
    return (gpio_get(self->port, self->pin));
}

//group configs
void plc_gpio_gr_cfg_in (const plc_gpio_t * group, uint8_t n);
void plc_gpio_gr_cfg_out(const plc_gpio_t * group, uint8_t n);
void plc_gpio_gr_set    (const plc_gpio_t * group, uint8_t n);
void plc_gpio_gr_clear  (const plc_gpio_t * group, uint8_t n);

//Grpups must be delared before call
#define PLC_GPIO_GR_CFG_IN(grp)  plc_gpio_gr_cfg_in (grp, (uint8_t)(sizeof(grp)/sizeof(plc_gpio_t)))
#define PLC_GPIO_GR_CFG_OUT(grp) plc_gpio_gr_cfg_out(grp, (uint8_t)(sizeof(grp)/sizeof(plc_gpio_t)))
#define PLC_GPIO_GR_SET(grp)     plc_gpio_gr_set(grp, (uint8_t)(sizeof(grp)/sizeof(plc_gpio_t)))
#define PLC_GPIO_GR_CLEAR(grp)   plc_gpio_gr_clear(grp, (uint8_t)(sizeof(grp)/sizeof(plc_gpio_t)))

#endif /* _PLC_GPIO_H_ */
