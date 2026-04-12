#include "uart_driver.h"
#include "uart_hal_if.h"
#include "platform_barrier.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * tx_done_cb — called from ISR (HAL_UART_TxCpltCallback) when the
 * current IT/DMA TX transfer ends.  Advances tx_tail and kicks the
 * next chunk if more data is pending.
 */
static void
tx_done_cb(void *ctx)
{
    uart_t *u = (uart_t *)ctx;

    u->tx_tail    = (uint16_t)((u->tx_tail + u->tx_inflight) % UART_TX_BUF_SIZE);
    u->tx_count   = (uint16_t)(u->tx_count - u->tx_inflight);
    u->tx_inflight = 0;
    u->tx_busy     = 0;

    /* Kick next chunk if more data is waiting (safe: we are in ISR) */
    if (u->tx_count > 0)
    {
        uint16_t tail  = u->tx_tail;
        uint16_t head  = u->tx_head;
        uint16_t chunk = (head > tail) ? (uint16_t)(head - tail)
                                       : (uint16_t)(UART_TX_BUF_SIZE - tail);

        u->tx_inflight = chunk;
        u->tx_busy     = 1;

        if (u->dma_tx) uart_hal_tx_dma(u->hal, &u->tx_buf[tail], chunk, tx_done_cb, u);
        else uart_hal_tx_it(u->hal, &u->tx_buf[tail], chunk, tx_done_cb, u);
    }
}

/*
 * rx_done_cb — called from ISR (HAL_UARTEx_RxEventCallback) when
 * ReceiveToIdle delivers data into the bounce buffer.  Copies bytes
 * to the RX ring, then immediately re-arms.
 */
static void
rx_done_cb(void *ctx, uint16_t size)
{
    uart_t *u = (uart_t *)ctx;

    for (uint16_t i = 0; i < size; i++)
    {
        uint16_t next = (uint16_t)((u->rx_head + 1u) % UART_RX_BUF_SIZE);
        if (next != u->rx_tail) /* not full */
        {
            u->rx_buf[u->rx_head] = u->rx_bounce[i];
            u->rx_head            = next;
            u->rx_count++;
        }
        /* else: ring full — byte is silently dropped */
    }

    /* Re-arm immediately (always uses same bounce buffer) */
    if (u->dma_rx) uart_hal_rx_dma(u->hal, u->rx_bounce, UART_RX_BOUNCE, rx_done_cb, u);
    else uart_hal_rx_it(u->hal, u->rx_bounce, UART_RX_BOUNCE, rx_done_cb, u);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int
uart_drv_init(uart_t *u)
{
    if (!u || !u->hal) return -1;

    /* Zero ring-buffer state (caller may have a pre-zeroed static, but
     * be safe in case of re-init after uart_drv_deinit). */
    u->tx_head = u->tx_tail = u->tx_count = u->tx_inflight = u->tx_busy = 0;
    u->rx_head = u->rx_tail = u->rx_count = 0;

    if (uart_hal_init(u->hal) != 0) return -1;

    /* Arm RX immediately — it stays armed for the lifetime of the driver */
    int rc;
    if (u->dma_rx) {
        rc = uart_hal_rx_dma(u->hal, u->rx_bounce, UART_RX_BOUNCE, rx_done_cb, u);
    }
    else {
        rc = uart_hal_rx_it(u->hal, u->rx_bounce, UART_RX_BOUNCE, rx_done_cb, u);
    }

    return rc;
}

void
uart_drv_deinit(uart_t *u)
{
    if (!u || !u->hal) return;

    uart_hal_deinit(u->hal);

    u->tx_busy     = 0;
    u->tx_inflight = 0;
    u->tx_count    = 0;
    u->rx_count    = 0;
}

int
uart_drv_configure(uart_t *u, const uart_cfg_t *cfg)
{
    if (!u || !cfg) return -1;
    return uart_hal_configure(u->hal, cfg->baud, cfg->wordlen, cfg->stop, cfg->parity);
}

int
uart_drv_write(uart_t *u, const uint8_t *buf, size_t len)
{
    if (!u || !buf) return -1;
    if (len == 0) return 0;

    uint32_t primask;
    PLATFORM_IRQ_SAVE(primask);

    size_t accepted = 0;
    while (accepted < len)
    {
        uint16_t next = (uint16_t)((u->tx_head + 1u) % UART_TX_BUF_SIZE);
        if (next == u->tx_tail) break; /* TX ring full */
        u->tx_buf[u->tx_head] = buf[accepted++];
        u->tx_head            = next;
        u->tx_count++;
    }

    /* Kick TX if idle and we added data */
    if (accepted > 0 && !u->tx_busy)
    {
        uint16_t tail  = u->tx_tail;
        uint16_t head  = u->tx_head;
        uint16_t chunk = (head > tail) ? (uint16_t)(head - tail)
                                       : (uint16_t)(UART_TX_BUF_SIZE - tail);

        u->tx_inflight = chunk;
        u->tx_busy     = 1;

        if (u->dma_tx) uart_hal_tx_dma(u->hal, &u->tx_buf[tail], chunk, tx_done_cb, u);
        else uart_hal_tx_it(u->hal, &u->tx_buf[tail], chunk, tx_done_cb, u);
    }

    PLATFORM_IRQ_RESTORE(primask);
    return (int)accepted;
}

int
uart_drv_read(uart_t *u, uint8_t *buf, size_t len)
{
    if (!u || !buf) return -1;

    uint32_t primask;
    PLATFORM_IRQ_SAVE(primask);

    size_t n = 0;
    while (n < len && u->rx_count > 0)
    {
        buf[n++]   = u->rx_buf[u->rx_tail];
        u->rx_tail = (uint16_t)((u->rx_tail + 1u) % UART_RX_BUF_SIZE);
        u->rx_count--;
    }

    PLATFORM_IRQ_RESTORE(primask);
    return (int)n;
}

int
uart_drv_read_available(uart_t *u)
{
    if (!u) return -1;
    return (int)u->rx_count;
}
