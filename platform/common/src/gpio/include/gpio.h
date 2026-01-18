#pragma once
#include <stdint.h>

typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_AF,
    GPIO_MODE_ANALOG,
} gpio_mode_t;

typedef enum {
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
} gpio_pull_t;

typedef enum {
    GPIO_LOW = 0,
    GPIO_HIGH = 1,
} gpio_level_t;

typedef enum {
    GPIO_IRQ_EDGE_RISING,
    GPIO_IRQ_EDGE_FALLING,
    GPIO_IRQ_EDGE_BOTH,
} gpio_irq_edge_t;


typedef uint32_t gpio_pin_t;

typedef void (*gpio_irq_cb_t)(gpio_pin_t pin, void *ctx);

struct gpio_ops {
    void (*init)(gpio_pin_t, gpio_mode_t, gpio_pull_t);
    void (*write)(gpio_pin_t, gpio_level_t);
    gpio_level_t (*read)(gpio_pin_t);
    void (*toggle)(gpio_pin_t);
	int  (*irq_register)(gpio_pin_t, gpio_irq_edge_t, gpio_irq_cb_t, void *);
    void (*irq_enable)(gpio_pin_t);
    void (*irq_disable)(gpio_pin_t);
};


void gpio_register(const struct gpio_ops *);

void gpio_init(gpio_pin_t, gpio_mode_t, gpio_pull_t);

void gpio_write(gpio_pin_t, gpio_level_t);

gpio_level_t gpio_read(gpio_pin_t);

void gpio_toggle(gpio_pin_t);

int gpio_irq_register(gpio_pin_t, gpio_irq_edge_t, gpio_irq_cb_t, void *);

void gpio_irq_enable(gpio_pin_t);

void gpio_irq_diable(gpio_pin_t);

/*
 * Encoding: [31:16] = port index, [15:0] = pin number
 * GPIOA --> 0
 * GPIOB --> 1
 * GPIOC --> 2
 * GPIOD --> 3
 * GPIOE --> 4
 * ...
 */
#define GPIO_PIN_ENCODE(port, pin) (((port) << 16) | (pin))

uint16_t gpio_get_pin(gpio_pin_t);

uint16_t gpio_get_port(gpio_pin_t);
