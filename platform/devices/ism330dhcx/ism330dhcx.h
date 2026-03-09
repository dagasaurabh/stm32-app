#pragma once

/*
 * ISM330DHCX — iNEMO inertial module: 6-axis IMU (accelerometer + gyroscope)
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x6A SA0=low / 0x6B SA0=high)
 *                SPI (rw_bit_mask=0x80, ai_bit_mask=0x00 — native multi-byte)
 * WHO_AM_I     : 0x6B
 *
 * Measurements:
 *   Accelerometer : ±2/±4/±8/±16 g, 16-bit output per axis (X, Y, Z)
 *   Gyroscope     : ±125/±250/±500/±1000/±2000/±4000 dps, 16-bit output per axis
 *
 * Sensitivities (driver defaults):
 *   Accel : 0.000061 g/LSB    at ±2 g range
 *   Gyro  : 0.00875 dps/LSB   at ±250 dps range
 *
 * Key features:
 *   - Independent accel and gyro ODRs: 12.5 Hz to 6.7 kHz
 *   - Hardware finite state machine (FSM) and machine learning core (MLC)
 *   - Built-in anti-aliasing filter for accel; selectable low-pass for gyro
 *   - Low power: 0.55 mA (208 Hz ODR both axes), 5 µA (accel LP1 12.5 Hz)
 *   - On b_u585i_iot02a: SA0 tied HIGH → address 0x6B
 *
 * Driver notes:
 *   - Enabled at 104 Hz ODR (accel ±2 g, gyro ±250 dps) by ism330dhcx_init()
 *   - Gyro and accel data registers read in a single 12-byte burst
 */

#include <stdint.h>
#include "regmap.h"

#define ISM330DHCX_WHOAMI 0x6B

/*
 * cb(ctx, ax, ay, az [g], gx, gy, gz [dps])
 * Accel sensitivity: 0.000061 g/LSB at ±2g
 * Gyro  sensitivity: 0.00875 dps/LSB at 250 dps
 */
typedef void (*ism330dhcx_cb_t)(void *ctx,
                                float ax, float ay, float az,
                                float gx, float gy, float gz);

typedef struct {
    regmap_t *map;

    /* 6 bytes gyro + 6 bytes accel, read in one 12-byte burst */
    uint8_t          raw[12];
    ism330dhcx_cb_t  cb;
    void            *cb_ctx;
} ism330dhcx_t;

int ism330dhcx_init(ism330dhcx_t *dev, regmap_t *map);
int ism330dhcx_read(ism330dhcx_t *dev,
                    float *ax, float *ay, float *az,
                    float *gx, float *gy, float *gz);
int ism330dhcx_read_async(ism330dhcx_t *dev, ism330dhcx_cb_t cb, void *ctx);
