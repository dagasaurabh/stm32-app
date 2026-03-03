#pragma once

#include <stdint.h>
#include "gpio.h"
#include "spi_types.h"   /* spi_evt_t, SPI_ERR_*, spi_cb_t */

#define SPI_MAX_BUSES   4
#define SPI_MAX_SLAVES  8

/*
 * spi_transfer — one segment of an SPI message.
 *
 * tx_buf    : buffer to transmit (NULL -> send dummy bytes)
 * rx_buf    : buffer to receive  (NULL -> discard incoming bytes)
 * len       : number of bytes in this segment
 * cs_change : if non-zero, de assert CS after this segment and
 *             re assert it before the next one (some devices need a
 *             brief CS pulse between command and data phases)
 * next      : linked list - do not set manually, use spi_message_add_transfer()
 */
struct spi_transfer {
    const uint8_t      *tx_buf;
    uint8_t            *rx_buf;
    uint16_t            len;
    uint8_t             cs_change;
    struct spi_transfer *next;      /* set by spi_message_add_transfer() */
};

/*
 * spi_message - an atomic SPI transaction.
 *
 * CS is asserted before the first transfer and de asserted after the last
 * (unless individual transfers have cs_change set).
 *
 * For spi_async(), set complete and context before calling spi_async().
 * complete() is called from ISR when all transfers finish.
 */
struct spi_message {
    struct spi_transfer *transfers; /* head of transfer list */
    struct spi_transfer *tail;      /* tail - for O(1) append */
    spi_cb_t             complete;  /* async completion callback (NULL for sync) */
    void                *context;  /* passed to complete() */
};

/*
 * spi_bus_ops - hardware controller operations.
 *
 * Only transfer_one() is required for spi_sync().
 * transfer_one_it() is required for spi_async().
 *
 *   transfer_one    : blocking raw segment.
 *   transfer_one_it : non-blocking; cb(cb_ctx, status) called from ISR.
 */
struct spi_bus_ops {
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*transfer_one)(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len);
    int (*transfer_one_it)(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len,
                           spi_cb_t cb, void *cb_ctx);
    int (*apply_config)(void *ctx, spi_mode_t mode, spi_clkdiv_t prescaler, spi_datasize_t datasize);
    int (*recover)(void *ctx, uint32_t error_flags);
};

/* A SPI hardware controller */
struct spi_bus {
    const char              *name;  /* "spi1", "spi2", … */
    const struct spi_bus_ops *ops;
    void                    *ctx;
};

/*
 * A slave device.
 *   name     - unique, e.g. "spi1.0", "spi1.1"
 *   bus_name - must match a registered spi_bus
 *   cs_pin   - GPIO driven by spi_sync()/spi_async() around each message
 */
struct spi_slave {
    const char *name;
    const char *bus_name;
    gpio_pin_t  cs_pin;

    /* Compile-time configuration */
    spi_mode_t      mode;        /* SPI_MODE_0..3 */
    spi_clkdiv_t    prescaler;   /* HAL prescaler value */
    spi_datasize_t  datasize;    /* 8 or 16 */
};

/* SPI bus and slave registration called by board layer during init */
int spi_bus_register(struct spi_bus *bus);
int spi_slave_register(struct spi_slave *slave);

/* Message API */
void spi_message_init(struct spi_message *msg);
void spi_message_add_transfer(struct spi_message *msg, struct spi_transfer *xfer);

/*
 * spi_sync  — execute a message atomically, blocking until complete.
 * spi_async — start a message non-blocking; msg->complete() triggered from ISR.
 *             Returns -1 if the bus is busy or transfer_one_it is not set.
 */
int spi_sync(int fd, struct spi_message *msg);
int spi_async(int fd, struct spi_message *msg);

/* Application API */
int spi_open(const char *slave_name);  /* returns fd */
int spi_close(int fd);
int spi_write(int fd, const uint8_t *buf, uint16_t len);
int spi_read(int fd, uint8_t *buf, uint16_t len);
int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, uint16_t len);

/* spi_poll -  needs to be called explicitly when using spi in async mode */
void spi_poll(void);
