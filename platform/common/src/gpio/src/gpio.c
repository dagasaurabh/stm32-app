#include <stdbool.h>
#include "gpio.h"

struct {
    bool initialized;
    const struct gpio_ops * ops;
}g_gpio;

void gpio_register(const struct gpio_ops * ops)
{
    if(g_gpio.initialized) return;

    g_gpio.initialized = true;

    g_gpio.ops = ops;
}

void gpio_init(gpio_pin_t pin, gpio_mode_t mode, gpio_pull_t pp)
{
	if(!g_gpio.initialized) return;
    if(g_gpio.ops && g_gpio.ops->init) g_gpio.ops->init(pin, mode, pp);
}

void gpio_write(gpio_pin_t pin, gpio_level_t level)
{
	if(!g_gpio.initialized) return;
    if(g_gpio.ops && g_gpio.ops->write) g_gpio.ops->write(pin, level);
}

gpio_level_t gpio_read(gpio_pin_t pin)
{
	if(!g_gpio.initialized) return GPIO_LOW;
    if(g_gpio.ops && g_gpio.ops->read) return g_gpio.ops->read(pin);

    return GPIO_LOW;
}

void gpio_toggle(gpio_pin_t pin)
{
	if(!g_gpio.initialized) return;
    if(g_gpio.ops && g_gpio.ops->toggle) g_gpio.ops->toggle(pin);
}

int gpio_irq_register(gpio_pin_t pin,
        gpio_irq_edge_t edge,
        gpio_irq_cb_t cb,
        void *ctx)
{
	if(!g_gpio.initialized) return -1;
    if(g_gpio.ops && g_gpio.ops->irq_register) return g_gpio.ops->irq_register(pin, edge, cb, ctx);
    return -1;
}

void gpio_irq_enable(gpio_pin_t pin)
{
	if(!g_gpio.initialized) return;
    if(g_gpio.ops && g_gpio.ops->irq_enable) g_gpio.ops->irq_enable(pin);
}

void gpio_irq_disable(gpio_pin_t pin)
{
	if(!g_gpio.initialized) return;
    if(g_gpio.ops && g_gpio.ops->irq_disable) g_gpio.ops->irq_disable(pin);
}


uint16_t gpio_get_pin(gpio_pin_t pin)
{
    return 1U << (pin & 0xFFFF);
}

uint16_t gpio_get_port(gpio_pin_t pin)
{
    return (pin >> 16) & 0xFF;
}

