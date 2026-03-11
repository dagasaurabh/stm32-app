#pragma once
#include <stdint.h>

typedef enum
{
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_OUTPUT_OD,
    GPIO_MODE_AF,
    GPIO_MODE_AF_OD,
    GPIO_MODE_ANALOG
} gpio_mode_t;

typedef enum
{
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} gpio_pull_t;

typedef enum
{
    GPIO_SPEED_LOW,
    GPIO_SPEED_MEDIUM,
    GPIO_SPEED_HIGH,
    GPIO_SPEED_VERY_HIGH
} gpio_speed_t;

typedef enum
{
    GPIO_IRQ_EDGE_RISING,
    GPIO_IRQ_EDGE_FALLING,
    GPIO_IRQ_EDGE_BOTH
} gpio_irq_edge_t;
