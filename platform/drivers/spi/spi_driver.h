#pragma once

#include <stdint.h>
#include "spi_hal_if.h"

typedef struct {
    void       *hal;  /* SPI_HandleTypeDef * — opaque */
    const char *name;
} spi_t;

int spi_drv_init(spi_t *dev);

int spi_drv_deinit(spi_t *dev);

/*
 * spi_drv_transfer_one — blocking raw segment.
 *
 * tx and/or rx may be NULL:
 *   tx != NULL, rx == NULL --> transmit only
 *   tx == NULL, rx != NULL --> receive only
 *   tx != NULL, rx != NULL --> full-duplex
 */
int spi_drv_transfer_one(spi_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len);

/*
 * spi_drv_transfer_one_it - non-blocking raw segment.
 *
 * Returns immediately; cb(ctx, event, error_flags) is called from ISR on completion.
 * tx and/or rx may be NULL (half-duplex).
 */
int spi_drv_transfer_one_it(spi_t *dev, const uint8_t *tx, uint8_t *rx, 
		uint16_t len, spi_cb_t cb, void *ctx);
