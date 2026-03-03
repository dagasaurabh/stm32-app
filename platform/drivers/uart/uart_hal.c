#include "uart_hal_if.h"
#include "stm32l5xx_hal.h"

int uart_hal_init(void *hal)
{
    return HAL_UART_Init((UART_HandleTypeDef *)hal);
}

int uart_hal_tx(void *hal, const uint8_t *buf, size_t len)
{
    HAL_StatusTypeDef st = HAL_UART_Transmit(
        (UART_HandleTypeDef *)hal,
        (uint8_t *)buf,
        (uint16_t)len,
        HAL_MAX_DELAY
    );
    return (st == HAL_OK) ? (int)len : -1;
}

int uart_hal_rx(void *hal, uint8_t *buf, size_t len)
{
    HAL_StatusTypeDef st = HAL_UART_Receive(
        (UART_HandleTypeDef *)hal,
        buf,
        (uint16_t)len,
        HAL_MAX_DELAY
    );
    return (st == HAL_OK) ? (int)len : -1;
}

