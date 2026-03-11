#include <errno.h>
#include <stdbool.h>
#include "hts221.h"

#define HTS221_REG_WHO_AM_I 0x0F
#define HTS221_REG_STATUS 0x27
#define HTS221_REG_CTRL1 0x20
#define HTS221_REG_H_OUT_L 0x28
#define HTS221_REG_CAL_BASE 0x30
#define HTS221_AI 0x80

/* STATUS register bits */
#define HTS221_STS_T_DA 0x01 /* temperature data available */
#define HTS221_STS_H_DA 0x02 /* humidity data available */
#define HTS221_STS_DRDY (HTS221_STS_T_DA | HTS221_STS_H_DA)

/*
 * Maximum STATUS poll iterations for sync reads.
 * Each i2c_mem_read at 400 kHz takes ~100 µs → 15000 iterations ≈ 1.5 s,
 * which covers the worst case of ODR = 1 Hz plus bus overhead.
 */
#define HTS221_DRDY_MAX_POLLS 15000

struct hts221_s
{
    sensor_t sensor;

    bool initialized;
    regmap_t *map;

    float T0_degC, T1_degC;
    float H0_rH, H1_rH;
    int16_t T0_OUT, T1_OUT;
    int16_t H0_T0_OUT, H1_T0_OUT;

    bool read_pending;
    uint8_t raw[4];

    sensor_done_cb_t pending_cb;
    void *pending_ctx;
};

static struct hts221_s s_pool[HTS221_MAX_DEVICES];

static void
hts221_convert(struct hts221_s *dev, float *temp_c, float *humidity_pct)
{
    int16_t raw_H = (int16_t)((uint16_t)dev->raw[0] | ((uint16_t)dev->raw[1] << 8));
    int16_t raw_T = (int16_t)((uint16_t)dev->raw[2] | ((uint16_t)dev->raw[3] << 8));

    if (humidity_pct)
        *humidity_pct = dev->H0_rH + (float)(raw_H - dev->H0_T0_OUT) * (dev->H1_rH - dev->H0_rH) /
                                         (float)(dev->H1_T0_OUT - dev->H0_T0_OUT);

    if (temp_c)
        *temp_c = dev->T0_degC + (float)(raw_T - dev->T0_OUT) * (dev->T1_degC - dev->T0_degC) /
                                     (float)(dev->T1_OUT - dev->T0_OUT);
}

static void
hts221_regmap_done(void *ctx, int status)
{
    struct hts221_s *dev = (struct hts221_s *)ctx;
    dev->read_pending = false;
    sensor_done_cb_t cb = dev->pending_cb;
    void *pctx = dev->pending_ctx;
    dev->pending_cb = NULL;
    dev->pending_ctx = NULL;
    if (cb)
        cb(&dev->sensor, status, pctx);
}

/*
 * hts221_data_ready — poll STATUS register for a single iteration.
 * Returns 1 if both T_DA and H_DA are set, 0 if not, -1 on I2C error.
 * Callers decide whether to loop (sync) or bail (async).
 */
static int
hts221_data_ready(struct hts221_s *dev)
{
    uint8_t status = 0;
    if (regmap_read(dev->map, HTS221_REG_STATUS, &status, 1) < 0)
        return -1;
    return ((status & HTS221_STS_DRDY) == HTS221_STS_DRDY) ? 1 : 0;
}

static int
hts221_ops_read_async(sensor_t *self, sensor_done_cb_t cb, void *ctx)
{
    struct hts221_s *dev = (struct hts221_s *)self;
    if (dev->read_pending)
        return -EBUSY;

    /*
     * Check data-ready before arming the async transfer.
     * Called from sensor_mgr_poll (main loop, not ISR), so a synchronous
     * STATUS register read is safe.  If data is not yet available, return
     * -EAGAIN so sensor_mgr re-arms after interval_ms (avoids callbacks
     * with stale reset-state values on the first ODR cycle).
     */
    int rdy = hts221_data_ready(dev);
    if (rdy < 0)
        return -EIO;
    if (rdy == 0)
        return -EAGAIN;

    dev->read_pending = true;
    dev->pending_cb = cb;
    dev->pending_ctx = ctx;
    return regmap_read_async(dev->map, HTS221_REG_H_OUT_L | HTS221_AI, dev->raw, 4,
                             hts221_regmap_done, dev);
}

static int
hts221_ops_read_sync(sensor_t *self, void *out)
{
    struct hts221_s *dev = (struct hts221_s *)self;
    hts221_data_t *data = (hts221_data_t *)out;

    /* Wait for both humidity and temperature data to be available. */
    int polls = 0;
    for (;;)
    {
        int rdy = hts221_data_ready(dev);
        if (rdy < 0)
            return -EIO;
        if (rdy > 0)
            break;
        if (++polls >= HTS221_DRDY_MAX_POLLS)
            return -ETIMEDOUT;
    }

    if (regmap_read(dev->map, HTS221_REG_H_OUT_L | HTS221_AI, dev->raw, 4) < 0)
        return -EIO;
    hts221_convert(dev, &data->temp_c, &data->humidity_pct);
    return 0;
}

static void
hts221_ops_get_data(sensor_t *self, void *out)
{
    struct hts221_s *dev = (struct hts221_s *)self;
    hts221_data_t *data = (hts221_data_t *)out;
    hts221_convert(dev, &data->temp_c, &data->humidity_pct);
}

sensor_t *
hts221_init(regmap_t *map)
{
    if (!map)
        return NULL;

    for (int i = 0; i < HTS221_MAX_DEVICES; i++)
    {
        if (s_pool[i].initialized && s_pool[i].map == map)
            return &s_pool[i].sensor;
    }

    struct hts221_s *dev = NULL;
    for (int i = 0; i < HTS221_MAX_DEVICES; i++)
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
    if (regmap_read(map, HTS221_REG_WHO_AM_I, &who, 1) < 0)
        return NULL;
    if (who != HTS221_WHOAMI)
        return NULL;

    uint8_t ctrl = 0x85;
    if (regmap_write(map, HTS221_REG_CTRL1, &ctrl, 1) < 0)
        return NULL;

    uint8_t cal[16];
    if (regmap_read(map, HTS221_REG_CAL_BASE | HTS221_AI, cal, 16) < 0)
        return NULL;

    dev->H0_rH = cal[0] / 2.0f;
    dev->H1_rH = cal[1] / 2.0f;

    uint16_t t0_x8 = (uint16_t)cal[2] | ((uint16_t)(cal[5] & 0x03) << 8);
    uint16_t t1_x8 = (uint16_t)cal[3] | ((uint16_t)((cal[5] >> 2) & 0x03) << 8);
    dev->T0_degC = t0_x8 / 8.0f;
    dev->T1_degC = t1_x8 / 8.0f;

    dev->H0_T0_OUT = (int16_t)((uint16_t)cal[6] | ((uint16_t)cal[7] << 8));
    dev->H1_T0_OUT = (int16_t)((uint16_t)cal[10] | ((uint16_t)cal[11] << 8));
    dev->T0_OUT = (int16_t)((uint16_t)cal[12] | ((uint16_t)cal[13] << 8));
    dev->T1_OUT = (int16_t)((uint16_t)cal[14] | ((uint16_t)cal[15] << 8));

    dev->sensor.read_async = hts221_ops_read_async;
    dev->sensor.read_sync = hts221_ops_read_sync;
    dev->sensor.get_data = hts221_ops_get_data;
    dev->sensor.data_size = sizeof(hts221_data_t);
    dev->initialized = true;
    return &dev->sensor;
}
