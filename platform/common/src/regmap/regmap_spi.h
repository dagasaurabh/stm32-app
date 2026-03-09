#pragma once

#include "regmap.h"
#include "spi.h"

#define REGMAP_SPI_MAX_DATA 32

/*
 * regmap_spi_t — SPI implementation of the regmap interface.
 *
 * Handles the standard MEMS SPI register framing:
 *   tx[0] = reg | rw_bit_mask | (len > 1 ? ai_bit_mask : 0)
 *   rx[1..n] = payload (offset 1 past the address byte)
 *
 * rw_bit_mask : bit set for reads  (0x80 for all 4 supported sensors)
 * ai_bit_mask : auto-increment bit (0x40 for HTS221, 0x00 for others)
 *
 * Only one async operation may be in flight at a time per instance.
 * Max data payload: REGMAP_SPI_MAX_DATA bytes (covers all 4 on-board sensors).
 *
 * Usage:
 *   static regmap_spi_t g_map;
 *   regmap_spi_init(&g_map, spi_open("spi1.hts221"), 0x80, 0x40);
 *   sensor_init(&g_dev, &g_map.map);
 *
 * regmap_t is the first member so &g_map.map == (regmap_t *)&g_map.
 */
typedef struct {
    regmap_t           map;        /* MUST be first */
    int                fd;
    uint8_t            rw_bit_mask; /* bit set for reads  (typically 0x80) */
    uint8_t            ai_bit_mask; /* auto-increment bit (0x40 or 0x00)   */

    /* Async state — one in-flight operation at a time */
    struct spi_message  msg;
    struct spi_transfer xfer;
    uint8_t             tx_buf[REGMAP_SPI_MAX_DATA + 1]; /* addr + data */
    uint8_t             rx_buf[REGMAP_SPI_MAX_DATA + 1]; /* mirrored rx */
    uint8_t            *data_buf; /* caller's output buffer */
    uint16_t            data_len;
    regmap_cb_t         cb;
    void               *cb_ctx;
} regmap_spi_t;

void regmap_spi_init(regmap_spi_t *r, int fd,
                     uint8_t rw_bit_mask, uint8_t ai_bit_mask);
