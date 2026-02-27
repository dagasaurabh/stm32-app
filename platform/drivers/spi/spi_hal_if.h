#pragma once

#include <stdint.h>

int spi_hal_init(void *hal);
int spi_hal_tx(void *hal, const uint8_t *buf, uint16_t len);
int spi_hal_rx(void *hal, uint8_t *buf, uint16_t len);
int spi_hal_transfer(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len);
