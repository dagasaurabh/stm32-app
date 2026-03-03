/*
 * spi.c — Facade layer of SPI subsystem.
 *
 * Responsibilities:
 *   - Maintains bus and slave registries
 *   - Provides fd-based application API
 *   - Implements message sequencing (sync + async)
 *   - Manages CS assertion/deassertion
 *   - Owns per-bus async state machine and queue
 *
 * Does NOT:
 *   - Touch hardware registers directly
 *   - Perform blocking HAL operations
 */

/*
 * NOTE:
 * Chip-select (CS) handling is fully owned by this facade layer.
 * Bus ops (transfer_one_it) must NOT manipulate CS.
 */

#include "spi.h"
#include "gpio.h"
#include <string.h>
#include "cmsis_compiler.h"  /* __get_PRIMASK, __set_PRIMASK, __disable_irq */
#include "cmsis_gcc.h"
#include "timer.h"
#include <assert.h>

#define SPI_MAX_BUSES   4
#define SPI_MAX_SLAVES  8

static struct spi_bus   *bus_table[SPI_MAX_BUSES];
static struct spi_slave *slave_table[SPI_MAX_SLAVES];

/*
 * fd_table maps small integer file descriptors to registered spi_slave objects.
 * Entries are set by spi_open() and cleared by spi_close().
 * spi_close() fails if the slave has an active or queued async transfer.
 */

static struct spi_slave *fd_table[SPI_MAX_SLAVES];

static void spi_async_irq_cb(void *ctx, spi_evt_t event, uint32_t error_flags);

struct spi_sync_ctx {
    volatile int      done;
    volatile spi_evt_t event;
    volatile uint32_t error_flags;
};

/* Per-bus pending queue entry */
typedef struct {
    int                fd;
    struct spi_message *msg;
} spi_queued_t;

#define SPI_ASYNC_QUEUE_DEPTH 4

/*
 * Per-bus async runtime state.
 *
 * Each physical SPI controller has exactly one instance of this struct.
 * It tracks:
 *   - Currently active message and transfer segment
 *   - Accumulated error flags across segments
 *   - Slave whose CS is currently asserted
 *   - Ring buffer of queued messages waiting for this bus
 *
 * st->msg == NULL means the bus is idle.
 */

typedef enum {
    SPI_BUS_IDLE = 0,
    SPI_BUS_ACTIVE
} spi_bus_status_t;

typedef struct {
    spi_bus_status_t status;

    /* Active transfer */
    struct spi_slave    *slave;
    struct spi_bus      *bus;
    struct spi_message  *msg;          /* NULL = idle */
    struct spi_transfer *current;
    uint32_t             error_flags;  /* accumulated SPI_ERR_* bits, 0 = ok */

    /* Pending message queue (ring buffer) */
    spi_queued_t         queue[SPI_ASYNC_QUEUE_DEPTH];
    uint8_t              q_head;
    uint8_t              q_tail;
    uint8_t              q_count;

    /* Active configuration cache */
    spi_mode_t      current_mode;
    spi_clkdiv_t    current_prescaler;
    spi_datasize_t  current_datasize;

    volatile bool needs_recovery;
    volatile uint32_t recovery_flags;
} spi_bus_state_t;

static spi_bus_state_t bus_state[SPI_MAX_BUSES];

/*
 * Save/restore critical section.
 *
 */
static inline uint32_t spi_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void spi_exit_critical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

static int s_get_bus_idx_by_name(const char * name) {
    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (bus_table[i] && strcmp(bus_table[i]->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void s_recover(spi_bus_state_t *st)
{
    if (st->needs_recovery) {
        st->bus->ops->recover(st->bus->ctx,
                st->recovery_flags);
        st->needs_recovery = false;
        st->recovery_flags = 0;
    }
}

/* SPI Registration */

int spi_bus_register(struct spi_bus *bus)
{
    if (!bus || !bus->ops || !bus->name) return -1;

    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (!bus_table[i]) {
            bus_table[i] = bus;
            if (bus->ops->open) bus->ops->open(bus->ctx);

            spi_bus_state_t *st = &bus_state[i];
            memset(st, 0, sizeof(*st));
            st->status = SPI_BUS_IDLE;
            st->current_mode = (spi_mode_t)-1;
            return 0;
        }
    }
    return -1;
}

int spi_slave_register(struct spi_slave *slave)
{
    if (!slave || !slave->name || !slave->bus_name) return -1;

    for (int i = 0; i < SPI_MAX_SLAVES; i++) {
        if (!slave_table[i]) {
            slave_table[i] = slave;
            return 0;
        }
    }
    return -1;
}

static int s_find_slave(const char *slave_name) {
    for(int fd = 0; fd < SPI_MAX_SLAVES; fd++) {
        if(!fd_table[fd]) continue;
        else if(strcmp(fd_table[fd]->name, slave_name) == 0) {
            return fd;
        };
    }
    return -1;
}

int spi_open(const char *slave_name)
{
    if (!slave_name) return -1;

    for (int s = 0; s < SPI_MAX_SLAVES; s++) {
        if (!slave_table[s] || strcmp(slave_table[s]->name, slave_name) != 0) continue;

        int rc = s_find_slave(slave_name);
        if(rc != -1) return rc;

        for (int fd = 0; fd < SPI_MAX_SLAVES; fd++) {
            if (!fd_table[fd]) {
                fd_table[fd] = slave_table[s];
                return fd;
            }
        }
        return -1; /* no free fd slot */
    }
    return -1; /* slave not registered */
}

/*
 * spi_close fails if:
 *   - The slave currently has an active async transfer
 *   - The slave is present in any bus queue
 *
 * This prevents lifetime races between ISR and application.
 */

int spi_close(int fd)
{
    if (fd < 0 || fd >= SPI_MAX_SLAVES || !fd_table[fd]) return -1;

    uint32_t irq_state = spi_enter_critical();
    struct spi_slave *slave = fd_table[fd];

    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (bus_state[i].msg && bus_state[i].slave == slave) {
            spi_exit_critical(irq_state);
            return -1;  /* active */
        }

        for (int q = 0; q < bus_state[i].q_count; q++) {
            int idx = (bus_state[i].q_head + q) % SPI_ASYNC_QUEUE_DEPTH;
            if (fd_table[bus_state[i].queue[idx].fd] == slave) {
                spi_exit_critical(irq_state);
                return -1;  /* queued */
            }
        }
    }

    fd_table[fd] = NULL;
    spi_exit_critical(irq_state);
    return 0;
}

/* Message API */

void spi_message_init(struct spi_message *msg)
{
    msg->transfers = NULL;
    msg->tail      = NULL;
    msg->complete  = NULL;
    msg->context   = NULL;
}

void spi_message_add_transfer(struct spi_message *msg, struct spi_transfer *xfer)
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

static void spi_sync_cb(void *ctx, spi_evt_t event, uint32_t error_flags)
{
    struct spi_sync_ctx *s = ctx;

    s->event       = event;
    s->error_flags = error_flags;
    s->done        = 1;
}

/*
 * spi_sync — synchronous wrapper around spi_async().
 *
 * Submits the message to the async engine and blocks (busy-wait)
 * until the message completion callback fires.
 *
 * Must not be called from ISR context.
 */

int spi_sync(int fd, struct spi_message *msg)
{
    if (__get_IPSR() != 0) return -1;

    struct spi_sync_ctx sync_ctx = {0};

    spi_cb_t user_cb = msg->complete;
    void *user_ctx   = msg->context;

    msg->complete = spi_sync_cb;
    msg->context  = &sync_ctx;

    if (spi_async(fd, msg) < 0) {
        msg->complete = user_cb;
        msg->context  = user_ctx;
        return -1;
    }

    mstimer_t timer;
    mstimer_start(&timer, 5000UL);

    struct spi_slave *slave = fd_table[fd];
    int bus_idx = s_get_bus_idx_by_name(slave->bus_name);
    assert(bus_idx >= 0);
    spi_bus_state_t *st = &bus_state[bus_idx];
    bool time_out = false;

    while (!sync_ctx.done) {
        spi_poll();
        if(!time_out && mstimer_expired(&timer)) {
            st->needs_recovery = true;
            s_recover(st);

            /* ISR won't call spi_async_irq_cb — manually clean up bus state */
            st->msg     = NULL;
            st->current = NULL;
            st->status  = SPI_BUS_IDLE;

            /* Unblock the wait loop */
            sync_ctx.event = SPI_EVT_ERROR;
            sync_ctx.done  = 1;
            time_out = true;
            gpio_write(st->slave->cs_pin, GPIO_HIGH);

        }
    }

    msg->complete = user_cb;
    msg->context  = user_ctx;

    if (time_out || sync_ctx.event != SPI_EVT_TXRX_DONE) return -1;

    return 0;
}

/* ------------------------------------------------------------------ */
/* Async (interrupt-driven) with per-bus message queue                 */
/* ------------------------------------------------------------------ */

/*
 * spi_start_transfer - assert CS and kick off the first transfer of a message.
 * Called from both spi_async() (task context) and spi_async_irq_cb() (ISR context).
 * Caller must have already filled:
 *   - st->slave
 *   - st->bus
 *   - st->msg
 *   - st->current
 *   - st->error_flags
 */
static void spi_start_transfer(spi_bus_state_t *st)
{
    struct spi_slave *slave = st->slave;
    gpio_write(slave->cs_pin, GPIO_LOW);
    st->bus->ops->transfer_one_it(st->bus->ctx,
                                   st->current->tx_buf, st->current->rx_buf,
                                   st->current->len,
                                   spi_async_irq_cb, st);
}

/*
 * spi_async_irq_cb — called from ISR after each transfer_one_it segment.
 *
 * Within a message:  chains next spi_transfer.
 * End of message  :  deasserts CS, fires user callback, then starts
 *                    the next queued message (if any) before returning
 *                    so the bus stays busy without a gap.
 *
 * NOTE: gpio_write() and transfer_one_it() are ISR-safe (direct register
 * writes / HAL IT entry points).  The user's msg->complete() must also
 * be ISR-safe.
 */
static void spi_async_irq_cb(void *ctx, spi_evt_t event, uint32_t error_flags)
{
    spi_bus_state_t *st    = (spi_bus_state_t *)ctx;
    struct spi_slave *slave = st->slave;

    if (event == SPI_EVT_ERROR) {
        st->error_flags |= error_flags;

        /* Mark fatal errors */
        if (error_flags & (SPI_ERR_MODF |
                    SPI_ERR_OVR  |
                    SPI_ERR_DMA)) {

            st->needs_recovery = true;
            st->recovery_flags = error_flags;
        }
    }

    /* cs_change between two segments of the same message */
    if (event != SPI_EVT_ERROR && st->current->cs_change && st->current->next) {
        gpio_write(slave->cs_pin, GPIO_HIGH);
        gpio_write(slave->cs_pin, GPIO_LOW);
    }

    st->current = st->current->next;

    if (st->current && st->error_flags == 0) {
        /* More transfers remain in this message - continue */
        st->bus->ops->transfer_one_it(st->bus->ctx,
                st->current->tx_buf, st->current->rx_buf,
                st->current->len,
                spi_async_irq_cb, st);
        return;
    }

    /* ----- Message complete ----- */
    gpio_write(slave->cs_pin, GPIO_HIGH); /* de assert CS */

    /* Capture callback info before overwriting the active state */
    spi_cb_t  complete   = st->msg->complete;
    void     *context    = st->msg->context;
    spi_evt_t final_evt  = (st->error_flags == 0) ? SPI_EVT_TXRX_DONE : SPI_EVT_ERROR;
    uint32_t  final_err  = st->error_flags;

    /*
     * Dequeue the next message (if any) and start it immediately so
     * the bus stays busy without an idle gap between messages.
     * We do this before firing the user callback so that:
     *   (a) the callback can safely call spi_async() — it will enqueue
     *       rather than race with our dequeue here.
     *   (b) the next message starts as soon as possible.
     */
    if (st->q_count > 0) {

        spi_queued_t next = st->queue[st->q_head];
        struct spi_slave *next_slave = fd_table[next.fd];

        bool config_change =
            (st->current_mode != next_slave->mode) ||
            (st->current_prescaler != next_slave->prescaler) ||
            (st->current_datasize != next_slave->datasize);

        if (st->needs_recovery || config_change) {
            /* DO NOT touch queue */
            /* let spi_poll() handle everything */
            /* do not return from here. Need to invoke user cb */

            st->msg = NULL;
            st->current = NULL;
            st->status = SPI_BUS_IDLE;
        }
        else {
            /* No heavy work needed -> safe to continue in ISR */
            st->q_head = (st->q_head + 1) % SPI_ASYNC_QUEUE_DEPTH;
            st->q_count--;

            st->slave = next_slave;
            st->msg   = next.msg;
            st->current = next.msg->transfers;
            st->error_flags = 0;

            spi_start_transfer(st);
        }
    }
    else {
        st->msg = NULL;
        st->status = SPI_BUS_IDLE;
    }

    /* Trigger user callback for the just-completed message */
    if (complete) complete(context, final_evt, final_err);
}

static void s_apply_config(spi_bus_state_t *st)
{
    if (st->bus->ops->apply_config) {

        st->bus->ops->apply_config(st->bus->ctx,
                st->slave->mode, st->slave->prescaler, st->slave->datasize);

        st->current_mode = st->slave->mode;
        st->current_prescaler = st->slave->prescaler;
        st->current_datasize = st->slave->datasize;
    }
}

/*
 * spi_async — start an interrupt-driven message.
 *
 * If the bus is idle the message starts immediately.
 * If the bus is busy the message is queued and started automatically
 * when the current message finishes.  The caller's msg->complete()
 * is always called exactly once, from ISR context.
 *
 * Returns  0 : message started or queued successfully.
 * Returns -1 : invalid arguments, transfer_one_it not set, or queue full.
 *
 * msg->complete must be non-NULL (async without a callback makes no sense).
 * The caller must not modify the message or its buffers until the callback fires.
 * This function must be called from thread context (not ISR).
 * Not re-entrant. Designed for single-threaded bare-metal use.
 */
int spi_async(int fd, struct spi_message *msg)
{
    /* Can not be called from ISR context */
    if (__get_IPSR() != 0) return -1;

    if (fd < 0 || fd >= SPI_MAX_SLAVES || !fd_table[fd]) return -1;
    if (!msg || !msg->transfers || !msg->complete) return -1;

    struct spi_slave *slave = fd_table[fd];

    int bus_idx = s_get_bus_idx_by_name(slave->bus_name);
    if(bus_idx < 0) return -1;

    struct spi_bus *bus = bus_table[bus_idx]; 

    if (!bus || !bus->ops->transfer_one_it) return -1;

    spi_bus_state_t *st = &bus_state[bus_idx];

    /*
     * Critical section covers only the idle-check + queue/state mutation.
     * GPIO and HAL IT calls (which can be slow or block briefly) are
     * outside the critical section.
     */

    uint32_t irq_state = spi_enter_critical();
    if(st->status == SPI_BUS_IDLE && st->q_count == 0) {

        /* Bus idle - set up state and mark busy before exiting critical */
        st->status      = SPI_BUS_ACTIVE;
        st->slave       = slave;
        st->bus         = bus;
        st->msg         = msg;
        st->current     = msg->transfers;
        st->error_flags = 0;
        spi_exit_critical(irq_state);

        s_recover(st);
        s_apply_config(st);
        spi_start_transfer(st);
    }
    else {
        /* Bus busy - enqueue */
        if (st->q_count >= SPI_ASYNC_QUEUE_DEPTH) {
            spi_exit_critical(irq_state);
            return -1; /* queue full */
        }
        st->queue[st->q_tail].fd  = fd;
        st->queue[st->q_tail].msg = msg;
        st->q_tail  = (uint8_t)((st->q_tail + 1) % SPI_ASYNC_QUEUE_DEPTH);
        st->q_count++;
        spi_exit_critical(irq_state);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Convenience wrappers — each is a single-transfer message            */
/* ------------------------------------------------------------------ */

int spi_write(int fd, const uint8_t *buf, uint16_t len)
{
    struct spi_transfer xfer = { .tx_buf = buf, .rx_buf = NULL, .len = len };
    struct spi_message  msg;
    spi_message_init(&msg);
    spi_message_add_transfer(&msg, &xfer);
    return spi_sync(fd, &msg);
}

int spi_read(int fd, uint8_t *buf, uint16_t len)
{
    struct spi_transfer xfer = { .tx_buf = NULL, .rx_buf = buf, .len = len };
    struct spi_message  msg;
    spi_message_init(&msg);
    spi_message_add_transfer(&msg, &xfer);
    return spi_sync(fd, &msg);
}

int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    struct spi_transfer xfer = { .tx_buf = tx, .rx_buf = rx, .len = len };
    struct spi_message  msg;
    spi_message_init(&msg);
    spi_message_add_transfer(&msg, &xfer);
    return spi_sync(fd, &msg);
}

void spi_poll(void)
{
    for (int i = 0; i < SPI_MAX_BUSES; i++) {

        spi_bus_state_t *st = &bus_state[i];

        if (st->status != SPI_BUS_IDLE) continue;

        if (!st->needs_recovery && st->q_count == 0) continue;

        uint32_t irq_state = spi_enter_critical();

        if (st->status != SPI_BUS_IDLE) {
            spi_exit_critical(irq_state);
            continue;
        }

        if (st->needs_recovery) {
            s_recover(st);
            spi_exit_critical(irq_state);
            continue;
        }

        if (st->q_count > 0) {

            spi_queued_t next = st->queue[st->q_head];
            st->q_head = (st->q_head + 1) % SPI_ASYNC_QUEUE_DEPTH;
            st->q_count--;

            st->slave = fd_table[next.fd];
            st->msg   = next.msg;
            st->current = next.msg->transfers;
            st->error_flags = 0;
            st->status = SPI_BUS_ACTIVE;

            spi_exit_critical(irq_state);

            s_apply_config(st);
            spi_start_transfer(st);
        }
        else {
            spi_exit_critical(irq_state);
        }
    }
}
