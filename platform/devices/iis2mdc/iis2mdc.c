#include <errno.h>
#include <stdbool.h>
#include "iis2mdc.h"

#define IIS2MDC_REG_WHO_AM_I 0x4F
#define IIS2MDC_REG_CFG_A 0x60
#define IIS2MDC_REG_CFG_C 0x62
#define IIS2MDC_REG_OUTX_L 0x68

#define IIS2MDC_MAG_SENS 0.0015f

struct iis2mdc_s
{
    sensor_t sensor;

    bool initialized;
    regmap_t *map;

    bool read_pending;
    uint8_t raw[6];

    sensor_done_cb_t pending_cb;
    void *pending_ctx;
};

static struct iis2mdc_s s_pool[IIS2MDC_MAX_DEVICES];

static void
iis2mdc_convert(struct iis2mdc_s *dev, float *mx, float *my, float *mz)
{
    int16_t xr = (int16_t)((uint16_t)dev->raw[0] | ((uint16_t)dev->raw[1] << 8));
    int16_t yr = (int16_t)((uint16_t)dev->raw[2] | ((uint16_t)dev->raw[3] << 8));
    int16_t zr = (int16_t)((uint16_t)dev->raw[4] | ((uint16_t)dev->raw[5] << 8));

    if (mx)
        *mx = (float)xr * IIS2MDC_MAG_SENS;
    if (my)
        *my = (float)yr * IIS2MDC_MAG_SENS;
    if (mz)
        *mz = (float)zr * IIS2MDC_MAG_SENS;
}

static void
iis2mdc_regmap_done(void *ctx, int status)
{
    struct iis2mdc_s *dev = (struct iis2mdc_s *)ctx;
    dev->read_pending = false;
    sensor_done_cb_t cb = dev->pending_cb;
    void *pctx = dev->pending_ctx;
    dev->pending_cb = NULL;
    dev->pending_ctx = NULL;
    if (cb)
        cb(&dev->sensor, status, pctx);
}

static int
iis2mdc_ops_read_async(sensor_t *self, sensor_done_cb_t cb, void *ctx)
{
    struct iis2mdc_s *dev = (struct iis2mdc_s *)self;
    if (dev->read_pending)
        return -EBUSY;
    dev->read_pending = true;
    dev->pending_cb = cb;
    dev->pending_ctx = ctx;
    return regmap_read_async(dev->map, IIS2MDC_REG_OUTX_L, dev->raw, 6, iis2mdc_regmap_done, dev);
}

static int
iis2mdc_ops_read_sync(sensor_t *self, void *out)
{
    struct iis2mdc_s *dev = (struct iis2mdc_s *)self;
    iis2mdc_data_t *data = (iis2mdc_data_t *)out;
    if (regmap_read(dev->map, IIS2MDC_REG_OUTX_L, dev->raw, 6) < 0)
        return -1;
    iis2mdc_convert(dev, &data->mx, &data->my, &data->mz);
    return 0;
}

static void
iis2mdc_ops_get_data(sensor_t *self, void *out)
{
    struct iis2mdc_s *dev = (struct iis2mdc_s *)self;
    iis2mdc_data_t *data = (iis2mdc_data_t *)out;
    iis2mdc_convert(dev, &data->mx, &data->my, &data->mz);
}

sensor_t *
iis2mdc_init(regmap_t *map)
{
    if (!map)
        return NULL;

    for (int i = 0; i < IIS2MDC_MAX_DEVICES; i++)
    {
        if (s_pool[i].initialized && s_pool[i].map == map)
            return &s_pool[i].sensor;
    }

    struct iis2mdc_s *dev = NULL;
    for (int i = 0; i < IIS2MDC_MAX_DEVICES; i++)
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
    if (regmap_read(map, IIS2MDC_REG_WHO_AM_I, &who, 1) < 0)
        return NULL;
    if (who != IIS2MDC_WHOAMI)
        return NULL;

    uint8_t rst = 0x20;
    if (regmap_write(map, IIS2MDC_REG_CFG_A, &rst, 1) < 0)
        return NULL;

    uint8_t val;
    int retries = 500;
    do
    {
        val = 0x20;
        regmap_read(map, IIS2MDC_REG_CFG_A, &val, 1);
    } while ((val & 0x20) && --retries > 0);
    if (retries == 0)
        return NULL;

    uint8_t cfg_c = 0x10;
    if (regmap_write(map, IIS2MDC_REG_CFG_C, &cfg_c, 1) < 0)
        return NULL;

    uint8_t cfg_a = 0x0C;
    if (regmap_write(map, IIS2MDC_REG_CFG_A, &cfg_a, 1) < 0)
        return NULL;

    dev->sensor.read_async = iis2mdc_ops_read_async;
    dev->sensor.read_sync = iis2mdc_ops_read_sync;
    dev->sensor.get_data = iis2mdc_ops_get_data;
    dev->sensor.data_size = sizeof(iis2mdc_data_t);
    dev->initialized = true;
    return &dev->sensor;
}
