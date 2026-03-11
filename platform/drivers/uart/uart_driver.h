#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    void *hal;        /* HAL handle (opaque) */
    const char *name; /* "S0", "S1", ... */
} uart_t;

/* Driver API */
int
uart_init(uart_t *u);
int
uart_write(uart_t *u, const uint8_t *buf, size_t len);
int
uart_read(uart_t *u, uint8_t *buf, size_t len);
