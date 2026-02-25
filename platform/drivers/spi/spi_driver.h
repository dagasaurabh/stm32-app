#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *hal;         /* HAL handle (opaque) */
    const char *name;  /* "SPI0", "SPI1", ... */
} spi_t;

/* Driver API */
int spi_init(spi_t *s);
int spi_write(spi_t *s, const uint8_t *buf, size_t len);
int spi_read(spi_t *s, uint8_t *buf, size_t len);
int spi_transfer(spi_t *s, const uint8_t *tx, uint8_t *rx, size_t len);
