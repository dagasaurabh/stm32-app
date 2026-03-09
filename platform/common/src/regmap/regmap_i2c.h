#pragma once

#include "regmap.h"
#include "i2c.h"

/*
 * regmap_i2c_t — I2C implementation of the regmap interface.
 *
 * Owns the async state (i2c_message, transfers, register address scratch
 * buffer) that previously lived inside each sensor struct.  After migrating
 * to regmap, sensor structs contain only bus-agnostic fields.
 *
 * Only one async operation may be in flight at a time per instance.
 *
 * Usage:
 *   static regmap_i2c_t g_map;
 *   regmap_i2c_init(&g_map, i2c_open("i2c1.sensor"));
 *   sensor_init(&g_dev, &g_map.map);
 *
 * regmap_t is the first member so &g_map.map == (regmap_t *)&g_map.
 */
typedef struct {
    regmap_t            map;       /* MUST be first */
    int                 fd;

    /* Async state — one in-flight operation at a time */
    struct i2c_message  msg;
    struct i2c_transfer xfer_reg;  /* write: register address byte */
    struct i2c_transfer xfer_data; /* read:  payload buffer */
    uint8_t             reg_buf;   /* scratch: holds reg addr during async op */
    regmap_cb_t         cb;
    void               *cb_ctx;
} regmap_i2c_t;

void regmap_i2c_init(regmap_i2c_t *r, int fd);
