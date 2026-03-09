#include "iis2mdc.h"

#define IIS2MDC_REG_WHO_AM_I  0x4F
#define IIS2MDC_REG_CFG_A     0x60
#define IIS2MDC_REG_CFG_C     0x62
#define IIS2MDC_REG_OUTX_L    0x68

#define IIS2MDC_MAG_SENS      0.0015f  /* gauss/LSB */

static void iis2mdc_async_done(void *ctx, int status);

static void iis2mdc_convert(iis2mdc_t *dev, float *mx, float *my, float *mz)
{
    int16_t xr = (int16_t)((uint16_t)dev->raw[0] | ((uint16_t)dev->raw[1] << 8));
    int16_t yr = (int16_t)((uint16_t)dev->raw[2] | ((uint16_t)dev->raw[3] << 8));
    int16_t zr = (int16_t)((uint16_t)dev->raw[4] | ((uint16_t)dev->raw[5] << 8));

    if (mx) *mx = (float)xr * IIS2MDC_MAG_SENS;
    if (my) *my = (float)yr * IIS2MDC_MAG_SENS;
    if (mz) *mz = (float)zr * IIS2MDC_MAG_SENS;
}

int iis2mdc_init(iis2mdc_t *dev, regmap_t *map)
{
    if (!map) return -1;
    dev->map = map;

    uint8_t who = 0;
    if (regmap_read(map, IIS2MDC_REG_WHO_AM_I, &who, 1) < 0) return -1;
    if (who != IIS2MDC_WHOAMI) return -1;

    /* Soft reset — clears all registers to defaults; SOFT_RST bit
     * auto-clears when reset is complete (~5 ms typical) */
    uint8_t rst = 0x20;  /* SOFT_RST bit in CFG_A */
    if (regmap_write(map, IIS2MDC_REG_CFG_A, &rst, 1) < 0) return -1;

    uint8_t val;
    int retries = 500;
    do {
        val = 0x20;
        regmap_read(map, IIS2MDC_REG_CFG_A, &val, 1);
    } while ((val & 0x20) && --retries > 0);
    if (retries == 0) return -1;

    /* BDU=1: block data update — output registers not updated until both
     * MSB and LSB have been read, preventing mismatched high/low bytes */
    uint8_t cfg_c = 0x10;
    if (regmap_write(map, IIS2MDC_REG_CFG_C, &cfg_c, 1) < 0) return -1;

    /* Continuous mode, 100 Hz ODR (ODR[1:0]=11, MD[1:0]=00) */
    uint8_t cfg_a = 0x0C;
    if (regmap_write(map, IIS2MDC_REG_CFG_A, &cfg_a, 1) < 0) return -1;

    return 0;
}

int iis2mdc_read(iis2mdc_t *dev, float *mx, float *my, float *mz)
{
    /* 6 bytes: OUTX_L(0x68), OUTX_H, OUTY_L, OUTY_H, OUTZ_L, OUTZ_H */
    if (regmap_read(dev->map, IIS2MDC_REG_OUTX_L, dev->raw, 6) < 0)
        return -1;
    iis2mdc_convert(dev, mx, my, mz);
    return 0;
}

int iis2mdc_read_async(iis2mdc_t *dev, iis2mdc_cb_t cb, void *ctx)
{
    dev->cb     = cb;
    dev->cb_ctx = ctx;
    return regmap_read_async(dev->map, IIS2MDC_REG_OUTX_L,
                             dev->raw, 6, iis2mdc_async_done, dev);
}

static void iis2mdc_async_done(void *ctx, int status)
{
    iis2mdc_t *dev = (iis2mdc_t *)ctx;
    if (status != 0 || !dev->cb) return;

    float mx, my, mz;
    iis2mdc_convert(dev, &mx, &my, &mz);
    dev->cb(dev->cb_ctx, mx, my, mz);
}
