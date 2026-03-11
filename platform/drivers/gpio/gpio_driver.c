#include "gpio_driver.h"
#include "gpio_hal_if.h"
#include <assert.h>

void
gpio_drv_init_pin(gpio_port_t port, uint32_t pin, gpio_mode_t mode, gpio_pull_t pull,
                  gpio_speed_t speed)
{
    assert(port != NULL);
    assert(pin != 0);
    gpio_hal_mode_t hal_mode = (gpio_hal_mode_t)-1;

#define mycase(__in, __out)                                                                        \
    case GPIO_MODE_##__in:                                                                         \
        hal_mode = GPIO_HAL_MODE_##__out;                                                          \
        break;
    switch (mode)
    {
        mycase(INPUT, INPUT);
        mycase(OUTPUT, OUTPUT);
        mycase(OUTPUT_OD, OUTPUT_OD);
        mycase(AF, AF);
        mycase(AF_OD, AF_OD);
        mycase(ANALOG, ANALOG);
    }
#undef mycase

    assert(hal_mode != -1);

    gpio_hal_speed_t hal_speed = (gpio_hal_speed_t)-1;

#define mycase(__in, __out)                                                                        \
    case GPIO_SPEED_##__in:                                                                        \
        hal_speed = GPIO_HAL_SPEED_##__out;                                                        \
        break;
    switch (speed)
    {
        mycase(LOW, LOW);
        mycase(MEDIUM, MEDIUM);
        mycase(HIGH, HIGH);
        mycase(VERY_HIGH, VERY_HIGH);
    }
#undef mycase
    assert(hal_speed != -1);

    gpio_hal_init(port, pin, hal_mode, pull, hal_speed);
}

void
gpio_drv_write(gpio_port_t port, uint32_t pin, uint8_t level)
{
    assert(port != NULL);
    assert(pin != 0);
    gpio_hal_write(port, pin, level);
}

uint8_t
gpio_drv_read(gpio_port_t port, uint32_t pin)
{
    assert(port != NULL);
    assert(pin != 0);
    return gpio_hal_read(port, pin);
}

void
gpio_drv_toggle(gpio_port_t port, uint32_t pin)
{
    assert(port != NULL);
    assert(pin != 0);
    gpio_hal_toggle(port, pin);
}

int
gpio_drv_irq_register(gpio_port_t port, uint32_t pin, gpio_irq_edge_t edge, gpio_drv_irq_cb_t cb,
                      void *ctx)
{
    assert(port != NULL);
    assert(pin != 0);

    gpio_hal_irq_config(port, pin, edge, cb, ctx);
    return 0;
}

void
gpio_drv_irq_enable(gpio_port_t port, uint32_t pin)
{
    assert(port != NULL);
    assert(pin != 0);

    gpio_hal_irq_enable(port, pin);
}

void
gpio_drv_irq_disable(gpio_port_t port, uint32_t pin)
{
    assert(port != NULL);
    assert(pin != 0);

    gpio_hal_irq_disable(port, pin);
}
