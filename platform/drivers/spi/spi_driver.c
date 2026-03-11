#include "spi_driver.h"
#include "spi_hal_if.h"

int
spi_drv_init(spi_t *dev)
{
    if (!dev || !dev->hal)
        return -1;
    return spi_hal_init(dev->hal);
}

int
spi_drv_deinit(spi_t *dev)
{
    if (!dev || !dev->hal)
        return -1;
    return spi_hal_deinit(dev->hal);
}

int
spi_drv_transfer_one(spi_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (!dev || !dev->hal || len == 0)
        return -1;

    if (tx && rx)
        return spi_hal_transfer(dev->hal, tx, rx, len);
    if (tx)
        return spi_hal_tx(dev->hal, tx, len);
    if (rx)
        return spi_hal_rx(dev->hal, rx, len);
    return -1;
}

int
spi_drv_transfer_one_it(spi_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len, spi_cb_t cb,
                        void *ctx)
{
    if (!dev || !dev->hal || len == 0)
        return -1;
    return spi_hal_transfer_it(dev->hal, tx, rx, len, cb, ctx);
}
