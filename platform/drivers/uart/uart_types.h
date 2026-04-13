#pragma once

#include <stdint.h>

/*
 * UART driver callback types.
 *
 * TX callback: fired when the IT/DMA transmit transfer completes.
 * RX callback: fired when ReceiveToIdle completes (idle line detected or
 *              buffer full), receiving the number of bytes placed in the
 *              bounce buffer.
 */
typedef void (*uart_hal_tx_cb_t)(void *ctx);
typedef void (*uart_hal_rx_cb_t)(void *ctx, uint16_t size);

/*
 * Runtime configuration passed to uart_hal_configure() and
 * uart_drv_configure().
 *
 * wordlen: 7, 8, or 9
 * stop:    1 or 2
 * parity:  0 = none, 1 = odd, 2 = even
 */
typedef struct
{
    uint32_t baud;
    uint32_t wordlen;
    uint32_t stop;
    uint32_t parity;
} uart_cfg_t;
