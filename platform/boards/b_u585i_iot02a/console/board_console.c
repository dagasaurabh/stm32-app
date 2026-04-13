#include "board.h"
#include "serial.h"
#include "uart_driver.h"
#include "stm32u5xx_hal.h"

static UART_HandleTypeDef husart1;

static uart_t console_uart = {
    .hal = &husart1,
};

static int
console_open(void *ctx)
{
    (void)ctx;
    return 0;
}

static int
console_close(void *ctx)
{
    (void)ctx;
    return 0;
}

static int
console_write(void *ctx, const uint8_t *buf, size_t len)
{
    return uart_drv_write((uart_t *)ctx, buf, len);
}

static int
console_read(void *ctx, uint8_t *buf, size_t len)
{
    return uart_drv_read((uart_t *)ctx, buf, len);
}

static int
console_read_available(void *ctx)
{
    return uart_drv_read_available((uart_t *)ctx);
}

static const struct serial_ops console_ops = {
    .open           = console_open,
    .close          = console_close,
    .write          = console_write,
    .read           = console_read,
    .read_available = console_read_available,
};

static struct serial_device console_dev = {
    .name = "S0",
    .ops  = &console_ops,
    .ctx  = &console_uart,
};

/* ------------------------------------------------------------------ */
/* USART1 IRQ handler                                                  */
/* ------------------------------------------------------------------ */

void
USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&husart1);
}

void
board_console_init(void)
{
    husart1.Instance          = USART1;
    husart1.Init.BaudRate     = 115200;
    husart1.Init.WordLength   = UART_WORDLENGTH_8B;
    husart1.Init.StopBits     = UART_STOPBITS_1;
    husart1.Init.Parity       = UART_PARITY_NONE;
    husart1.Init.Mode         = UART_MODE_TX_RX;
    husart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    husart1.Init.OverSampling = UART_OVERSAMPLING_16;

    /* uart_drv_init calls HAL_UART_Init (triggering HAL_UART_MspInit to
     * configure GPIO + NVIC) then immediately arms ReceiveToIdle_IT. */
    uart_drv_init(&console_uart);

    serial_register(&console_dev, SERIAL_ROLE_STDIO);
}
