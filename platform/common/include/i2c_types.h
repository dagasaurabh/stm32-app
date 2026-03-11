#pragma once

#include <stdint.h>

/*
 * Portable I2C event and error types.
 *
 * Vendor HAL error codes are mapped to I2C_ERR_* inside i2c_hal.c.
 * Upper layers see only these portable values.
 */

typedef enum
{
    I2C_EVT_TX_DONE = 0, /* transmit transfer segment complete  */
    I2C_EVT_RX_DONE = 1, /* receive transfer segment complete   */
    I2C_EVT_DONE = 2,    /* full message complete (facade level) */
    I2C_EVT_ERROR = 3,   /* transfer failed; see error_flags    */
} i2c_evt_t;

#define I2C_ERR_NONE 0x00U    /* no error                          */
#define I2C_ERR_NACK 0x01U    /* NACK from slave                   */
#define I2C_ERR_TIMEOUT 0x02U /* bus timeout                       */
#define I2C_ERR_BERR 0x04U    /* bus error (misplaced START/STOP)  */
#define I2C_ERR_ARLO 0x08U    /* arbitration lost                  */
#define I2C_ERR_DMA 0x10U     /* DMA transfer error                */
#define I2C_ERR_ABORT 0x20U   /* transfer discarded by i2c_reset() */

/*
 * i2c_cb_t — universal I2C completion callback.
 * Always invoked from ISR context — implementation must be ISR-safe.
 */
typedef void (*i2c_cb_t)(void *ctx, i2c_evt_t event, uint32_t error_flags);

typedef enum
{
    I2C_SPEED_STANDARD = 100,   /* 100 kHz */
    I2C_SPEED_FAST = 400,       /* 400 kHz */
    I2C_SPEED_FAST_PLUS = 1000, /* 1 MHz   */
} i2c_speed_t;

typedef enum
{
    I2C_ADDR_7BIT = 0,
    I2C_ADDR_10BIT = 1,
} i2c_addr_mode_t;

typedef enum
{
    I2C_DIR_WRITE = 0,
    I2C_DIR_READ = 1,
} i2c_dir_t;

/*
 * i2c_xfer_opt_t — sequential frame options.
 *
 * Used to map multi-transfer messages to HAL sequential API:
 *   FIRST_AND_LAST : single transfer message (START … STOP)
 *   FIRST          : first in a multi-transfer message (START … no-STOP)
 *   NEXT           : middle transfer (no-START … no-STOP)
 *   LAST           : last transfer (no-START … STOP)
 */
typedef enum
{
    I2C_XFER_FIRST_AND_LAST = 0,
    I2C_XFER_FIRST = 1,
    I2C_XFER_NEXT = 2,
    I2C_XFER_LAST = 3,
} i2c_xfer_opt_t;
