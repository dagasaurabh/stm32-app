#pragma once

#include <stdint.h>
#include "i2c_types.h"
#include "gpio.h"    /* gpio_pin_t — used for bit-bang bus recovery */

typedef struct {
    void       *hal;       /* I2C_HandleTypeDef * — opaque              */
    const char *name;      /* "i2c1", "i2c2", …                        */
    gpio_pin_t  scl_pin;   /* used by bit-bang bus recovery             */
    gpio_pin_t  sda_pin;
} i2c_t;

int i2c_drv_init(i2c_t *dev);
int i2c_drv_deinit(i2c_t *dev);

/*
 * i2c_drv_transfer_one — blocking raw transfer.
 * addr : 8-bit shifted I2C address
 * Returns 0 on success, -1 on error.
 */
int i2c_drv_transfer_one(i2c_t *dev, uint16_t addr, i2c_dir_t dir,
        uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt);

/*
 * i2c_drv_transfer_one_it — non-blocking raw transfer.
 *
 * Returns immediately; cb(ctx, event, error_flags) called from ISR on completion.
 * use_dma: request DMA variant; falls back to IT if DMA fails.
 */
int i2c_drv_transfer_one_it(i2c_t *dev, uint16_t addr, i2c_dir_t dir, uint8_t *buf,
        uint16_t len, i2c_xfer_opt_t opt, uint8_t use_dma, i2c_cb_t cb, void *ctx);

/*
 * i2c_drv_abort — abort any in-progress transfer (soft reset).
 */
int i2c_drv_abort(i2c_t *dev);
