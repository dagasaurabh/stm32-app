#pragma once

/*
 * LPS22HH — MEMS nano pressure sensor (barometric + temperature)
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x5C SA0=low / 0x5D SA0=high)
 *                SPI (rw_bit_mask=0x80, ai_bit_mask=0x00 — native multi-byte)
 * WHO_AM_I     : 0xB3
 *
 * Measurements:
 *   Pressure    : 260 to 1260 hPa, ±0.5 hPa accuracy (typ), 24-bit output
 *   Temperature : -40 to +85 °C,  ±1.5 °C accuracy (typ), 16-bit output
 *
 * Key features:
 *   - Configurable ODR: one-shot, 1, 10, 25, 50, 75, 100, 200 Hz
 *   - Built-in low-pass filter for noise reduction at high ODR
 *   - Low power: 3 µA (1 Hz ODR), 1 µA (power-down)
 *   - On b_u585i_iot02a: SA0 tied HIGH → address 0x5D
 *
 * Driver notes:
 *   - Enabled at 1 Hz ODR by lps22hh_init(); use one-shot for lower power
 *   - Pressure raw is a signed 24-bit value; divide by 4096 for hPa
 *   - Temperature raw is signed 16-bit; divide by 100 for °C
 */

#include <stdint.h>
#include "regmap.h"

#define LPS22HH_WHOAMI 0xB3

/* cb(ctx, pressure_hpa, temp_c) — fired from ISR after async read */
typedef void (*lps22hh_cb_t)(void *ctx, float pressure_hpa, float temp_c);

typedef struct {
    regmap_t *map;

    uint8_t      raw[5];  /* PRESS_XL, PRESS_L, PRESS_H, TEMP_L, TEMP_H */
    lps22hh_cb_t cb;
    void        *cb_ctx;
} lps22hh_t;

/*
 * lps22hh_init — WHO_AM_I check + 1 Hz ODR enable.
 * Returns 0 on success, -1 on error.
 */
int lps22hh_init(lps22hh_t *dev, regmap_t *map);

/*
 * lps22hh_read — blocking single-shot read.
 * pressure_hpa or temp_c may be NULL to skip.
 */
int lps22hh_read(lps22hh_t *dev, float *pressure_hpa, float *temp_c);

/*
 * lps22hh_read_async — non-blocking; cb fires from ISR after poll() drives transfer.
 */
int lps22hh_read_async(lps22hh_t *dev, lps22hh_cb_t cb, void *ctx);
