#include "regmap_i2c.h"
#include "i2c_types.h"
#include <stddef.h>
#include <errno.h>

/*
 * i2c_err_to_errno — map I2C hardware error flags to POSIX errno codes.
 *
 * This is the only place I2C-specific error knowledge crosses into the
 * regmap abstraction. Everything above (sensor drivers, sensor_mgr, app
 * callbacks) sees only standard errno values — no I2C headers required.
 *
 * Priority order matters when multiple bits are set simultaneously
 * (e.g. NACK + TIMEOUT on a stuck bus); the most actionable error wins:
 *
 *   TIMEOUT → -ETIMEDOUT  bus clock-stretched beyond limit or SCL stuck low;
 *                          likely a hardware fault or missing pull-up.
 *   NACK    → -ENXIO      slave did not acknowledge its address; device may
 *                          be absent, powered off, or at a wrong address.
 *   ARLO    → -EAGAIN     arbitration lost to another master; the transfer
 *                          was not corrupted — caller may retry immediately.
 *   default → -EIO        bus error (misplaced START/STOP) or DMA fault;
 *                          bus state is unknown, recovery may be needed.
 */
static int i2c_err_to_errno(uint32_t flags)
{
    if (flags & I2C_ERR_TIMEOUT) return -ETIMEDOUT;
    if (flags & I2C_ERR_NACK)    return -ENXIO;
    if (flags & I2C_ERR_ARLO)    return -EAGAIN;
    return -EIO;
}

static int regmap_i2c_read (regmap_t *map, uint8_t reg,
        uint8_t *buf, uint16_t len);
static int regmap_i2c_write     (regmap_t *map, uint8_t reg,
        const uint8_t *buf, uint16_t len);
static int regmap_i2c_read_async(regmap_t *map, uint8_t reg,
        uint8_t *buf, uint16_t len, regmap_cb_t cb, void *cb_ctx);
static void regmap_i2c_async_done(void *ctx, i2c_evt_t event, uint32_t error_flags);

regmap_t *regmap_i2c_init(regmap_i2c_t *self, int fd)
{
    if (!self || fd < 0) return NULL;
    self->map.read       = regmap_i2c_read;
    self->map.write      = regmap_i2c_write;
    self->map.read_async = regmap_i2c_read_async;
    self->fd             = fd;
    return &self->map;
}

static int regmap_i2c_read(regmap_t *map, uint8_t reg, uint8_t *buf, uint16_t len)
{
    regmap_i2c_t *r = (regmap_i2c_t *)map;
    return i2c_mem_read(r->fd, reg, 1, buf, len);
}

static int regmap_i2c_write(regmap_t *map, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    regmap_i2c_t *r = (regmap_i2c_t *)map;
    return i2c_mem_write(r->fd, reg, 1, buf, len);
}

static int regmap_i2c_read_async(regmap_t *map, uint8_t reg, uint8_t *buf,
        uint16_t len, regmap_cb_t cb, void *cb_ctx)
{
    regmap_i2c_t *r = (regmap_i2c_t *)map;

    r->reg_buf = reg;
    r->cb      = cb;
    r->cb_ctx  = cb_ctx;

    r->xfer_reg  = (struct i2c_transfer){
        .dir = I2C_DIR_WRITE, .buf = &r->reg_buf, .len = 1
    };
    r->xfer_data = (struct i2c_transfer){
        .dir = I2C_DIR_READ, .buf = buf, .len = len
    };

    i2c_message_init(&r->msg);
    i2c_message_add_transfer(&r->msg, &r->xfer_reg);
    i2c_message_add_transfer(&r->msg, &r->xfer_data);
    r->msg.complete = regmap_i2c_async_done;
    r->msg.context  = r;

    return i2c_async(r->fd, &r->msg);
}

static void regmap_i2c_async_done(void *ctx, i2c_evt_t event,
        uint32_t error_flags)
{
    regmap_i2c_t *r = (regmap_i2c_t *)ctx;
    int status = (event == I2C_EVT_DONE) ? 0 : i2c_err_to_errno(error_flags);
    if (r->cb) r->cb(r->cb_ctx, status);
}

