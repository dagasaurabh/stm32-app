#pragma once
#include <stdbool.h>

typedef enum {
    GPIO_OUT,
    GPIO_IN
} gpio_dir_t;

void gpio_init(int pin, gpio_dir_t dir);
void gpio_write(int pin, bool value);
bool gpio_read(int pin);

