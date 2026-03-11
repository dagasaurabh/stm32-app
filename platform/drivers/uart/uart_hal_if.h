#pragma once

#include <stdint.h>
#include <stddef.h>

int
uart_hal_init(void *hal);
int
uart_hal_tx(void *hal, const uint8_t *buf, size_t len);
int
uart_hal_rx(void *hal, uint8_t *buf, size_t len);
