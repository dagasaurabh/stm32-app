#pragma once

/*
 * HTS221 — Capacitive digital relative humidity and temperature sensor
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x5F, fixed) / SPI (rw=0x80, ai=0x40)
 * WHO_AM_I     : 0xBC
 *
 * hts221_init() returns a sensor_t * for use with sensor_read_sync/async
 * directly or via sensor_mgr_register(). The driver owns all internal state.
 */

#include <stdint.h>
#include "regmap.h"
#include "sensor.h"

#define HTS221_WHOAMI 0xBC

#ifndef HTS221_MAX_DEVICES
#define HTS221_MAX_DEVICES 1
#endif

typedef struct {
    float temp_c;
    float humidity_pct;
} hts221_data_t;

/*
 * hts221_init — WHO_AM_I check, power-on, calibration read.
 * Returns sensor_t * on success, NULL on error or pool exhaustion.
 */
sensor_t *hts221_init(regmap_t *map);
