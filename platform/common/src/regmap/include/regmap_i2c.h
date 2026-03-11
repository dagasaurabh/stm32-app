#pragma once

#include "regmap.h"
#include "i2c.h"

/*
 * regmap_i2c_t — I2C implementation of the regmap interface.
 *
 * Caller allocates regmap_i2c_t (static or on the stack) and passes it to
 * regmap_i2c_init(), which wires up the function pointers and returns
 * &self->map.  Sensors receive only the regmap_t * and never see this struct.
 *
 * Only one async operation may be in flight at a time per instance.
 *
 * Usage:
 *   static regmap_i2c_t g_map;
 *   regmap_t *map = regmap_i2c_init(&g_map, i2c_open("i2c2.hts221"));
 *   hts221_t *dev = hts221_init(map);
 */
typedef struct {
    regmap_t            map;
    int                 fd;

    /* Async state — one in-flight operation at a time */
    struct i2c_message  msg;
    struct i2c_transfer xfer_reg;  /* write: register address byte */
    struct i2c_transfer xfer_data; /* read:  payload buffer */
    uint8_t             reg_buf;   /* scratch: holds reg addr during async op */
    regmap_cb_t         cb;
    void               *cb_ctx;
} regmap_i2c_t;

/*
 * regmap_i2c_init — wire ops into self->map, store fd, return &self->map.
 * Returns NULL if self or fd is invalid.
 */
regmap_t *regmap_i2c_init(regmap_i2c_t *self, int fd);
