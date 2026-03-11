#pragma once

#include <stdint.h>

/*
 * regmap — bus-agnostic register-map abstraction.
 *
 * Provides a uniform read/write interface over any register-mapped device,
 * regardless of whether the underlying bus is I2C, SPI, or anything else.
 * Sensor drivers call only regmap_read(), regmap_write(), and
 * regmap_read_async(); the bus implementation is selected at init time by
 * the application or board layer.
 *
 * Scope: devices with 8-bit register addresses (covers virtually all MEMS
 * environmental/motion sensors).  Devices with 16-bit addresses, command-
 * based protocols (QSPI), or no register model require a different layer.
 */

/*
 * regmap_cb_t — completion callback for async reads.
 * Always called from ISR context.
 *   status : 0 = success, -1 = error
 */
typedef void (*regmap_cb_t)(void *ctx, int status);

/*
 * regmap_t — bus handle passed to sensor drivers.
 *
 * Function pointers are embedded directly; no separate ops table or void *ctx.
 * Concrete implementations (regmap_i2c_t / regmap_spi_t) place regmap_t as
 * their first member so that a regmap_t * can be cast back to the concrete
 * type inside each operation:
 *
 *   regmap_i2c_t *r = (regmap_i2c_t *)map;   // valid: base is first member
 */
typedef struct regmap_s {
    int (*read) (struct regmap_s *map, uint8_t reg, uint8_t *buf, uint16_t len);
    int (*write) (struct regmap_s *map, uint8_t reg, const uint8_t *buf, uint16_t len);
    int (*read_async)(struct regmap_s *map, uint8_t reg, uint8_t *buf, uint16_t len,
            regmap_cb_t cb, void *cb_ctx);
} regmap_t;

/* Inline wrappers */

static inline int regmap_read(regmap_t *m, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return m->read(m, reg, buf, len);
}

static inline int regmap_write(regmap_t *m, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    return m->write(m, reg, buf, len);
}

static inline int regmap_read_async(regmap_t *m, uint8_t reg, uint8_t *buf, uint16_t len,
        regmap_cb_t cb, void *cb_ctx)
{
    return m->read_async(m, reg, buf, len, cb, cb_ctx);
}
