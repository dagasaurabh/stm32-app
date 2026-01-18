#include "gpio.h"

__attribute__((weak))
void gpio_init(int pin, gpio_dir_t dir)
{
    (void)pin;
    (void)dir;
}

__attribute__((weak))
void gpio_write(int pin, bool value)
{
    (void)pin;
    (void)value;
}

__attribute__((weak))
bool gpio_read(int pin)
{
    (void)pin;
    return false;
}

