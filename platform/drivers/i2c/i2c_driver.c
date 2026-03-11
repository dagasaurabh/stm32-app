#include "i2c_driver.h"
#include "i2c_hal_if.h"

int i2c_drv_init(i2c_t *dev)
{
    if (!dev || !dev->hal) return -1;
    return i2c_hal_init(dev->hal);
}

int i2c_drv_deinit(i2c_t *dev)
{
    if (!dev || !dev->hal) return -1;
    return i2c_hal_deinit(dev->hal);
}

int i2c_drv_transfer_one(i2c_t *dev, uint16_t addr, i2c_dir_t dir,
        uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt)
{
    if (!dev || !dev->hal || !buf || len == 0) return -1;
    (void)opt;	/* blocking API always issues START+STOP */
    if (dir == I2C_DIR_WRITE) return i2c_hal_master_tx(dev->hal, addr, buf, len);
    else                      return i2c_hal_master_rx(dev->hal, addr, buf, len);
}

int i2c_drv_transfer_one_it(i2c_t *dev, uint16_t addr, i2c_dir_t dir, 
        uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt, uint8_t use_dma,
        i2c_cb_t cb, void *ctx)
{
    if (!dev || !dev->hal || !buf || len == 0 || !cb) return -1;

    if (dir == I2C_DIR_WRITE) {
        if (use_dma) return i2c_hal_master_seq_tx_dma(dev->hal, addr, buf, len, opt, cb, ctx);
        return i2c_hal_master_seq_tx_it(dev->hal, addr, buf, len, opt, cb, ctx);
    } else {
        if (use_dma) return i2c_hal_master_seq_rx_dma(dev->hal, addr, buf, len, opt, cb, ctx);
        return i2c_hal_master_seq_rx_it(dev->hal, addr, buf, len, opt, cb, ctx);
    }
}

int i2c_drv_abort(i2c_t *dev)
{
    if (!dev || !dev->hal) return -1;
    return i2c_hal_abort(dev->hal);
}
