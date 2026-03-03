#pragma once


void board_init(void);

#ifdef BOARD_HAS_CONSOLE
void board_console_init(void);
#endif

#ifdef BOARD_HAS_SPI
#include "gpio.h"
#include "spi_types.h"
void board_spi_init(void);
int  board_spi_add_slave(const char *bus_name, const char *name, gpio_pin_t cs_pin,
        spi_mode_t mode, spi_clkdiv_t prescaler, spi_datasize_t datasize);
#endif
