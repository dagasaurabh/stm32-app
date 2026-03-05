#pragma once

#include <stdint.h>
#include "i2c_types.h"

/*
 * i2c_hal_if.h — HAL interface for I2C driver.
 *
 * All functions take an opaque void *hal pointer (I2C_HandleTypeDef *).
 * Vendor HAL error codes are mapped to I2C_ERR_* inside i2c_hal.c.
 */

int i2c_hal_init(void *hal);
int i2c_hal_deinit(void *hal);
int i2c_hal_abort(void *hal);

/*
 * Blocking transfers — used by i2c_drv_transfer_one().
 * addr : 8-bit shifted address.
 */
int i2c_hal_master_tx(void *hal, uint16_t addr, const uint8_t *buf, uint16_t len);
int i2c_hal_master_rx(void *hal, uint16_t addr, uint8_t *buf, uint16_t len);

/*
 * Interrupt-driven sequential transfers — used by i2c_drv_transfer_one_it().
 * xfer_options maps to HAL I2C_FIRST_AND_LAST_FRAME / I2C_FIRST_FRAME /
 * I2C_NEXT_FRAME / I2C_LAST_FRAME constants.
 * cb is called from ISR on completion.
 */
int i2c_hal_master_seq_tx_it(void *hal, uint16_t addr, uint8_t *buf, uint16_t len,
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx);
int i2c_hal_master_seq_rx_it(void *hal, uint16_t addr, uint8_t *buf, uint16_t len, 
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx);

/*
 * DMA variants — fall back to IT if DMA is unavailable.
 */
int i2c_hal_master_seq_tx_dma(void *hal, uint16_t addr, uint8_t *buf, uint16_t len,
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx);
int i2c_hal_master_seq_rx_dma(void *hal, uint16_t addr, uint8_t *buf, uint16_t len,
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx);
