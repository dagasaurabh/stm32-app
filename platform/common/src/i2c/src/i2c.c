/*
 * i2c.c — Facade layer of I2C subsystem.
 *
 * Responsibilities:
 *   - Maintains bus and slave registries
 *   - Provides fd-based application API
 *   - Implements message sequencing (sync + async)
 *   - Owns per-bus async state machine and queue
 *   - Lazy config apply (speed, addr_mode) per slave
 *
 * Does NOT:
 *   - Touch hardware registers directly
 *   - Perform blocking HAL operations (except inside convenience wrappers)
 */

#include "i2c.h"
#include <string.h>
#include "cmsis_compiler.h"   /* __get_PRIMASK, __set_PRIMASK, __disable_irq */
#include "cmsis_gcc.h"
#include "timer.h"
#include <assert.h>

#define I2C_MAX_BUSES   4
#define I2C_MAX_SLAVES  8

static struct i2c_bus   *bus_table[I2C_MAX_BUSES];
static struct i2c_slave *slave_table[I2C_MAX_SLAVES];
static struct i2c_slave *fd_table[I2C_MAX_SLAVES];

static void i2c_async_irq_cb(void *ctx, i2c_evt_t event, uint32_t error_flags);
static void s_complete_failed_message(struct i2c_message *msg, uint32_t error_flags);

struct i2c_sync_ctx {
    volatile int        done;
    volatile i2c_evt_t  event;
    volatile uint32_t   error_flags;
};

typedef struct {
    int                fd;
    struct i2c_message *msg;
} i2c_queued_t;

#define I2C_ASYNC_QUEUE_DEPTH 4

typedef enum {
    I2C_BUS_IDLE = 0,
    I2C_BUS_ACTIVE
} i2c_bus_status_t;

typedef struct {
    i2c_bus_status_t status;

    /* Active transfer */
    struct i2c_slave    *slave;
    struct i2c_bus      *bus;
    struct i2c_message  *msg;          /* NULL = idle */
    struct i2c_transfer *current;
    uint32_t             error_flags;

    /* Pending message queue (ring buffer) */
    i2c_queued_t  queue[I2C_ASYNC_QUEUE_DEPTH];
    uint8_t       q_head;
    uint8_t       q_tail;
    uint8_t       q_count;

    /* Active configuration cache */
    i2c_speed_t      current_speed;
    i2c_addr_mode_t  current_addr_mode;

    volatile bool     needs_recovery;
    volatile bool     needs_bus_recovery;
    volatile uint32_t recovery_flags;
} i2c_bus_state_t;

static i2c_bus_state_t bus_state[I2C_MAX_BUSES];

/* ------------------------------------------------------------------ */
/* Critical section helpers                                             */
/* ------------------------------------------------------------------ */

static inline uint32_t i2c_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void i2c_exit_critical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static int s_get_bus_idx_by_name(const char *name)
{
    for (int i = 0; i < I2C_MAX_BUSES; i++) {
        if (bus_table[i] && strcmp(bus_table[i]->name, name) == 0) return i;
    }
    return -1;
}

/*
 * get_xfer_options — compute i2c_xfer_opt_t from position in linked list.
 *
 * Called for each transfer just before it is submitted to the driver.
 * xfer must be non-NULL; msg->transfers must be non-NULL.
 */
static i2c_xfer_opt_t get_xfer_options(const struct i2c_message *msg,
                                        const struct i2c_transfer *xfer)
{
    bool is_first = (xfer == msg->transfers);
    bool is_last  = (xfer->next == NULL);

    if (is_first && is_last)  return I2C_XFER_FIRST_AND_LAST;
    if (is_first)             return I2C_XFER_FIRST;
    if (is_last)              return I2C_XFER_LAST;
    return I2C_XFER_NEXT;
}

static void s_recover(i2c_bus_state_t *st)
{
    if (st->needs_bus_recovery) {
        if (st->bus->ops->bus_recover)
            st->bus->ops->bus_recover(st->bus->ctx);
        st->needs_bus_recovery = false;
    }

    if (st->needs_recovery) {
        if (st->bus->ops->recover)
            st->bus->ops->recover(st->bus->ctx, st->recovery_flags);
        st->needs_recovery = false;
        st->recovery_flags = 0;
    }
}

/* ------------------------------------------------------------------ */
/* Registration                                                         */
/* ------------------------------------------------------------------ */

int i2c_bus_register(struct i2c_bus *bus)
{
    if (!bus || !bus->ops || !bus->name) return -1;

    for (int i = 0; i < I2C_MAX_BUSES; i++) {
        if (!bus_table[i]) {
            bus_table[i] = bus;
            if (bus->ops->open) bus->ops->open(bus->ctx);

            i2c_bus_state_t *st = &bus_state[i];
            memset(st, 0, sizeof(*st));
            st->status             = I2C_BUS_IDLE;
            st->current_speed      = (i2c_speed_t)-1;
            st->current_addr_mode  = (i2c_addr_mode_t)-1;
            return 0;
        }
    }
    return -1;
}

int i2c_slave_register(struct i2c_slave *slave)
{
    if (!slave || !slave->name || !slave->bus_name) return -1;

    for (int i = 0; i < I2C_MAX_SLAVES; i++) {
        if (!slave_table[i]) {
            slave_table[i] = slave;
            return 0;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Open / Close                                                         */
/* ------------------------------------------------------------------ */

static int s_find_open_slave(const char *slave_name)
{
    for (int fd = 0; fd < I2C_MAX_SLAVES; fd++) {
        if (fd_table[fd] && strcmp(fd_table[fd]->name, slave_name) == 0)
            return fd;
    }
    return -1;
}

int i2c_open(const char *slave_name)
{
    if (!slave_name) return -1;

    for (int s = 0; s < I2C_MAX_SLAVES; s++) {
        if (!slave_table[s] || strcmp(slave_table[s]->name, slave_name) != 0)
            continue;

        int rc = s_find_open_slave(slave_name);
        if (rc >= 0) return rc;

        for (int fd = 0; fd < I2C_MAX_SLAVES; fd++) {
            if (!fd_table[fd]) {
                fd_table[fd] = slave_table[s];
                return fd;
            }
        }
        return -1;
    }
    return -1;
}

int i2c_close(int fd)
{
    if (fd < 0 || fd >= I2C_MAX_SLAVES || !fd_table[fd]) return -1;

    uint32_t irq_state = i2c_enter_critical();
    struct i2c_slave *slave = fd_table[fd];

    for (int i = 0; i < I2C_MAX_BUSES; i++) {
        if (bus_state[i].msg && bus_state[i].slave == slave) {
            i2c_exit_critical(irq_state);
            return -1;   /* active */
        }
        for (int q = 0; q < bus_state[i].q_count; q++) {
            int idx = (bus_state[i].q_head + q) % I2C_ASYNC_QUEUE_DEPTH;
            if (fd_table[bus_state[i].queue[idx].fd] == slave) {
                i2c_exit_critical(irq_state);
                return -1;   /* queued */
            }
        }
    }

    fd_table[fd] = NULL;
    i2c_exit_critical(irq_state);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Message API                                                          */
/* ------------------------------------------------------------------ */

void i2c_message_init(struct i2c_message *msg)
{
    msg->transfers = NULL;
    msg->tail      = NULL;
    msg->complete  = NULL;
    msg->context   = NULL;
}

void i2c_message_add_transfer(struct i2c_message *msg, struct i2c_transfer *xfer)
{
    xfer->next = NULL;
    if (!msg->tail) {
        msg->transfers = xfer;
        msg->tail      = xfer;
    } else {
        msg->tail->next = xfer;
        msg->tail       = xfer;
    }
}

/* ------------------------------------------------------------------ */
/* Sync                                                                 */
/* ------------------------------------------------------------------ */

static void i2c_sync_cb(void *ctx, i2c_evt_t event, uint32_t error_flags)
{
    struct i2c_sync_ctx *s = ctx;
    s->event       = event;
    s->error_flags = error_flags;
    s->done        = 1;
}

int i2c_sync(int fd, struct i2c_message *msg)
{
    if (__get_IPSR() != 0) return -1;
    if (fd < 0 || fd >= I2C_MAX_SLAVES || !fd_table[fd]) return -1;
    if (!msg || !msg->transfers) return -1;

    struct i2c_sync_ctx sync_ctx = {0};

    i2c_cb_t user_cb  = msg->complete;
    void    *user_ctx = msg->context;

    msg->complete = i2c_sync_cb;
    msg->context  = &sync_ctx;

    if (i2c_async(fd, msg) < 0) {
        msg->complete = user_cb;
        msg->context  = user_ctx;
        return -1;
    }

    mstimer_t timer;
    mstimer_start(&timer, 5000UL);

    struct i2c_slave *slave = fd_table[fd];
    int bus_idx = s_get_bus_idx_by_name(slave->bus_name);
    assert(bus_idx >= 0);
    i2c_bus_state_t *st = &bus_state[bus_idx];
    bool timed_out = false;

    while (!sync_ctx.done) {
        i2c_poll();
        if (!timed_out && mstimer_expired(&timer)) {
            st->needs_recovery = true;
            s_recover(st);

            st->msg     = NULL;
            st->current = NULL;
            st->status  = I2C_BUS_IDLE;

            sync_ctx.event = I2C_EVT_ERROR;
            sync_ctx.done  = 1;
            timed_out = true;
        }
    }

    msg->complete = user_cb;
    msg->context  = user_ctx;

    if (timed_out || sync_ctx.event != I2C_EVT_DONE) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Async (interrupt-driven) with per-bus message queue                 */
/* ------------------------------------------------------------------ */

static int i2c_start_transfer(i2c_bus_state_t *st)
{
    i2c_xfer_opt_t opt = get_xfer_options(st->msg, st->current);

    return st->bus->ops->transfer_one_it(
        st->bus->ctx,
        st->slave->addr << 1,     /* HAL uses 8-bit shifted address */
        st->current->dir,
        st->current->buf,
        st->current->len,
        opt,
        st->current->use_dma,
        i2c_async_irq_cb,
        st);
}

/*
 * i2c_async_irq_cb — called from ISR after each transfer_one_it segment.
 *
 * Within a message: chains next i2c_transfer.
 * End of message : fires user callback, then starts next queued message.
 */
static void i2c_async_irq_cb(void *ctx, i2c_evt_t event, uint32_t error_flags)
{
    i2c_bus_state_t *st = (i2c_bus_state_t *)ctx;

    if (!st || !st->msg || !st->current) {
        return;
    }

    if (event == I2C_EVT_ERROR) {
        st->error_flags |= error_flags;

        if (error_flags & (I2C_ERR_BERR | I2C_ERR_ARLO | I2C_ERR_DMA)) {
            st->needs_bus_recovery = true;
        } else {
            st->needs_recovery = true;
        }
        st->recovery_flags = error_flags;
    }

    st->current = st->current->next;

    if (st->current && st->error_flags == 0) {
        /* More transfers remain in this message — continue */
        if (i2c_start_transfer(st) == 0) {
            return;
        }

        st->error_flags |= I2C_ERR_TIMEOUT;
        st->needs_recovery = true;
        st->recovery_flags |= I2C_ERR_TIMEOUT;
    }

    /* ----- Message complete ----- */
    i2c_cb_t  complete  = st->msg->complete;
    void     *context   = st->msg->context;
    i2c_evt_t final_evt = (st->error_flags == 0) ? I2C_EVT_DONE : I2C_EVT_ERROR;
    uint32_t  final_err = st->error_flags;

    if (st->q_count > 0) {
        i2c_queued_t next        = st->queue[st->q_head];
        struct i2c_slave *next_slave = fd_table[next.fd];

        bool config_change =
            (st->current_speed     != next_slave->speed) ||
            (st->current_addr_mode != next_slave->addr_mode);

        if (st->needs_recovery || st->needs_bus_recovery || config_change) {
            /* Defer to i2c_poll() for heavy work */
            st->msg     = NULL;
            st->current = NULL;
            st->status  = I2C_BUS_IDLE;
        } else {
            st->q_head  = (st->q_head + 1) % I2C_ASYNC_QUEUE_DEPTH;
            st->q_count--;

            st->slave       = next_slave;
            st->msg         = next.msg;
            st->current     = next.msg->transfers;
            st->error_flags = 0;

            if (i2c_start_transfer(st) < 0) {
                s_complete_failed_message(next.msg, I2C_ERR_TIMEOUT);
                st->needs_recovery = true;
                st->recovery_flags |= I2C_ERR_TIMEOUT;
                st->msg     = NULL;
                st->current = NULL;
                st->status  = I2C_BUS_IDLE;
            }
        }
    } else {
        st->msg    = NULL;
        st->status = I2C_BUS_IDLE;
    }

    if (complete) complete(context, final_evt, final_err);
}

static void s_apply_config(i2c_bus_state_t *st)
{
    if (st->bus->ops->apply_config) {
        st->bus->ops->apply_config(st->bus->ctx,
                                   st->slave->speed,
                                   st->slave->addr_mode);
        st->current_speed     = st->slave->speed;
        st->current_addr_mode = st->slave->addr_mode;
    }
}

static void s_complete_failed_message(struct i2c_message *msg, uint32_t error_flags)
{
    if (!msg || !msg->complete) return;
    msg->complete(msg->context, I2C_EVT_ERROR, error_flags);
}

int i2c_async(int fd, struct i2c_message *msg)
{
    if (__get_IPSR() != 0) return -1;

    if (fd < 0 || fd >= I2C_MAX_SLAVES || !fd_table[fd]) return -1;
    if (!msg || !msg->transfers || !msg->complete) return -1;

    struct i2c_slave *slave = fd_table[fd];

    int bus_idx = s_get_bus_idx_by_name(slave->bus_name);
    if (bus_idx < 0) return -1;

    struct i2c_bus *bus = bus_table[bus_idx];
    if (!bus || !bus->ops->transfer_one_it) return -1;

    i2c_bus_state_t *st = &bus_state[bus_idx];

    uint32_t irq_state = i2c_enter_critical();

    if (st->status == I2C_BUS_IDLE && st->q_count == 0) {
        st->status      = I2C_BUS_ACTIVE;
        st->slave       = slave;
        st->bus         = bus;
        st->msg         = msg;
        st->current     = msg->transfers;
        st->error_flags = 0;
        i2c_exit_critical(irq_state);

        s_recover(st);
        s_apply_config(st);
        if (i2c_start_transfer(st) < 0) {
            uint32_t irq_state2 = i2c_enter_critical();
            st->msg            = NULL;
            st->current        = NULL;
            st->status         = I2C_BUS_IDLE;
            st->needs_recovery = true;
            st->recovery_flags |= I2C_ERR_TIMEOUT;
            i2c_exit_critical(irq_state2);
            return -1;
        }
    } else {
        if (st->q_count >= I2C_ASYNC_QUEUE_DEPTH) {
            i2c_exit_critical(irq_state);
            return -1;
        }
        st->queue[st->q_tail].fd  = fd;
        st->queue[st->q_tail].msg = msg;
        st->q_tail  = (uint8_t)((st->q_tail + 1) % I2C_ASYNC_QUEUE_DEPTH);
        st->q_count++;
        i2c_exit_critical(irq_state);
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Poll — thread-context work deferred from ISR                        */
/* ------------------------------------------------------------------ */

void i2c_poll(void)
{
    for (int i = 0; i < I2C_MAX_BUSES; i++) {
        i2c_bus_state_t *st = &bus_state[i];

        if (st->status != I2C_BUS_IDLE) continue;
        if (!st->needs_recovery && !st->needs_bus_recovery && st->q_count == 0)
            continue;

        uint32_t irq_state = i2c_enter_critical();

        if (st->status != I2C_BUS_IDLE) {
            i2c_exit_critical(irq_state);
            continue;
        }

        if (st->needs_recovery || st->needs_bus_recovery) {
            i2c_exit_critical(irq_state);
            s_recover(st);
            continue;
        }

        if (st->q_count > 0) {
            i2c_queued_t next = st->queue[st->q_head];
            st->q_head  = (st->q_head + 1) % I2C_ASYNC_QUEUE_DEPTH;
            st->q_count--;

            st->slave       = fd_table[next.fd];
            st->msg         = next.msg;
            st->current     = next.msg->transfers;
            st->error_flags = 0;
            st->status      = I2C_BUS_ACTIVE;

            i2c_exit_critical(irq_state);

            s_apply_config(st);
            if (i2c_start_transfer(st) < 0) {
                s_complete_failed_message(next.msg, I2C_ERR_TIMEOUT);
                st->msg            = NULL;
                st->current        = NULL;
                st->status         = I2C_BUS_IDLE;
                st->needs_recovery = true;
                st->recovery_flags |= I2C_ERR_TIMEOUT;
            }
        } else {
            i2c_exit_critical(irq_state);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Convenience wrappers                                                 */
/* ------------------------------------------------------------------ */

int i2c_write(int fd, const uint8_t *buf, uint16_t len)
{
    struct i2c_transfer xfer = {
        .dir = I2C_DIR_WRITE, .buf = (uint8_t *)buf, .len = len
    };
    struct i2c_message msg;
    i2c_message_init(&msg);
    i2c_message_add_transfer(&msg, &xfer);
    return i2c_sync(fd, &msg);
}

int i2c_read(int fd, uint8_t *buf, uint16_t len)
{
    struct i2c_transfer xfer = {
        .dir = I2C_DIR_READ, .buf = buf, .len = len
    };
    struct i2c_message msg;
    i2c_message_init(&msg);
    i2c_message_add_transfer(&msg, &xfer);
    return i2c_sync(fd, &msg);
}

/*
 * i2c_mem_write — write mem_addr_size address bytes then data bytes.
 *
 * Uses two WRITE transfers in one message so the bus issues only one
 * START (repeated-start between the two is suppressed by I2C_XFER_NEXT).
 */
int i2c_mem_write(int fd, uint16_t mem_addr, uint8_t mem_addr_size,
                  const uint8_t *buf, uint16_t len)
{
    uint8_t addr_buf[2] = {0,};
    uint16_t addr_len;

    if (mem_addr_size == 2) {
        addr_buf[0] = (uint8_t)(mem_addr >> 8);
        addr_buf[1] = (uint8_t)(mem_addr & 0xFF);
        addr_len = 2;
    } else {
        addr_buf[0] = (uint8_t)(mem_addr & 0xFF);
        addr_len = 1;
    }

    struct i2c_transfer xfer_addr = {
        .dir = I2C_DIR_WRITE, .buf = addr_buf, .len = addr_len
    };
    struct i2c_transfer xfer_data = {
        .dir = I2C_DIR_WRITE, .buf = (uint8_t *)buf, .len = len
    };

    struct i2c_message msg;
    i2c_message_init(&msg);
    i2c_message_add_transfer(&msg, &xfer_addr);
    i2c_message_add_transfer(&msg, &xfer_data);
    return i2c_sync(fd, &msg);
}

/*
 * i2c_mem_read — write mem address then read data (repeated START).
 *
 * Two transfers: WRITE (addr bytes) + READ (data bytes), mapped to
 * I2C_XFER_FIRST and I2C_XFER_LAST by get_xfer_options().
 */
int i2c_mem_read(int fd, uint16_t mem_addr, uint8_t mem_addr_size,
                 uint8_t *buf, uint16_t len)
{
    uint8_t addr_buf[2];
    uint16_t addr_len;

    if (mem_addr_size == 2) {
        addr_buf[0] = (uint8_t)(mem_addr >> 8);
        addr_buf[1] = (uint8_t)(mem_addr & 0xFF);
        addr_len = 2;
    } else {
        addr_buf[0] = (uint8_t)(mem_addr & 0xFF);
        addr_len = 1;
    }

    struct i2c_transfer xfer_addr = {
        .dir = I2C_DIR_WRITE, .buf = addr_buf, .len = addr_len
    };
    struct i2c_transfer xfer_data = {
        .dir = I2C_DIR_READ, .buf = buf, .len = len
    };

    struct i2c_message msg;
    i2c_message_init(&msg);
    i2c_message_add_transfer(&msg, &xfer_addr);
    i2c_message_add_transfer(&msg, &xfer_data);
    return i2c_sync(fd, &msg);
}
