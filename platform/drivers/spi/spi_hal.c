#include "spi_hal_if.h"
#include "stm32l5xx_hal.h"

int spi_hal_init(void *hal)
{
    return HAL_SPI_Init((SPI_HandleTypeDef *)hal);
}

int spi_hal_tx(void *hal, const uint8_t *buf, uint16_t len)
{
    return HAL_SPI_Transmit(
        (SPI_HandleTypeDef *)hal,
        (uint8_t *)buf,
        len,
		HAL_MAX_DELAY
    );
}

int spi_hal_rx(void *hal, uint8_t *buf, uint16_t len)
{
    return HAL_SPI_Receive(
        (SPI_HandleTypeDef *)hal,
        buf,
        len,
        HAL_MAX_DELAY
    );
}

int spi_hal_transfer(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return HAL_SPI_TransmitReceive(
        (SPI_HandleTypeDef *)hal,
        (uint8_t *)tx,
        rx,
        len,
        HAL_MAX_DELAY
    );
}
