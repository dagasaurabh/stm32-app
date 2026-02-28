#pragma once

#include <stdint.h>

/*
 * Portable SPI event and error types.
 *
 * Used by both the facade (spi.h) and the HAL interface (spi_hal_if.h)
 * without either depending on the other.  Vendor HAL error codes are
 * mapped to SPI_ERR_* inside spi_hal.c — nowhere else.
 */

/*
 * spi_evt_t — which interrupt completion callback fired.
 *
 * At the HAL/driver layer: reflects the specific ISR that ran.
 * At the facade layer (spi_message.complete): SPI_EVT_TXRX_DONE means
 * the whole message succeeded; SPI_EVT_ERROR means at least one
 * transfer failed (inspect error_flags for the cause).
 */
typedef enum {
    SPI_EVT_TX_DONE   = 0,  /* transmit-only transfer complete  */
    SPI_EVT_RX_DONE   = 1,  /* receive-only transfer complete   */
    SPI_EVT_TXRX_DONE = 2,  /* full-duplex transfer complete    */
    SPI_EVT_ERROR     = 3,  /* transfer failed; see error_flags */
} spi_evt_t;

/*
 * SPI_ERR_* — portable error flag bitmask.
 *
 * Values are NOT tied to any vendor's HAL constants.  spi_hal.c maps
 * vendor-specific error codes to these flags; upper layers see only these.
 *
 * Non-zero only when event == SPI_EVT_ERROR.
 */
#define SPI_ERR_NONE  0x00U  /* no error                   */
#define SPI_ERR_MODF  0x01U  /* mode fault (SS low in master mode) */
#define SPI_ERR_CRC   0x02U  /* CRC mismatch               */
#define SPI_ERR_OVR   0x04U  /* overrun (data lost)        */
#define SPI_ERR_FRE   0x08U  /* frame format error         */
#define SPI_ERR_DMA   0x10U  /* DMA transfer error         */

/*
 * spi_cb_t — universal SPI completion callback.
 *
 *   ctx         : opaque pointer supplied by the caller
 *   event       : which event occurred
 *   error_flags : SPI_ERR_* bitmask; meaningful only when event == SPI_EVT_ERROR
 *
 * Always invoked from ISR context — implementation must be ISR-safe.
 */
typedef void (*spi_cb_t)(void *ctx, spi_evt_t event, uint32_t error_flags);
