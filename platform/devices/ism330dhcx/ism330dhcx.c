#include <errno.h>
#include <stdbool.h>
#include "ism330dhcx.h"

#define ISM330DHCX_REG_WHO_AM_I  0x0F
#define ISM330DHCX_REG_CTRL1_XL  0x10
#define ISM330DHCX_REG_CTRL2_G   0x11
#define ISM330DHCX_REG_OUTX_L_G  0x22
#define ISM330DHCX_REG_OUTX_L_A  0x28

#define ISM330DHCX_ACCEL_SENS  0.000061f
#define ISM330DHCX_GYRO_SENS   0.00875f

struct ism330dhcx_s {
    sensor_t     sensor;

    bool         initialized;
    regmap_t    *map;

    bool    read_pending;
    uint8_t raw[12];

    sensor_done_cb_t pending_cb;
    void            *pending_ctx;
};

static struct ism330dhcx_s s_pool[ISM330DHCX_MAX_DEVICES];

static void ism330dhcx_convert(struct ism330dhcx_s *dev,
        float *ax, float *ay, float *az,
        float *gx, float *gy, float *gz)
{
    int16_t gxr = (int16_t)((uint16_t)dev->raw[0]  | ((uint16_t)dev->raw[1]  << 8));
    int16_t gyr = (int16_t)((uint16_t)dev->raw[2]  | ((uint16_t)dev->raw[3]  << 8));
    int16_t gzr = (int16_t)((uint16_t)dev->raw[4]  | ((uint16_t)dev->raw[5]  << 8));
    int16_t axr = (int16_t)((uint16_t)dev->raw[6]  | ((uint16_t)dev->raw[7]  << 8));
    int16_t ayr = (int16_t)((uint16_t)dev->raw[8]  | ((uint16_t)dev->raw[9]  << 8));
    int16_t azr = (int16_t)((uint16_t)dev->raw[10] | ((uint16_t)dev->raw[11] << 8));

    if (gx) *gx = (float)gxr * ISM330DHCX_GYRO_SENS;
    if (gy) *gy = (float)gyr * ISM330DHCX_GYRO_SENS;
    if (gz) *gz = (float)gzr * ISM330DHCX_GYRO_SENS;
    if (ax) *ax = (float)axr * ISM330DHCX_ACCEL_SENS;
    if (ay) *ay = (float)ayr * ISM330DHCX_ACCEL_SENS;
    if (az) *az = (float)azr * ISM330DHCX_ACCEL_SENS;
}

static void ism330dhcx_regmap_done(void *ctx, int status)
{
    struct ism330dhcx_s *dev = (struct ism330dhcx_s *)ctx;
    dev->read_pending = false;
    sensor_done_cb_t cb   = dev->pending_cb;
    void            *pctx = dev->pending_ctx;
    dev->pending_cb  = NULL;
    dev->pending_ctx = NULL;
    if (cb) cb(&dev->sensor, status, pctx);
}

static int ism330dhcx_ops_read_async(sensor_t *self,
        sensor_done_cb_t cb, void *ctx)
{
    struct ism330dhcx_s *dev = (struct ism330dhcx_s *)self;
    if (dev->read_pending) return -EBUSY;
    dev->read_pending = true;
    dev->pending_cb   = cb;
    dev->pending_ctx  = ctx;
    return regmap_read_async(dev->map, ISM330DHCX_REG_OUTX_L_G,
                             dev->raw, 12, ism330dhcx_regmap_done, dev);
}

static int ism330dhcx_ops_read_sync(sensor_t *self, void *out)
{
    struct ism330dhcx_s *dev  = (struct ism330dhcx_s *)self;
    ism330dhcx_data_t   *data = (ism330dhcx_data_t *)out;
    if (regmap_read(dev->map, ISM330DHCX_REG_OUTX_L_G, dev->raw, 12) < 0) return -1;
    ism330dhcx_convert(dev,
                       &data->ax, &data->ay, &data->az,
                       &data->gx, &data->gy, &data->gz);
    return 0;
}

static void ism330dhcx_ops_get_data(sensor_t *self, void *out)
{
    struct ism330dhcx_s *dev  = (struct ism330dhcx_s *)self;
    ism330dhcx_data_t   *data = (ism330dhcx_data_t *)out;
    ism330dhcx_convert(dev,
                       &data->ax, &data->ay, &data->az,
                       &data->gx, &data->gy, &data->gz);
}

sensor_t *ism330dhcx_init(regmap_t *map)
{
    if (!map) return NULL;

    for (int i = 0; i < ISM330DHCX_MAX_DEVICES; i++) {
        if (s_pool[i].initialized && s_pool[i].map == map) return &s_pool[i].sensor;
    }

    struct ism330dhcx_s *dev = NULL;
    for (int i = 0; i < ISM330DHCX_MAX_DEVICES; i++) {
        if (!s_pool[i].initialized) { dev = &s_pool[i]; break; }
    }
    if (!dev) return NULL;

    dev->map = map;

    uint8_t who = 0;
    if (regmap_read(map, ISM330DHCX_REG_WHO_AM_I, &who, 1) < 0) return NULL;
    if (who != ISM330DHCX_WHOAMI) return NULL;

    uint8_t ctrl = 0x40;
    if (regmap_write(map, ISM330DHCX_REG_CTRL1_XL, &ctrl, 1) < 0) return NULL;
    if (regmap_write(map, ISM330DHCX_REG_CTRL2_G,  &ctrl, 1) < 0) return NULL;

    dev->sensor.read_async = ism330dhcx_ops_read_async;
    dev->sensor.read_sync  = ism330dhcx_ops_read_sync;
    dev->sensor.get_data   = ism330dhcx_ops_get_data;
    dev->sensor.data_size  = sizeof(ism330dhcx_data_t);
    dev->initialized       = true;
    return &dev->sensor;
}
