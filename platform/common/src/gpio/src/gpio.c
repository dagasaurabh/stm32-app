#include <stdbool.h>
#include "gpio.h"
#include <stddef.h>

typedef struct {
    bool                  initialized;
    const struct gpio_ops *ops;
} gpio_ctrl_t;

static gpio_ctrl_t g_gpio[GPIO_MAX_CONTROLLERS];

uint8_t gpio_get_ctrl(gpio_pin_t pin)
{
    return (pin >> 24) & 0xFFU;
}

int gpio_register(uint8_t ctrl_id, const struct gpio_ops *ops)
{
    if (ctrl_id >= GPIO_MAX_CONTROLLERS) return -1;
    if (ops == NULL)                   return -1;
    if (g_gpio[ctrl_id].initialized)    return -1;
    g_gpio[ctrl_id].ops         = ops;
    g_gpio[ctrl_id].initialized = true;
    return 0;
}

void gpio_init(gpio_pin_t pin, gpio_mode_t mode, gpio_pull_t pp, gpio_speed_t speed)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->init) g_gpio[ctrl].ops->init(pin, mode, pp, speed);
}

void gpio_write(gpio_pin_t pin, gpio_level_t level)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->write) g_gpio[ctrl].ops->write(pin, level);
}

gpio_level_t gpio_read(gpio_pin_t pin)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return GPIO_LOW;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->read) return g_gpio[ctrl].ops->read(pin);
    return GPIO_LOW;
}

void gpio_toggle(gpio_pin_t pin)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->toggle) g_gpio[ctrl].ops->toggle(pin);
}

int gpio_irq_register(gpio_pin_t pin,
        gpio_irq_edge_t edge,
        gpio_irq_cb_t cb,
        void *ctx)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return -1;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->irq_register) return g_gpio[ctrl].ops->irq_register(pin, edge, cb, ctx);
    return -1;
}

void gpio_irq_enable(gpio_pin_t pin)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->irq_enable) g_gpio[ctrl].ops->irq_enable(pin);
}

void gpio_irq_disable(gpio_pin_t pin)
{
    uint8_t ctrl = gpio_get_ctrl(pin);
    if (ctrl >= GPIO_MAX_CONTROLLERS || !g_gpio[ctrl].initialized) return;
    if (g_gpio[ctrl].ops && g_gpio[ctrl].ops->irq_disable) g_gpio[ctrl].ops->irq_disable(pin);
}

uint16_t gpio_pin_mask(gpio_pin_t pin)
{
    uint32_t pin_idx = pin & 0xFFFFU;
    if (pin_idx >= 16U) {
        return 0U;
    }

    return (uint16_t)(1U << pin_idx);
}

uint8_t gpio_get_bank(gpio_pin_t pin)
{
    return (pin >> 16) & 0xFF;
}
