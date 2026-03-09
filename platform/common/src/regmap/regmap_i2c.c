#include "regmap_i2c.h"
#include "i2c_types.h"

static int  regmap_i2c_read      (void *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
static int  regmap_i2c_write     (void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len);
static int  regmap_i2c_read_async(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len,
                                   regmap_cb_t cb, void *cb_ctx);
static void regmap_i2c_async_done(void *ctx, i2c_evt_t event, uint32_t error_flags);

static const regmap_ops_t regmap_i2c_ops = {
    .read       = regmap_i2c_read,
    .write      = regmap_i2c_write,
    .read_async = regmap_i2c_read_async,
};

void regmap_i2c_init(regmap_i2c_t *r, int fd)
{
    r->map.ops = &regmap_i2c_ops;
    r->map.ctx = r;
    r->fd      = fd;
}

static int regmap_i2c_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
    regmap_i2c_t *r = (regmap_i2c_t *)ctx;
    return i2c_mem_read(r->fd, reg, 1, buf, len);
}

static int regmap_i2c_write(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    regmap_i2c_t *r = (regmap_i2c_t *)ctx;
    return i2c_mem_write(r->fd, reg, 1, buf, len);
}

static int regmap_i2c_read_async(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len,
                                   regmap_cb_t cb, void *cb_ctx)
{
    regmap_i2c_t *r = (regmap_i2c_t *)ctx;

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

static void regmap_i2c_async_done(void *ctx, i2c_evt_t event, uint32_t error_flags)
{
    (void)error_flags;
    regmap_i2c_t *r = (regmap_i2c_t *)ctx;
    if (r->cb) {
        r->cb(r->cb_ctx, (event == I2C_EVT_DONE) ? 0 : -1);
    }
}
