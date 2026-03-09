#include "ism330dhcx.h"

#define ISM330DHCX_REG_WHO_AM_I  0x0F
#define ISM330DHCX_REG_CTRL1_XL  0x10   /* Accel control */
#define ISM330DHCX_REG_CTRL2_G   0x11   /* Gyro control  */
#define ISM330DHCX_REG_OUTX_L_G  0x22   /* Gyro output base  (6 bytes) */
#define ISM330DHCX_REG_OUTX_L_A  0x28   /* Accel output base (6 bytes) */

/* Sensitivity at chosen full-scale settings */
#define ISM330DHCX_ACCEL_SENS  0.000061f  /* g/LSB  at ±2g    */
#define ISM330DHCX_GYRO_SENS   0.00875f   /* dps/LSB at 250dps */

static void ism330dhcx_async_done(void *ctx, int status);

static void ism330dhcx_convert(ism330dhcx_t *dev,
                               float *ax, float *ay, float *az,
                               float *gx, float *gy, float *gz)
{
    /* raw[0..5] = gyro X,Y,Z (int16 LE each) */
    int16_t gxr = (int16_t)((uint16_t)dev->raw[0]  | ((uint16_t)dev->raw[1]  << 8));
    int16_t gyr = (int16_t)((uint16_t)dev->raw[2]  | ((uint16_t)dev->raw[3]  << 8));
    int16_t gzr = (int16_t)((uint16_t)dev->raw[4]  | ((uint16_t)dev->raw[5]  << 8));
    /* raw[6..11] = accel X,Y,Z (int16 LE each) */
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

int ism330dhcx_init(ism330dhcx_t *dev, regmap_t *map)
{
    if (!map) return -1;
    dev->map = map;

    uint8_t who = 0;
    if (regmap_read(map, ISM330DHCX_REG_WHO_AM_I, &who, 1) < 0) return -1;
    if (who != ISM330DHCX_WHOAMI) return -1;

    /* Accel: ODR=104 Hz, ±2g full-scale */
    uint8_t ctrl = 0x40;
    if (regmap_write(map, ISM330DHCX_REG_CTRL1_XL, &ctrl, 1) < 0) return -1;

    /* Gyro: ODR=104 Hz, 250 dps full-scale */
    if (regmap_write(map, ISM330DHCX_REG_CTRL2_G, &ctrl, 1) < 0) return -1;

    return 0;
}

int ism330dhcx_read(ism330dhcx_t *dev,
                    float *ax, float *ay, float *az,
                    float *gx, float *gy, float *gz)
{
    /* Read 12 bytes: OUTX_L_G(0x22) through OUTZ_H_A(0x2D) consecutively */
    if (regmap_read(dev->map, ISM330DHCX_REG_OUTX_L_G, dev->raw, 12) < 0)
        return -1;
    ism330dhcx_convert(dev, ax, ay, az, gx, gy, gz);
    return 0;
}

int ism330dhcx_read_async(ism330dhcx_t *dev, ism330dhcx_cb_t cb, void *ctx)
{
    dev->cb     = cb;
    dev->cb_ctx = ctx;
    return regmap_read_async(dev->map, ISM330DHCX_REG_OUTX_L_G,
                             dev->raw, 12, ism330dhcx_async_done, dev);
}

static void ism330dhcx_async_done(void *ctx, int status)
{
    ism330dhcx_t *dev = (ism330dhcx_t *)ctx;
    if (status != 0 || !dev->cb) return;

    float ax, ay, az, gx, gy, gz;
    ism330dhcx_convert(dev, &ax, &ay, &az, &gx, &gy, &gz);
    dev->cb(dev->cb_ctx, ax, ay, az, gx, gy, gz);
}
