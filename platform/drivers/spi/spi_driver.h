#pragma once

#include <stdint.h>

typedef struct {
    void       *hal;  /* SPI_HandleTypeDef * — opaque */
    const char *name;
} spi_t;

int spi_drv_init(spi_t *dev);
int spi_drv_tx(spi_t *dev, const uint8_t *buf, uint16_t len);
int spi_drv_rx(spi_t *dev, uint8_t *buf, uint16_t len);
int spi_drv_transfer(spi_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len);
