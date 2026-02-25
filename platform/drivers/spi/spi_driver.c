#include "spi_driver.h"
#include "spi_hal_if.h"

int spi_init(spi_t *s)
{
    if (!s || !s->hal) return -1;
    return spi_hal_init(s->hal);
}

int spi_write(spi_t *s, const uint8_t *buf, size_t len)
{
    if (!s || !buf || len == 0) return -1;
    return spi_hal_tx(s->hal, buf, len);
}

int spi_read(spi_t *s, uint8_t *buf, size_t len)
{
    if (!s || !buf || len == 0) return -1;
    return spi_hal_rx(s->hal, buf, len);
}

int spi_transfer(spi_t *s, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!s || !tx || !rx || len == 0) return -1;
    return spi_hal_txrx(s->hal, tx, rx, len);
}
