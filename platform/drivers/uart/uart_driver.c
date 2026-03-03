#include "uart_driver.h"
#include "uart_hal_if.h"

int uart_init(uart_t *u)
{
    if (!u || !u->hal) return -1;
    return uart_hal_init(u->hal);
}

int uart_write(uart_t *u, const uint8_t *buf, size_t len)
{
    if (!u || !u->hal || !buf || len == 0) return -1;
    return uart_hal_tx(u->hal, buf, len);
}

int uart_read(uart_t *u, uint8_t *buf, size_t len)
{
    if (!u || !u->hal || !buf || len == 0) return -1;
    return uart_hal_rx(u->hal, buf, len);
}

