#pragma once

#include "gpio.h"

void board_init(void);

#ifdef BOARD_HAS_CONSOLE
void board_console_init(void);
#endif

#ifdef BOARD_HAS_SPI
void board_spi_init(gpio_pin_t cs_pin);
#endif
