#pragma once       
                   
#include <stdint.h>
#include <stddef.h>

typedef enum {
    GPIO_HAL_MODE_INPUT,
    GPIO_HAL_MODE_OUTPUT,
    GPIO_HAL_MODE_AF,
    GPIO_HAL_MODE_ANALOG,
} gpio_hal_mode_t;

typedef enum {
    GPIO_HAL_PULL_NONE,
    GPIO_HAL_PULL_UP,
    GPIO_HAL_PULL_DOWN,
} gpio_hal_pull_t;

typedef enum {
    GPIO_HAL_IRQ_EDGE_RISING,
    GPIO_HAL_IRQ_EDGE_FALLING,
    GPIO_HAL_IRQ_EDGE_BOTH,
} gpio_hal_irq_edge_t;


void gpio_hal_init(void * port, uint32_t pin,
		gpio_hal_mode_t mode,
		gpio_hal_pull_t pull);

void gpio_hal_write(void * port, uint32_t pin, uint8_t level);
uint8_t gpio_hal_read(void *port, uint32_t pin);
void gpio_hal_toggle(void * port, uint32_t pin);


typedef void (*gpio_hal_irq_cb_t)(void *ctx);

void gpio_hal_irq_config(void *port,
		uint32_t pin,
		gpio_hal_irq_edge_t edge,
		gpio_hal_irq_cb_t cb,
		void *ctx);

void gpio_hal_irq_enable(void *port, uint32_t pin);
void gpio_hal_irq_disable(void *port, uint32_t pin);

