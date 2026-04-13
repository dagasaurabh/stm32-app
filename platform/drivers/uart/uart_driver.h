#pragma once

#include <stdint.h>
#include <stddef.h>
#include "uart_types.h"

/* ------------------------------------------------------------------ */
/* Buffer sizes                                                        */
/* ------------------------------------------------------------------ */

#define UART_TX_BUF_SIZE 256u /* TX ring buffer - bytes accepted non-blocking  */
#define UART_RX_BUF_SIZE 256u /* RX ring buffer - bytes stored before drain    */
#define UART_RX_BOUNCE    64u /* ReceiveToIdle target (IT or DMA bounce)       */

/* ------------------------------------------------------------------ */
/* Driver instance                                                     */
/*                                                                     */
/* Caller zero-initialises the struct, then sets .hal (and optionally  */
/* .dma_tx / .dma_rx) before calling uart_drv_init().                 */
/* ------------------------------------------------------------------ */

typedef struct uart_s
{
    void *hal;    /* UART_HandleTypeDef *  (must be set before uart_drv_init) */
    void *dma_tx; /* DMA_HandleTypeDef *   NULL → interrupt mode              */
    void *dma_rx; /* DMA_HandleTypeDef *   NULL → interrupt mode              */

    /* TX ring buffer ------------------------------------------------- */
    uint8_t           tx_buf[UART_TX_BUF_SIZE];
    volatile uint16_t tx_head;     /* written by uart_drv_write (thread ctx) */
    volatile uint16_t tx_tail;     /* advanced by tx_done_cb (ISR)           */
    volatile uint16_t tx_count;    /* bytes pending in TX ring               */
    volatile uint16_t tx_inflight; /* bytes in the current IT/DMA transfer   */
    volatile uint8_t  tx_busy;     /* 1 = IT/DMA transfer in flight          */

    /* RX ring buffer ------------------------------------------------- */
    uint8_t           rx_buf[UART_RX_BUF_SIZE];
    volatile uint16_t rx_head;  /* advanced by rx_done_cb (ISR)             */
    volatile uint16_t rx_tail;  /* advanced by uart_drv_read (thread ctx)   */
    volatile uint16_t rx_count; /* bytes available in RX ring               */

    /* RX bounce buffer (ReceiveToIdle target) ------------------------ */
    uint8_t rx_bounce[UART_RX_BOUNCE];
} uart_t;

/* ------------------------------------------------------------------ */
/* Driver API                                                          */
/* ------------------------------------------------------------------ */

/*
 * uart_drv_init — init the UART peripheral and arm RX immediately.
 *   Caller must have set u->hal (and optionally u->dma_tx/dma_rx).
 *   Returns 0 on success, -1 on error.
 */
int  uart_drv_init(uart_t *u);

/*
 * uart_drv_deinit — stop TX/RX, deinit peripheral.
 */
void uart_drv_deinit(uart_t *u);

/*
 * uart_drv_configure — change baud/format at runtime without
 *   re-configuring GPIO (MspInit already set up the pins).
 *   Returns 0 on success, -1 on error.
 */
int  uart_drv_configure(uart_t *u, const uart_cfg_t *cfg);

/*
 * uart_drv_write — enqueue bytes into the TX ring buffer and kick the
 *   IT/DMA transfer if idle.  Non-blocking; returns the number of bytes
 *   accepted (may be less than len if the ring is full).
 *   Returns -1 if u or buf is NULL.
 */
int  uart_drv_write(uart_t *u, const uint8_t *buf, size_t len);

/*
 * uart_drv_read — drain bytes from the RX ring into buf.  Non-blocking;
 *   returns 0 if no data is available.
 *   Returns -1 if u or buf is NULL.
 */
int  uart_drv_read(uart_t *u, uint8_t *buf, size_t len);

/*
 * uart_drv_read_available — number of bytes waiting in the RX ring.
 *   Returns -1 if u is NULL.
 */
int  uart_drv_read_available(uart_t *u);
