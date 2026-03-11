#pragma once

#include <stdint.h>
#include "i2c_types.h"

/*
 * i2c_transfer — one segment of an I2C message.
 *
 * dir     : I2C_DIR_WRITE or I2C_DIR_READ
 * buf     : data buffer (write: source data; read: destination)
 * len     : number of bytes
 * use_dma : request DMA for this transfer (falls back to IT on failure)
 * next    : linked list — set by i2c_message_add_transfer()
 */
struct i2c_transfer {
    i2c_dir_t            dir;
    uint8_t             *buf;
    uint16_t             len;
    uint8_t              use_dma;
    struct i2c_transfer *next;
};

/*
 * i2c_message — an atomic I2C transaction.
 *
 * Frame boundaries are determined by position in the linked list of
 * transfers, mapped to i2c_xfer_opt_t (FIRST_AND_LAST / FIRST / NEXT / LAST).
 * For i2c_async(), complete() is called from ISR when all transfers finish.
 */
struct i2c_message {
    struct i2c_transfer *transfers;
    struct i2c_transfer *tail;
    i2c_cb_t             complete;
    void                *context;
};

/*
 * i2c_bus_ops — hardware controller operations.
 *
 * transfer_one    : blocking; returns when transfer completes.
 * transfer_one_it : non-blocking; cb(cb_ctx, event, err) called from ISR.
 * apply_config    : update bus speed / addressing mode; called lazily.
 * recover         : software abort + reinit after a transfer error.
 * bus_recover     : GPIO bit-bang 9-clock recovery for bus hang.
 */
struct i2c_bus_ops {
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*transfer_one)(void *ctx, uint16_t addr, i2c_dir_t dir,
                        uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt);
    int (*transfer_one_it)(void *ctx, uint16_t addr, i2c_dir_t dir,
                           uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt,
                           uint8_t use_dma, i2c_cb_t cb, void *cb_ctx);
    int (*apply_config)(void *ctx, i2c_speed_t speed, i2c_addr_mode_t addr_mode);
    int (*recover)(void *ctx, uint32_t error_flags);
    int (*bus_recover)(void *ctx);
};

/* A physical I2C controller */
struct i2c_bus {
    const char               *name;   /* "i2c1", "i2c2", … */
    const struct i2c_bus_ops *ops;
    void                     *ctx;
};

/*
 * An I2C slave device.
 *   name      - unique, e.g. "i2c1.mpu", "i2c1.bmp"
 *   bus_name  - must match a registered i2c_bus
 *   addr      - device address (7-bit value, NOT shifted)
 *   addr_mode - I2C_ADDR_7BIT or I2C_ADDR_10BIT
 *   speed     - bus speed to apply when this slave is active
 */
struct i2c_slave {
    const char      *name;
    const char      *bus_name;
    uint16_t         addr;
    i2c_addr_mode_t  addr_mode;
    i2c_speed_t      speed;
};

/* Registration — called by board layer */
int i2c_bus_register(struct i2c_bus *bus);
int i2c_slave_register(struct i2c_slave *slave);

/* Message API */
void i2c_message_init(struct i2c_message *msg);
void i2c_message_add_transfer(struct i2c_message *msg, struct i2c_transfer *xfer);

/*
 * i2c_sync  — execute a message atomically, blocking until complete.
 *             Must not be called from ISR context.
 * i2c_async — start a message non-blocking; msg->complete() triggered from ISR.
 *             Returns -1 if bus queue is full or args invalid.
 */
int i2c_sync(int fd, struct i2c_message *msg);
int i2c_async(int fd, struct i2c_message *msg);

/* Application API */

/*
 * i2c_open — open a registered slave by name; returns fd or -1.
 *
 * PRECONDITION: the bus that the slave belongs to must already be registered
 * via i2c_bus_register() before calling i2c_open().  In practice this means
 * board_i2c_init() must be called before board_i2c_add_slave() / i2c_open().
 * Violating this order returns -1 with no further diagnostic.
 */
int i2c_open(const char *slave_name);   /* returns fd */
int i2c_close(int fd);

/* Must be called from the main loop when using async mode */
void i2c_poll(void);

/*
 * i2c_reset — abort any in-flight transfer, drain the queue, and recover the bus.
 *
 * Intended for recovery when sensor_mgr detects repeated errors or a bus hang.
 * All discarded messages have their completion callbacks invoked with
 * I2C_EVT_ERROR / I2C_ERR_ABORT so callers unblock cleanly.
 *
 * Must NOT be called from ISR context.
 * Returns 0 on success, -1 if fd is invalid.
 */
int i2c_reset(int fd);

/*
 * I2C_MEM_WRITE_MAX_DATA — maximum data payload for i2c_mem_write().
 *
 * i2c_mem_write combines the register address and data bytes into a single
 * stack buffer to work around the STM32 HAL TX→TX sequential transfer bug
 * (see i2c.c).  The buffer is 2 (max addr bytes) + this constant bytes.
 * Passing len > I2C_MEM_WRITE_MAX_DATA returns -1 immediately.
 */
#define I2C_MEM_WRITE_MAX_DATA  32

/* Convenience wrappers — blocking, build single/two-transfer messages internally */
int i2c_write(int fd, const uint8_t *buf, uint16_t len);
int i2c_read(int fd, uint8_t *buf, uint16_t len);
int i2c_mem_write(int fd, uint16_t mem_addr, uint8_t mem_addr_size,
                  const uint8_t *buf, uint16_t len);
int i2c_mem_read(int fd, uint16_t mem_addr, uint8_t mem_addr_size,
                 uint8_t *buf, uint16_t len);
