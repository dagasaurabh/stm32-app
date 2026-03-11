#pragma once

/*
 * IIS2MDC — High-accuracy, ultra-low-power 3-axis digital magnetometer
 *
 * Manufacturer : STMicroelectronics
 * Interface    : I2C (address 0x1E, fixed) / SPI (rw_bit_mask=0x80, ai_bit_mask=0x00)
 * WHO_AM_I     : 0x40
 * Sensitivity  : 0.0015 gauss/LSB
 */

#include <stdint.h>
#include "regmap.h"
#include "sensor.h"

#define IIS2MDC_WHOAMI 0x40

#ifndef IIS2MDC_MAX_DEVICES
#define IIS2MDC_MAX_DEVICES 1
#endif

typedef struct {
    float mx, my, mz;   /* gauss */
} iis2mdc_data_t;

sensor_t *iis2mdc_init(regmap_t *map);
