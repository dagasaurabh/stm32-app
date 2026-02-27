#include "spi_driver.h"
#include "spi_hal_if.h"

int spi_drv_init(spi_t *dev)
{
    if (!dev || !dev->hal) return -1;
    return spi_hal_init(dev->hal);
}

int spi_drv_tx(spi_t *dev, const uint8_t *buf, uint16_t len)
{
    if (!dev || !dev->hal || !buf || len == 0) return -1;

    return spi_hal_tx(dev->hal, buf, len);
}

int spi_drv_rx(spi_t *dev, uint8_t *buf, uint16_t len)
{
    if (!dev || !dev->hal || !buf || len == 0) return -1;

    return spi_hal_rx(dev->hal, buf, len);
}

int spi_drv_transfer(spi_t *dev, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (!dev || !dev->hal || len == 0) return -1;

    return spi_hal_transfer(dev->hal, tx, rx, len);
}
