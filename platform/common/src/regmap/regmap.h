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
 * regmap_ops_t — bus implementation function table.
 * All three ops are required.
 * read / write are synchronous (blocking).
 * read_async starts a non-blocking read; cb fires from ISR on completion.
 */
typedef struct {
    int (*read)      (void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
    int (*write)     (void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);
    int (*read_async)(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len,
                      regmap_cb_t cb, void *cb_ctx);
} regmap_ops_t;

/*
 * regmap_t — handle passed to sensor drivers.
 *
 * Concrete implementations (regmap_i2c_t / regmap_spi_t) place regmap_t
 * as their first member so that &impl.map == (regmap_t *)&impl.
 */
typedef struct {
    const regmap_ops_t *ops;
    void               *ctx;
} regmap_t;

/* Inline wrappers — zero overhead, no visible function-pointer indirection */

static inline int regmap_read(regmap_t *m, uint8_t reg,
                               uint8_t *buf, uint16_t len)
{
    return m->ops->read(m->ctx, reg, buf, len);
}

static inline int regmap_write(regmap_t *m, uint8_t reg,
                                const uint8_t *buf, uint16_t len)
{
    return m->ops->write(m->ctx, reg, buf, len);
}

static inline int regmap_read_async(regmap_t *m, uint8_t reg,
                                     uint8_t *buf, uint16_t len,
                                     regmap_cb_t cb, void *cb_ctx)
{
    return m->ops->read_async(m->ctx, reg, buf, len, cb, cb_ctx);
}
