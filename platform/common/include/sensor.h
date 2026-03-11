#pragma once

#include <stdint.h>

/*
 * sensor.h — generic sensor interface.
 *
 * sensor_t is a transport-agnostic vtable. It works for any physical sensor
 * regardless of underlying bus: I2C, SPI, ADC, UART, etc.
 *
 * sensor_t MUST be the first member of every sensor driver's internal struct,
 * enabling the driver's vtable functions to cast sensor_t * back to the full
 * driver struct (C11 §6.7.2.1 p15 first-member layout guarantee).
 *
 * Direct use (single sensor, no manager):
 *   sensor_t *s = hts221_init(map);
 *   sensor_read_sync(s, &data);
 *
 * Via manager (multiple sensors, fan-out, streaming):
 *   sensor_mgr_register(SENSOR_HTS221, s, 100, 2);
 */

typedef struct sensor_s sensor_t;

/* Completion callback fired from ISR context when an async read completes */
typedef void (*sensor_done_cb_t)(sensor_t *self, int status, void *ctx);

struct sensor_s {
    /*
     * read_async — arm a non-blocking read.
     * cb is called from ISR on completion. Returns 0 or -EBUSY.
     */
    int  (*read_async)(sensor_t *self, sensor_done_cb_t cb, void *ctx);

    /*
     * read_sync — blocking single-shot read.
     * Converts and writes result directly into out.
     * Returns 0 on success, -1 on error.
     */
    int  (*read_sync) (sensor_t *self, void *out);

    /*
     * get_data — copy the most recently async-read result into out.
     * Only valid after a successful read_async completion.
     */
    void (*get_data)  (sensor_t *self, void *out);

    /*
     * data_size — sizeof(this driver's data struct).
     *
     * Set by each driver's _init to sizeof(sensor_data_t).
     * Checked by sensor_mgr_register() against SENSOR_MAX_DATA_SIZE to catch
     * mismatches at startup before any data is read.
     */
    uint16_t data_size;
};

/* Inline wrappers */

static inline int sensor_read_async(sensor_t *s, sensor_done_cb_t cb, void *ctx)
{
    return (s && s->read_async) ? s->read_async(s, cb, ctx) : -1;
}

static inline int sensor_read_sync(sensor_t *s, void *out)
{
    return (s && s->read_sync) ? s->read_sync(s, out) : -1;
}

static inline void sensor_get_data(sensor_t *s, void *out)
{
    (s && s->get_data) ? s->get_data(s, out) : -1;
}
