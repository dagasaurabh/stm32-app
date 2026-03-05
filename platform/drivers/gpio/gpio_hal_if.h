#pragma once

#include <stdint.h>
#include <stddef.h>
#include "gpio_types.h"

/* gpio_hal_mode_t and gpio_hal_speed_t keep their own prefixes to avoid
 * clashing with STM32 HAL legacy macros:
 *   GPIO_MODE_INPUT, GPIO_MODE_ANALOG, GPIO_MODE_OUTPUT_OD ...
 *   GPIO_SPEED_LOW, GPIO_SPEED_HIGH (= GPIO_SPEED_FREQ_VERY_HIGH in legacy!) */
typedef enum {
    GPIO_HAL_MODE_INPUT,
    GPIO_HAL_MODE_OUTPUT,
    GPIO_HAL_MODE_OUTPUT_OD,
    GPIO_HAL_MODE_AF,
    GPIO_HAL_MODE_AF_OD,
    GPIO_HAL_MODE_ANALOG,
} gpio_hal_mode_t;

typedef enum {
    GPIO_HAL_SPEED_LOW,
    GPIO_HAL_SPEED_MEDIUM,
    GPIO_HAL_SPEED_HIGH,
    GPIO_HAL_SPEED_VERY_HIGH,
} gpio_hal_speed_t;


void gpio_hal_init(void *port, uint32_t pin,
		gpio_hal_mode_t mode,
		gpio_pull_t pull,
		gpio_hal_speed_t speed);

void gpio_hal_write(void *port, uint32_t pin, uint8_t level);
uint8_t gpio_hal_read(void *port, uint32_t pin);
void gpio_hal_toggle(void *port, uint32_t pin);


typedef void (*gpio_hal_irq_cb_t)(void *ctx);

void gpio_hal_irq_config(void *port,
		uint32_t pin,
		gpio_irq_edge_t edge,
		gpio_hal_irq_cb_t cb,
		void *ctx);

void gpio_hal_irq_enable(void *port, uint32_t pin);
void gpio_hal_irq_disable(void *port, uint32_t pin);
