#pragma once

/*
 * HTS221 — Capacitive digital relative humidity and temperature sensor
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x5F, fixed — no address pin)
 *                SPI (rw_bit_mask=0x80, ai_bit_mask=0x40)
 * WHO_AM_I     : 0xBC
 *
 * Measurements:
 *   Temperature : -40 to +120 °C, ±0.5 °C accuracy (typ), 16-bit output
 *   Humidity    :   0 to 100 %RH, ±3.5 %RH accuracy (typ), 16-bit output
 *
 * Key features:
 *   - Factory-calibrated; calibration coefficients stored in OTP registers
 *   - Configurable ODR: one-shot, 1 Hz, 7 Hz, 12.5 Hz
 *   - Low power: 2 µA (1 Hz ODR), 0.5 µA (standby)
 *
 * Driver notes:
 *   - hts221_init() reads OTP calibration and applies 2-point linear correction
 *   - Register auto-increment requires OR-ing sub-address with 0x80 on this device
 */

#include <stdint.h>
#include "regmap.h"

#define HTS221_WHOAMI 0xBC

/* cb(ctx, temp_c, humidity_pct) — fired from ISR after async read */
typedef void (*hts221_cb_t)(void *ctx, float temp_c, float humidity_pct);

typedef struct {
    regmap_t *map;

    /* Calibration coefficients — loaded at init */
    float   T0_degC, T1_degC;
    float   H0_rH,   H1_rH;
    int16_t T0_OUT,  T1_OUT;
    int16_t H0_T0_OUT, H1_T0_OUT;

    uint8_t     raw[4];  /* H_OUT_L, H_OUT_H, T_OUT_L, T_OUT_H */
    hts221_cb_t cb;
    void       *cb_ctx;
} hts221_t;

/*
 * hts221_init — WHO_AM_I check, power-on, calibration read.
 * Returns 0 on success, -1 on error (NACK, WHO_AM_I mismatch).
 */
int hts221_init(hts221_t *dev, regmap_t *map);

/*
 * hts221_read — blocking single-shot read.
 * temp_c or humidity_pct may be NULL to skip that output.
 */
int hts221_read(hts221_t *dev, float *temp_c, float *humidity_pct);

/*
 * hts221_read_async — non-blocking; cb(ctx, temp_c, humidity_pct) fires from ISR.
 * Call i2c_poll() / spi_poll() from the main loop to drive transfers.
 */
int hts221_read_async(hts221_t *dev, hts221_cb_t cb, void *ctx);
