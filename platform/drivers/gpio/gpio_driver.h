#pragma once

#include <stdint.h>
#include "gpio_types.h"

/* Opaque handle */
typedef void *gpio_port_t;

/* Driver API */
void
gpio_drv_init_pin(gpio_port_t port, uint32_t pin, gpio_mode_t mode, gpio_pull_t pull,
                  gpio_speed_t speed);

void
gpio_drv_write(gpio_port_t port, uint32_t pin, uint8_t level);

uint8_t
gpio_drv_read(gpio_port_t port, uint32_t pin);

void
gpio_drv_toggle(gpio_port_t port, uint32_t pin);

/* IRQ API */
typedef void (*gpio_drv_irq_cb_t)(void *ctx);

int
gpio_drv_irq_register(gpio_port_t port, uint32_t pin, gpio_irq_edge_t edge, gpio_drv_irq_cb_t cb,
                      void *ctx);

void
gpio_drv_irq_enable(gpio_port_t port, uint32_t pin);
void
gpio_drv_irq_disable(gpio_port_t port, uint32_t pin);
