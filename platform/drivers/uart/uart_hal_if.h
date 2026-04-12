#pragma once

#include <stdint.h>
#include <stddef.h>
#include "uart_types.h"

/*
 * UART HAL interface — the only layer that knows STM32 HAL types.
 * uart_driver.c calls these; uart_hal.c implements them.
 * Tests substitute mock_uart_hal.c instead.
 */

/* Blocking init / deinit -------------------------------------------------- */

int  uart_hal_init(void *hal);
void uart_hal_deinit(void *hal);

/*
 * Runtime baud/format change.  GPIO is already configured by MspInit;
 * only the peripheral registers are updated.
 */
int  uart_hal_configure(void *hal, uint32_t baud, uint32_t wordlen,
                        uint32_t stop, uint32_t parity);

/* Interrupt-driven (IT) --------------------------------------------------- */

/*
 * Start a non-blocking IT transmit.
 * cb(ctx) is called from HAL_UART_TxCpltCallback when the transfer ends.
 */
int  uart_hal_tx_it(void *hal, const uint8_t *buf, uint16_t len,
                    uart_hal_tx_cb_t cb, void *ctx);

/*
 * Arm ReceiveToIdle_IT on the bounce buffer.
 * cb(ctx, size) is called from HAL_UARTEx_RxEventCallback each time the
 * IDLE line fires or the buffer fills.  The driver re-arms from within cb.
 */
int  uart_hal_rx_it(void *hal, uint8_t *buf, uint16_t buf_len,
                    uart_hal_rx_cb_t cb, void *ctx);

/* DMA variants (optional — board must configure DMA handles in MspInit) --- */

/*
 * Start a non-blocking DMA transmit.
 * Same callback contract as uart_hal_tx_it.
 * Falls back to IT if huart->hdmatx is NULL (set by board MspInit).
 */
int  uart_hal_tx_dma(void *hal, const uint8_t *buf, uint16_t len,
                     uart_hal_tx_cb_t cb, void *ctx);

/*
 * Arm ReceiveToIdle_DMA on the bounce buffer.
 * Same callback contract as uart_hal_rx_it.
 * Falls back to IT if huart->hdmarx is NULL (set by board MspInit).
 */
int  uart_hal_rx_dma(void *hal, uint8_t *buf, uint16_t buf_len,
                     uart_hal_rx_cb_t cb, void *ctx);
