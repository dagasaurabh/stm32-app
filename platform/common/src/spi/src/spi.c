#include "spi.h"
#include "gpio.h"
#include <string.h>
#include "cmsis_compiler.h"  /* __get_PRIMASK, __set_PRIMASK, __disable_irq */
//#include "cmsis_compiler.h"
#include "cmsis_gcc.h"

static struct spi_bus   *bus_table[SPI_MAX_BUSES];
static struct spi_slave *slave_table[SPI_MAX_SLAVES];
static struct spi_slave *fd_table[SPI_MAX_SLAVES];

/* SPI Registration */

int spi_bus_register(struct spi_bus *bus)
{
    if (!bus || !bus->ops || !bus->name) return -1;

    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (!bus_table[i]) {
            bus_table[i] = bus;
            if (bus->ops->open) bus->ops->open(bus->ctx);
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


int spi_open(const char *slave_name)
{
    if (!slave_name) return -1;

    for (int s = 0; s < SPI_MAX_SLAVES; s++) {
        if (!slave_table[s] || strcmp(slave_table[s]->name, slave_name) != 0) continue;

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

int spi_close(int fd)
{
    if (fd < 0 || fd >= SPI_MAX_SLAVES || !fd_table[fd]) return -1;
    fd_table[fd] = NULL;
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

/*
 * spi_sync — execute a message atomically.
 *
 * CS is asserted before the first transfer.  Between transfers,
 * cs_change=1 causes a brief de assert/re assert.
 * CS is always de asserted at the end of the message.
 */
int spi_sync(int fd, struct spi_message *msg)
{
    if (fd < 0 || fd >= SPI_MAX_SLAVES || !fd_table[fd]) return -1;
    if (!msg || !msg->transfers) return -1;

    struct spi_slave *slave = fd_table[fd];

    struct spi_bus *bus = NULL;
    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (bus_table[i] && strcmp(bus_table[i]->name, slave->bus_name) == 0) {
            bus = bus_table[i];
            break;
        }
    }
    if (!bus || !bus->ops->transfer_one) return -1;

    gpio_write(slave->cs_pin, GPIO_LOW);

    int ret = 0;
    for (struct spi_transfer *xfer = msg->transfers; xfer; xfer = xfer->next) {
        ret = bus->ops->transfer_one(bus->ctx, xfer->tx_buf, xfer->rx_buf, xfer->len);
        if (ret) break;

        if (xfer->cs_change && xfer->next) {
            gpio_write(slave->cs_pin, GPIO_HIGH);
            gpio_write(slave->cs_pin, GPIO_LOW);
        }
    }

    gpio_write(slave->cs_pin, GPIO_HIGH);
    return ret;
}

/* ------------------------------------------------------------------ */
/* Async (interrupt-driven) with per-bus message queue                 */
/* ------------------------------------------------------------------ */

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

/* Per-bus pending queue entry */
typedef struct {
    int                fd;
    struct spi_message *msg;
} spi_queued_t;

#define SPI_ASYNC_QUEUE_DEPTH 4

/* Per-bus async state */
typedef struct {
    /* Active transfer */
    int                  fd;
    struct spi_bus      *bus;
    struct spi_message  *msg;          /* NULL = idle */
    struct spi_transfer *current;
    uint32_t             error_flags;  /* accumulated SPI_ERR_* bits, 0 = ok */

    /* Pending message queue (ring buffer) */
    spi_queued_t         queue[SPI_ASYNC_QUEUE_DEPTH];
    uint8_t              q_head;
    uint8_t              q_tail;
    uint8_t              q_count;
} spi_bus_state_t;

static spi_bus_state_t bus_state[SPI_MAX_BUSES];

static void spi_async_irq_cb(void *ctx, spi_evt_t event, uint32_t error_flags);

/*
 * spi_start_transfer - assert CS and kick off the first transfer of a message.
 * Called from both spi_async() (task context) and spi_async_irq_cb() (ISR context).
 * Caller must have already filled st->{fd, bus, msg, current, status}.
 */
static void spi_start_transfer(spi_bus_state_t *st)
{
    struct spi_slave *slave = fd_table[st->fd];
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
    struct spi_slave *slave = fd_table[st->fd];

    if (event == SPI_EVT_ERROR) st->error_flags |= error_flags;

    /* cs_change between two segments of the same message */
    if (event != SPI_EVT_ERROR && st->current->cs_change && st->current->next) {
        gpio_write(slave->cs_pin, GPIO_HIGH);
        gpio_write(slave->cs_pin, GPIO_LOW);
    }

    st->current = st->current->next;

    if (st->current && st->error_flags == 0) {
        /* More transfers remain in this message — continue */
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
        st->q_head  = (uint8_t)((st->q_head + 1) % SPI_ASYNC_QUEUE_DEPTH);
        st->q_count--;

        st->fd          = next.fd;
        /* st->bus stays the same — all queued msgs are for this bus */
        st->msg         = next.msg;
        st->current     = next.msg->transfers;
        st->error_flags = 0;

        spi_start_transfer(st);
    } else {
        st->msg = NULL; /* bus is now idle */
    }

    /* Trigger user callback for the just-completed message */
    if (complete) complete(context, final_evt, final_err);
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
 */
int spi_async(int fd, struct spi_message *msg)
{
    if (fd < 0 || fd >= SPI_MAX_SLAVES || !fd_table[fd]) return -1;
    if (!msg || !msg->transfers || !msg->complete) return -1;

    struct spi_slave *slave = fd_table[fd];

    struct spi_bus *bus = NULL;
    int bus_idx = -1;
    for (int i = 0; i < SPI_MAX_BUSES; i++) {
        if (bus_table[i] && strcmp(bus_table[i]->name, slave->bus_name) == 0) {
            bus     = bus_table[i];
            bus_idx = i;
            break;
        }
    }
    if (!bus || !bus->ops->transfer_one_it) return -1;

    spi_bus_state_t *st = &bus_state[bus_idx];

    /*
     * Critical section covers only the idle-check + queue/state mutation.
     * GPIO and HAL IT calls (which can be slow or block briefly) are
     * outside the critical section.
     */
    uint32_t irq_state = spi_enter_critical();

    if (st->msg == NULL) {
        /* Bus idle — set up state and mark busy before exiting critical */
        st->fd          = fd;
        st->bus         = bus;
        st->msg         = msg;
        st->current     = msg->transfers;
        st->error_flags = 0;
        spi_exit_critical(irq_state);

        spi_start_transfer(st);
    } else {
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
