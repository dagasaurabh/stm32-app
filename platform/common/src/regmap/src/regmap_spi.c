#include <string.h>
#include <errno.h>
#include "regmap_spi.h"
#include "spi_types.h"

static int regmap_spi_read(regmap_t *map, uint8_t reg,
        uint8_t *buf, uint16_t len);
static int regmap_spi_write(regmap_t *map, uint8_t reg,
        const uint8_t *buf, uint16_t len);
static int regmap_spi_read_async(regmap_t *map, uint8_t reg, uint8_t *buf,
        uint16_t len, regmap_cb_t cb, void *cb_ctx);
static void regmap_spi_async_done(void *ctx, spi_evt_t event, uint32_t error_flags);

regmap_t *regmap_spi_init(regmap_spi_t *self, int fd,
                           uint8_t rw_bit_mask, uint8_t ai_bit_mask)
{
    if (!self || fd < 0) return NULL;
    self->map.read       = regmap_spi_read;
    self->map.write      = regmap_spi_write;
    self->map.read_async = regmap_spi_read_async;
    self->fd             = fd;
    self->rw_bit_mask    = rw_bit_mask;
    self->ai_bit_mask    = ai_bit_mask;
    return &self->map;
}

static int regmap_spi_read(regmap_t *map, uint8_t reg, uint8_t *buf, uint16_t len)
{
    regmap_spi_t *r = (regmap_spi_t *)map;

    uint8_t tx[REGMAP_SPI_MAX_DATA + 1] = {0};
    uint8_t rx[REGMAP_SPI_MAX_DATA + 1] = {0};

    tx[0] = reg | r->rw_bit_mask | (len > 1 ? r->ai_bit_mask : 0);

    struct spi_transfer xfer = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len    = (uint16_t)(len + 1),
    };

    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_transfer(&msg, &xfer);

    int ret = spi_sync(r->fd, &msg);
    if (ret == 0) memcpy(buf, &rx[1], len);
    return ret;
}

static int regmap_spi_write(regmap_t *map, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    regmap_spi_t *r = (regmap_spi_t *)map;

    uint8_t tx[REGMAP_SPI_MAX_DATA + 1];
    tx[0] = reg | (len > 1 ? r->ai_bit_mask : 0);
    memcpy(&tx[1], buf, len);

    struct spi_transfer xfer = {
        .tx_buf = tx,
        .rx_buf = NULL,
        .len    = (uint16_t)(len + 1),
    };

    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_transfer(&msg, &xfer);

    return spi_sync(r->fd, &msg);
}

static int regmap_spi_read_async(regmap_t *map, uint8_t reg,
        uint8_t *buf, uint16_t len, regmap_cb_t cb, void *cb_ctx)
{
    regmap_spi_t *r = (regmap_spi_t *)map;

    r->data_buf = buf;
    r->data_len = len;
    r->cb       = cb;
    r->cb_ctx   = cb_ctx;

    memset(r->tx_buf, 0, len + 1);
    r->tx_buf[0] = reg | r->rw_bit_mask | (len > 1 ? r->ai_bit_mask : 0);

    r->xfer = (struct spi_transfer){
        .tx_buf = r->tx_buf,
        .rx_buf = r->rx_buf,
        .len    = (uint16_t)(len + 1),
    };

    spi_message_init(&r->msg);
    spi_message_add_transfer(&r->msg, &r->xfer);
    r->msg.complete = regmap_spi_async_done;
    r->msg.context  = r;

    return spi_async(r->fd, &r->msg);
}

/*
 * All SPI errors map to -EIO. Unlike I2C, SPI has no per-flag distinctions
 * worth surfacing to the caller:
 *
 *   MODF  — mode fault (SS asserted by another master); implies a wiring or
 *            board configuration problem, not a transient condition.
 *   CRC   — CRC mismatch; data was corrupted in flight.
 *   OVR   — overrun; RX data was lost because software/DMA was too slow.
 *   FRE   — frame format error; clock/data framing mismatch.
 *   DMA   — DMA transfer fault.
 *
 * None of these are retryable the way I2C arbitration loss is, and SPI has
 * no NACK or bus-hang concept. All indicate a hardware or configuration fault
 */
static void regmap_spi_async_done(void *ctx, spi_evt_t event, uint32_t error_flags)
{
    (void)error_flags;
    regmap_spi_t *r = (regmap_spi_t *)ctx;
    if (event == SPI_EVT_TXRX_DONE) memcpy(r->data_buf, &r->rx_buf[1], r->data_len);
    if (r->cb) r->cb(r->cb_ctx, (event == SPI_EVT_TXRX_DONE) ? 0 : -EIO);
}

