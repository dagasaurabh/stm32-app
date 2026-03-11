#include <errno.h>
#include <stdbool.h>
#include "lps22hh.h"

#define LPS22HH_REG_WHO_AM_I 0x0F
#define LPS22HH_REG_CTRL1 0x10
#define LPS22HH_REG_PRESS_XL 0x28

struct lps22hh_s
{
    sensor_t sensor;

    bool initialized;
    regmap_t *map;

    bool read_pending;
    uint8_t raw[5];

    sensor_done_cb_t pending_cb;
    void *pending_ctx;
};

static struct lps22hh_s s_pool[LPS22HH_MAX_DEVICES];

static void
lps22hh_convert(struct lps22hh_s *dev, float *pressure_hpa, float *temp_c)
{
    uint32_t raw_p =
        (uint32_t)dev->raw[0] | ((uint32_t)dev->raw[1] << 8) | ((uint32_t)dev->raw[2] << 16);
    int16_t raw_t = (int16_t)((uint16_t)dev->raw[3] | ((uint16_t)dev->raw[4] << 8));
    if (pressure_hpa)
        *pressure_hpa = (float)raw_p / 4096.0f;
    if (temp_c)
        *temp_c = (float)raw_t / 100.0f;
}

static void
lps22hh_regmap_done(void *ctx, int status)
{
    struct lps22hh_s *dev = (struct lps22hh_s *)ctx;
    dev->read_pending = false;
    sensor_done_cb_t cb = dev->pending_cb;
    void *pctx = dev->pending_ctx;
    dev->pending_cb = NULL;
    dev->pending_ctx = NULL;
    if (cb)
        cb(&dev->sensor, status, pctx);
}

static int
lps22hh_ops_read_async(sensor_t *self, sensor_done_cb_t cb, void *ctx)
{
    struct lps22hh_s *dev = (struct lps22hh_s *)self;
    if (dev->read_pending)
        return -EBUSY;
    dev->read_pending = true;
    dev->pending_cb = cb;
    dev->pending_ctx = ctx;
    return regmap_read_async(dev->map, LPS22HH_REG_PRESS_XL, dev->raw, 5, lps22hh_regmap_done, dev);
}

static int
lps22hh_ops_read_sync(sensor_t *self, void *out)
{
    struct lps22hh_s *dev = (struct lps22hh_s *)self;
    lps22hh_data_t *data = (lps22hh_data_t *)out;
    if (regmap_read(dev->map, LPS22HH_REG_PRESS_XL, dev->raw, 5) < 0)
        return -1;
    lps22hh_convert(dev, &data->pressure_hpa, &data->temp_c);
    return 0;
}

static void
lps22hh_ops_get_data(sensor_t *self, void *out)
{
    struct lps22hh_s *dev = (struct lps22hh_s *)self;
    lps22hh_data_t *data = (lps22hh_data_t *)out;
    lps22hh_convert(dev, &data->pressure_hpa, &data->temp_c);
}

sensor_t *
lps22hh_init(regmap_t *map)
{
    if (!map)
        return NULL;

    for (int i = 0; i < LPS22HH_MAX_DEVICES; i++)
    {
        if (s_pool[i].initialized && s_pool[i].map == map)
            return &s_pool[i].sensor;
    }

    struct lps22hh_s *dev = NULL;
    for (int i = 0; i < LPS22HH_MAX_DEVICES; i++)
    {
        if (!s_pool[i].initialized)
        {
            dev = &s_pool[i];
            break;
        }
    }
    if (!dev)
        return NULL;

    dev->map = map;

    uint8_t who = 0;
    if (regmap_read(map, LPS22HH_REG_WHO_AM_I, &who, 1) < 0)
        return NULL;
    if (who != LPS22HH_WHOAMI)
        return NULL;

    uint8_t ctrl = 0x10;
    if (regmap_write(map, LPS22HH_REG_CTRL1, &ctrl, 1) < 0)
        return NULL;

    dev->sensor.read_async = lps22hh_ops_read_async;
    dev->sensor.read_sync = lps22hh_ops_read_sync;
    dev->sensor.get_data = lps22hh_ops_get_data;
    dev->sensor.data_size = sizeof(lps22hh_data_t);
    dev->initialized = true;
    return &dev->sensor;
}
