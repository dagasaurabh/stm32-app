#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "sensor.h"

#ifndef BOARD_MAX_SENSORS
#define BOARD_MAX_SENSORS 0
#endif

#ifndef BOARD_MAX_SUBS
#define BOARD_MAX_SUBS 0
#endif

#if defined(BOARD_HAS_SENSORS) && (BOARD_MAX_SENSORS == 0)
#error                                                                                             \
    "BOARD_HAS_SENSORS is defined but BOARD_MAX_SENSORS is 0. Set BOARD_MAX_SENSORS to the number of sensors in your board CMakeLists.txt."
#endif

#if defined(BOARD_HAS_SENSORS) && (BOARD_MAX_SUBS == 0)
#error                                                                                             \
    "BOARD_HAS_SENSORS is defined but BOARD_MAX_SUBS is 0. Set BOARD_MAX_SUBS to the total number of subscriber slots needed."
#endif

/*
 * SENSOR_MAX_DATA_SIZE — size of the per-entry data buffer in sensor_entry_t.
 *
 * MUST be >= sizeof(largest sensor data struct) used with this manager.
 * Current largest: ism330dhcx_data_t (6 floats = 24 bytes).
 *
 * When adding a new sensor driver, check sizeof(new_sensor_data_t) and
 * increase this constant if necessary.  Failure to do so causes sensor_get_data
 * to write beyond the buffer — silent memory corruption, no compile error.
 */
#define SENSOR_MAX_DATA_SIZE 24

/*
 * sensor_id_t — type tags for the known sensor families.
 * Passed to sensor_mgr_register() for metadata/logging only.
 * NOT used as a lookup key — multiple instances of the same type are allowed.
 */
typedef enum
{
    SENSOR_HTS221 = 0,
    SENSOR_LPS22HH,
    SENSOR_ISM330DHCX,
    SENSOR_IIS2MDC,
    SENSOR_ID_MAX
} sensor_id_t;

typedef struct _sensor_manager sensor_manager_t; /* opaque */

/*
 * sensor_data_cb_t — delivered to all subscribers when a read completes.
 *
 *   handle : instance handle returned by sensor_mgr_register()
 *   data   : pointer to the sensor's data struct; NULL on error
 *   status : 0 = success, negative errno on failure (-EIO, -ETIMEDOUT, …)
 *   ctx    : subscriber's context pointer
 *
 * ISR CONTEXT WARNING: This callback fires from interrupt context (I2C/SPI ISR).
 * The callback MUST NOT:
 *   - Call any i2c_* / spi_* function
 *   - Call sensor_mgr_read(), sensor_mgr_stream_start()
 *   - Call printf(), malloc(), or any non-reentrant library function
 *   - Block or spin-wait
 * The callback SHOULD:
 *   - Copy data to volatile/shared variables for main-loop consumption
 *   - Set a volatile flag to notify the main loop
 *   - Keep execution time minimal (microseconds, not milliseconds)
 *
 * BUS FAULT RECOVERY:
 *   sensor_mgr stops streaming after SENSOR_MGR_MAX_ERRORS consecutive failures
 *   to avoid hammering a dead bus.  The app is responsible for recovery:
 *   detect consecutive errors in the callback (or a watchdog in the main loop),
 *   call i2c_reset(fd) / spi_reset(fd) on the affected bus, then call
 *   sensor_mgr_stream_start(handle) to resume.  The app may register a dedicated
 *   subscriber callback solely for bus health monitoring alongside a data callback.
 */
typedef void (*sensor_data_cb_t)(int handle, const void *data, int status, void *ctx);

/*
 * sensor_mgr_register — register a sensor instance.
 *   type        : sensor family tag (sensor_id_t), used for logging only
 *   device      : sensor_t * (first member of sensor driver struct)
 *   interval_ms : minimum ms between reads in streaming mode
 *   n_subs      : subscriber slots to reserve from the pool
 *
 * Returns a non-negative handle on success, negative errno on failure.
 * Pass the handle to all subsequent sensor_mgr_* calls.
 * Multiple instances of the same type may be registered independently.
 */
int
sensor_mgr_register(sensor_id_t type, sensor_t *device, uint32_t interval_ms, uint8_t n_subs);
int
sensor_mgr_subscribe(int handle, sensor_data_cb_t cb, void *ctx);
int
sensor_mgr_unsubscribe(int handle, sensor_data_cb_t cb);

/* Blocking single-shot read — result written into out */
int
sensor_mgr_read_sync(int handle, void *out);

/* One-shot async pull — subscribers notified on completion */
int
sensor_mgr_read(int handle);
int
sensor_mgr_stream_start(int handle);
int
sensor_mgr_stream_stop(int handle);

/*
 * sensor_mgr_reset — clear error state and stop streaming.
 *
 * Resets error_count to 0 and sets streaming = false.
 * Call after i2c_reset()/spi_reset() to prepare for sensor_mgr_stream_start().
 */
int
sensor_mgr_reset(int handle);

/* Call from main loop */
void
sensor_mgr_poll(void);
