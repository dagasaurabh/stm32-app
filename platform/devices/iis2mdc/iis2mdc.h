#pragma once

/*
 * IIS2MDC — High-accuracy, ultra-low-power 3-axis digital magnetometer
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x1E, fixed — no address pin)
 *                SPI (rw_bit_mask=0x80, ai_bit_mask=0x00 — native multi-byte)
 * WHO_AM_I     : 0x40
 *
 * Measurements:
 *   Magnetic field : ±50 gauss full-scale, 16-bit output per axis (X, Y, Z)
 *   Sensitivity    : 0.0015 gauss/LSB (1.5 mGauss/LSB)
 *
 * Key features:
 *   - Configurable ODR: 10, 20, 50, 100 Hz
 *   - Continuous and single-measurement modes
 *   - Hard-iron and soft-iron correction via offset registers
 *   - Ultra-low power: 100 µA (10 Hz ODR), 1 µA (idle)
 *   - Integrated temperature sensor (not exposed by this driver)
 *
 * Driver notes:
 *   - Enabled in continuous mode at 100 Hz ODR by iis2mdc_init()
 *   - Output registers auto-increment natively; no sub-address OR needed
 *   - Gauss conversion: raw_int16 × 0.0015 gauss/LSB
 */

#include <stdint.h>
#include "regmap.h"

#define IIS2MDC_WHOAMI 0x40

/* cb(ctx, mx, my, mz [gauss]) — sensitivity: 0.0015 gauss/LSB */
typedef void (*iis2mdc_cb_t)(void *ctx, float mx, float my, float mz);

typedef struct {
    regmap_t *map;

    uint8_t     raw[6];  /* OUTX_L, OUTX_H, OUTY_L, OUTY_H, OUTZ_L, OUTZ_H */
    iis2mdc_cb_t cb;
    void        *cb_ctx;
} iis2mdc_t;

int iis2mdc_init(iis2mdc_t *dev, regmap_t *map);
int iis2mdc_read(iis2mdc_t *dev, float *mx, float *my, float *mz);
int iis2mdc_read_async(iis2mdc_t *dev, iis2mdc_cb_t cb, void *ctx);
