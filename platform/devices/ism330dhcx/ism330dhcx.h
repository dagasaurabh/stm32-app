#pragma once

/*
 * ISM330DHCX — iNEMO inertial module: 6-axis IMU (accel + gyro)
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (0x6A SA0=low / 0x6B SA0=high) / SPI (rw=0x80, ai=0x00)
 * WHO_AM_I     : 0x6B
 * On b_u585i_iot02a: SA0 tied HIGH → address 0x6B
 *
 * ism330dhcx_data_t is 6 floats = 24 bytes = SENSOR_MAX_DATA_SIZE.
 */

#include <stdint.h>
#include "regmap.h"
#include "sensor.h"

#define ISM330DHCX_WHOAMI 0x6B

#ifndef ISM330DHCX_MAX_DEVICES
#define ISM330DHCX_MAX_DEVICES 1
#endif

typedef struct {
    float ax, ay, az;   /* g   — 0.000061 g/LSB at ±2g  */
    float gx, gy, gz;   /* dps — 0.00875 dps/LSB at 250 dps */
} ism330dhcx_data_t;

sensor_t *ism330dhcx_init(regmap_t *map);
