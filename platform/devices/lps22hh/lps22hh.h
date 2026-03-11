#pragma once

/*
 * LPS22HH — MEMS nano pressure sensor (barometric + temperature)
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (0x5C SA0=low / 0x5D SA0=high) / SPI (rw=0x80, ai=0x00)
 * WHO_AM_I     : 0xB3
 * On b_u585i_iot02a: SA0 tied HIGH → address 0x5D
 */

#include <stdint.h>
#include "regmap.h"
#include "sensor.h"

#define LPS22HH_WHOAMI 0xB3

#ifndef LPS22HH_MAX_DEVICES
#define LPS22HH_MAX_DEVICES 1
#endif

typedef struct
{
    float pressure_hpa;
    float temp_c;
} lps22hh_data_t;

sensor_t *
lps22hh_init(regmap_t *map);
