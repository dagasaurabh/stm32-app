#pragma once

#include <stddef.h>
#include <stdint.h>

int spi_hal_init(void *hal);
int spi_hal_tx(void *hal, const uint8_t *buf, size_t len);
int spi_hal_rx(void *hal, uint8_t *buf, size_t len);
int spi_hal_txrx(void *hal, const uint8_t *tx, uint8_t *rx, size_t len);
