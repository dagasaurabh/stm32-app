#pragma once

#include <stdint.h>
#include "spi_types.h" /* spi_evt_t, SPI_ERR_*, spi_cb_t */

int
spi_hal_init(void *hal);
int
spi_hal_deinit(void *hal);

/* Blocking - returns when transfer is complete */
int
spi_hal_tx(void *hal, const uint8_t *buf, uint16_t len);
int
spi_hal_rx(void *hal, uint8_t *buf, uint16_t len);
int
spi_hal_transfer(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len);

/*
 * Non-blocking - starts the transfer and returns immediately.
 * cb(ctx, event, error_flags) is called from ISR when the transfer completes.
 * tx and/or rx may be NULL (half-duplex).
 */
int
spi_hal_transfer_it(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len, spi_cb_t cb,
                    void *ctx);
