#pragma once

#include "regmap.h"
#include "spi.h"

#define REGMAP_SPI_MAX_DATA 32

/*
 * regmap_spi_t — SPI implementation of the regmap interface.
 *
 * Caller allocates regmap_spi_t (static or on the stack) and passes it to
 * regmap_spi_init(), which wires up the function pointers and returns
 * &self->map.  Sensors receive only the regmap_t * and never see this struct.
 *
 * Handles the standard MEMS SPI register framing:
 *   tx[0] = reg | rw_bit_mask | (len > 1 ? ai_bit_mask : 0)
 *   rx[1..n] = payload (offset 1 past the address byte)
 *
 * rw_bit_mask : bit set for reads  (0x80 for all supported sensors)
 * ai_bit_mask : auto-increment bit (0x40 for HTS221, 0x00 for others)
 *
 * Only one async operation may be in flight at a time per instance.
 *
 * Usage:
 *   static regmap_spi_t g_map;
 *   regmap_t *map = regmap_spi_init(&g_map, spi_open("spi1.hts221"), 0x80, 0x40);
 *   hts221_t *dev = hts221_init(map);
 */
typedef struct {
    regmap_t           map;
    int                fd;
    uint8_t            rw_bit_mask;
    uint8_t            ai_bit_mask;

    /* Async state — one in-flight operation at a time */
    struct spi_message  msg;
    struct spi_transfer xfer;
    uint8_t             tx_buf[REGMAP_SPI_MAX_DATA + 1];
    uint8_t             rx_buf[REGMAP_SPI_MAX_DATA + 1];
    uint8_t            *data_buf;
    uint16_t            data_len;
    regmap_cb_t         cb;
    void               *cb_ctx;
} regmap_spi_t;

/*
 * regmap_spi_init — wire ops into self->map, store fd and bit masks, return &self->map.
 * Returns NULL if self or fd is invalid.
 */
regmap_t *regmap_spi_init(regmap_spi_t *self, int fd,
        uint8_t rw_bit_mask, uint8_t ai_bit_mask);
