#include "lps22hh.h"

#define LPS22HH_REG_WHO_AM_I  0x0F
#define LPS22HH_REG_CTRL1     0x10
#define LPS22HH_REG_PRESS_XL  0x28

static void lps22hh_async_done(void *ctx, int status);

static void lps22hh_convert(lps22hh_t *dev, float *pressure_hpa, float *temp_c)
{
    /* Pressure: 24-bit unsigned, 1 LSB = 1/4096 hPa */
    uint32_t raw_p = (uint32_t)dev->raw[0] |
                     ((uint32_t)dev->raw[1] << 8) |
                     ((uint32_t)dev->raw[2] << 16);
    /* Temperature: 16-bit signed, 1 LSB = 1/100 °C */
    int16_t raw_t = (int16_t)((uint16_t)dev->raw[3] | ((uint16_t)dev->raw[4] << 8));

    if (pressure_hpa) *pressure_hpa = (float)raw_p / 4096.0f;
    if (temp_c)       *temp_c       = (float)raw_t / 100.0f;
}

int lps22hh_init(lps22hh_t *dev, regmap_t *map)
{
    if (!map) return -1;
    dev->map = map;

    uint8_t who = 0;
    if (regmap_read(map, LPS22HH_REG_WHO_AM_I, &who, 1) < 0) return -1;
    if (who != LPS22HH_WHOAMI) return -1;

    /* ODR = 1 Hz, BDU disabled */
    uint8_t ctrl = 0x10;
    if (regmap_write(map, LPS22HH_REG_CTRL1, &ctrl, 1) < 0) return -1;

    return 0;
}

int lps22hh_read(lps22hh_t *dev, float *pressure_hpa, float *temp_c)
{
    /* 5 consecutive bytes: PRESS_XL(0x28), PRESS_L, PRESS_H, TEMP_L, TEMP_H */
    if (regmap_read(dev->map, LPS22HH_REG_PRESS_XL, dev->raw, 5) < 0)
        return -1;
    lps22hh_convert(dev, pressure_hpa, temp_c);
    return 0;
}

int lps22hh_read_async(lps22hh_t *dev, lps22hh_cb_t cb, void *ctx)
{
    dev->cb     = cb;
    dev->cb_ctx = ctx;
    return regmap_read_async(dev->map, LPS22HH_REG_PRESS_XL,
                             dev->raw, 5, lps22hh_async_done, dev);
}

static void lps22hh_async_done(void *ctx, int status)
{
    lps22hh_t *dev = (lps22hh_t *)ctx;
    if (status != 0 || !dev->cb) return;

    float p = 0.0f, t = 0.0f;
    lps22hh_convert(dev, &p, &t);
    dev->cb(dev->cb_ctx, p, t);
}
