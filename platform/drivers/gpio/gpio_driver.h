#pragma once

#include <stdint.h>

/* Opaque handles */
typedef void* gpio_port_t;

typedef enum {
    GPIO_DRV_MODE_INPUT,
    GPIO_DRV_MODE_OUTPUT,
    GPIO_DRV_MODE_AF,
    GPIO_DRV_MODE_ANALOG,
} gpio_drv_mode_t;

typedef enum {
    GPIO_DRV_PULL_NONE,
    GPIO_DRV_PULL_UP,
    GPIO_DRV_PULL_DOWN,
} gpio_drv_pull_t;

typedef enum {
    GPIO_DRV_IRQ_EDGE_RISING,
    GPIO_DRV_IRQ_EDGE_FALLING,
    GPIO_DRV_IRQ_EDGE_BOTH,
} gpio_drv_irq_edge_t;

/* Driver API */
void gpio_drv_init_pin(gpio_port_t port,
		uint32_t pin,
		gpio_drv_mode_t mode,
		gpio_drv_pull_t pull);

void gpio_drv_write(gpio_port_t port, uint32_t pin, uint8_t level);

uint8_t gpio_drv_read(gpio_port_t port, uint32_t pin);

void gpio_drv_toggle(gpio_port_t port, uint32_t pin);

/* IRQ API */
typedef void (*gpio_drv_irq_cb_t)(gpio_port_t port, uint32_t pin, void *ctx);

int gpio_drv_irq_register(gpio_port_t port,
		uint32_t pin,
		gpio_drv_irq_edge_t edge,
		gpio_drv_irq_cb_t cb,
		void *ctx);

void gpio_drv_irq_enable(gpio_port_t port, uint32_t pin);
void gpio_drv_irq_disable(gpio_port_t port, uint32_t pin);
