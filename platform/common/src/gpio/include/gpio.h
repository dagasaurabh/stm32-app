#pragma once
#include <stdint.h>
#include "gpio_types.h"

typedef enum
{
    GPIO_LOW = 0,
    GPIO_HIGH = 1,
} gpio_level_t;

typedef uint32_t gpio_pin_t;

typedef void (*gpio_irq_cb_t)(gpio_pin_t pin, void *ctx);

struct gpio_ops
{
    void (*init)(gpio_pin_t, gpio_mode_t, gpio_pull_t, gpio_speed_t);
    void (*write)(gpio_pin_t, gpio_level_t);
    gpio_level_t (*read)(gpio_pin_t);
    void (*toggle)(gpio_pin_t);
    int (*irq_register)(gpio_pin_t, gpio_irq_edge_t, gpio_irq_cb_t, void *);
    void (*irq_enable)(gpio_pin_t);
    void (*irq_disable)(gpio_pin_t);
};

/* Controller IDs */
#define GPIO_CTRL_ONCHIP 0
#define GPIO_MAX_CONTROLLERS 4

int
gpio_register(uint8_t ctrl_id, const struct gpio_ops *ops);

uint8_t
gpio_get_ctrl(gpio_pin_t pin);

void gpio_init(gpio_pin_t, gpio_mode_t, gpio_pull_t, gpio_speed_t);

void gpio_write(gpio_pin_t, gpio_level_t);

gpio_level_t gpio_read(gpio_pin_t);

void gpio_toggle(gpio_pin_t);

int
gpio_irq_register(gpio_pin_t, gpio_irq_edge_t, gpio_irq_cb_t, void *);

void gpio_irq_enable(gpio_pin_t);

void gpio_irq_disable(gpio_pin_t);

/*
 * Encoding: [31:24] = ctrl index | [23:16] = bank index | [15:0] = pin number
 * bank is a binary index of a pin group within a controller:
 *   on-chip: bank = port index (GPIOA=0, GPIOB=1, ...)
 *   external I/O expander: bank = expander bank index
 * GPIO_PIN_ENCODE(port, pin) produces ctrl=0 (on-chip) by construction.
 * GPIOA --> 0
 * GPIOB --> 1
 * GPIOC --> 2
 * GPIOD --> 3
 * GPIOE --> 4
 * ...
 */
#define GPIO_PIN_ENCODE(port, pin) (((uint32_t)(port) << 16) | (uint32_t)(pin))

/* Extended encoding with explicit controller index */
#define GPIO_PIN_ENCODE_EXT(ctrl, bank, pin)                                                       \
    (((uint32_t)(ctrl) << 24) | ((uint32_t)(bank) << 16) | (uint32_t)(pin))

/* Returns the HAL pin bitmask (1 << pin_index) for use with GPIO_TypeDef */
uint16_t gpio_pin_mask(gpio_pin_t);

uint8_t gpio_get_bank(gpio_pin_t);
