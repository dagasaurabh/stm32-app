#include "hts221.h"

/* Register map */
#define HTS221_REG_WHO_AM_I   0x0F
#define HTS221_REG_CTRL1      0x20
#define HTS221_REG_STATUS     0x27
#define HTS221_REG_H_OUT_L    0x28
#define HTS221_REG_T_OUT_L    0x2A
#define HTS221_REG_CAL_BASE   0x30

/*
 * HTS221_AI — auto-increment flag for multi-byte register reads.
 *
 * On I2C: bit 7 of the sub-address selects auto-increment.
 * On SPI: bit 7 is the R/W bit (1=read), already set by regmap_spi internally.
 *         Bit 6 is the SPI auto-increment bit, set by regmap_spi_init(ai_bit_mask=0x40).
 *
 * Passing (reg | HTS221_AI) to regmap_read / regmap_read_async therefore works
 * correctly on both buses: the lower 6 bits carry the register address unchanged.
 */
#define HTS221_AI              0x80

static void hts221_async_done(void *ctx, int status);

static void hts221_convert(hts221_t *dev, float *temp_c, float *humidity_pct)
{
    int16_t raw_H = (int16_t)((uint16_t)dev->raw[0] | ((uint16_t)dev->raw[1] << 8));
    int16_t raw_T = (int16_t)((uint16_t)dev->raw[2] | ((uint16_t)dev->raw[3] << 8));

    if (humidity_pct) {
        *humidity_pct = dev->H0_rH +
            (float)(raw_H - dev->H0_T0_OUT) *
            (dev->H1_rH - dev->H0_rH) /
            (float)(dev->H1_T0_OUT - dev->H0_T0_OUT);
    }
    if (temp_c) {
        *temp_c = dev->T0_degC +
            (float)(raw_T - dev->T0_OUT) *
            (dev->T1_degC - dev->T0_degC) /
            (float)(dev->T1_OUT - dev->T0_OUT);
    }
}

int hts221_init(hts221_t *dev, regmap_t *map)
{
    if (!map) return -1;
    dev->map = map;

    /* WHO_AM_I */
    uint8_t who = 0;
    if (regmap_read(map, HTS221_REG_WHO_AM_I, &who, 1) < 0) return -1;
    if (who != HTS221_WHOAMI) return -1;

    /* PD=1 (active), BDU=1, ODR=12.5 Hz */
    uint8_t ctrl = 0x85;
    if (regmap_write(map, HTS221_REG_CTRL1, &ctrl, 1) < 0) return -1;

    /*
     * Read all 16 calibration bytes from 0x30..0x3F in one burst.
     * Layout (index = register - 0x30):
     *   [0]  H0_rH_x2      [1]  H1_rH_x2
     *   [2]  T0_degC_x8_lo [3]  T1_degC_x8_lo  [4] reserved
     *   [5]  T1|T0 MSBs    [6]  H0_T0_OUT_L    [7]  H0_T0_OUT_H
     *   [8-9] reserved     [10] H1_T0_OUT_L    [11] H1_T0_OUT_H
     *   [12] T0_OUT_L      [13] T0_OUT_H       [14] T1_OUT_L  [15] T1_OUT_H
     */
    uint8_t cal[16];
    if (regmap_read(map, HTS221_REG_CAL_BASE | HTS221_AI, cal, 16) < 0) return -1;

    dev->H0_rH = cal[0] / 2.0f;
    dev->H1_rH = cal[1] / 2.0f;

    uint16_t t0_x8 = (uint16_t)cal[2] | ((uint16_t)(cal[5] & 0x03) << 8);
    uint16_t t1_x8 = (uint16_t)cal[3] | ((uint16_t)((cal[5] >> 2) & 0x03) << 8);
    dev->T0_degC = t0_x8 / 8.0f;
    dev->T1_degC = t1_x8 / 8.0f;

    dev->H0_T0_OUT = (int16_t)((uint16_t)cal[6]  | ((uint16_t)cal[7]  << 8));
    dev->H1_T0_OUT = (int16_t)((uint16_t)cal[10] | ((uint16_t)cal[11] << 8));
    dev->T0_OUT    = (int16_t)((uint16_t)cal[12] | ((uint16_t)cal[13] << 8));
    dev->T1_OUT    = (int16_t)((uint16_t)cal[14] | ((uint16_t)cal[15] << 8));

    return 0;
}

int hts221_read(hts221_t *dev, float *temp_c, float *humidity_pct)
{
    /* H_OUT_L(0x28), H_OUT_H, T_OUT_L(0x2A), T_OUT_H — 4 consecutive bytes */
    if (regmap_read(dev->map, HTS221_REG_H_OUT_L | HTS221_AI, dev->raw, 4) < 0)
        return -1;
    hts221_convert(dev, temp_c, humidity_pct);
    return 0;
}

int hts221_read_async(hts221_t *dev, hts221_cb_t cb, void *ctx)
{
    dev->cb     = cb;
    dev->cb_ctx = ctx;
    return regmap_read_async(dev->map, HTS221_REG_H_OUT_L | HTS221_AI,
                             dev->raw, 4, hts221_async_done, dev);
}

static void hts221_async_done(void *ctx, int status)
{
    hts221_t *dev = (hts221_t *)ctx;
    if (status != 0 || !dev->cb) return;

    float t = 0.0f, h = 0.0f;
    hts221_convert(dev, &t, &h);
    dev->cb(dev->cb_ctx, t, h);
}
