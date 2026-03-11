#include "spi_hal_if.h"
#if defined(STM32L552xx)
#include "stm32l5xx_hal.h"
#elif defined(STM32U585xx)
#include "stm32u5xx_hal.h"
#else
#error "spi_hal.c: unsupported MCU family — define STM32L552xx or STM32U585xx"
#endif

/* ------------------------------------------------------------------ */
/* Per-instance async state                                            */
/* ------------------------------------------------------------------ */

#define SPI_HAL_MAX_INSTANCES 3

static struct
{
    spi_cb_t cb;
    void *ctx;
} hal_slots[SPI_HAL_MAX_INSTANCES];

static int
instance_to_idx(const SPI_TypeDef *inst)
{
    if (inst == SPI1)
        return 0;
    if (inst == SPI2)
        return 1;
    if (inst == SPI3)
        return 2;
    return -1;
}

/* ------------------------------------------------------------------ */
/* Vendor HAL error code → portable SPI_ERR_* mapping                 */
/*                                                                     */
/* This is the ONLY place in the codebase that knows HAL_SPI_ERROR_*  */
/* values.  All other layers see only SPI_ERR_* flags from spi_types.h*/
/* ------------------------------------------------------------------ */

static uint32_t
map_hal_error(uint32_t hal_err)
{
    uint32_t flags = SPI_ERR_NONE;

    if (hal_err & HAL_SPI_ERROR_MODF)
        flags |= SPI_ERR_MODF;
    if (hal_err & HAL_SPI_ERROR_CRC)
        flags |= SPI_ERR_CRC;
    if (hal_err & HAL_SPI_ERROR_OVR)
        flags |= SPI_ERR_OVR;
    if (hal_err & HAL_SPI_ERROR_FRE)
        flags |= SPI_ERR_FRE;
    if (hal_err & HAL_SPI_ERROR_DMA)
        flags |= SPI_ERR_DMA;

    return flags;
}

/* ------------------------------------------------------------------ */
/* Blocking                                                            */
/* ------------------------------------------------------------------ */

int
spi_hal_init(void *hal)
{
    return HAL_SPI_Init((SPI_HandleTypeDef *)hal);
}

int
spi_hal_deinit(void *hal)
{
    return HAL_SPI_DeInit((SPI_HandleTypeDef *)hal);
}

int
spi_hal_tx(void *hal, const uint8_t *buf, uint16_t len)
{
    return HAL_SPI_Transmit((SPI_HandleTypeDef *)hal, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

int
spi_hal_rx(void *hal, uint8_t *buf, uint16_t len)
{
    return HAL_SPI_Receive((SPI_HandleTypeDef *)hal, buf, len, HAL_MAX_DELAY);
}

int
spi_hal_transfer(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)hal, (uint8_t *)tx, rx, len, HAL_MAX_DELAY);
}

/* ------------------------------------------------------------------ */
/* Non-blocking (IT)                                                   */
/* ------------------------------------------------------------------ */

int
spi_hal_transfer_it(void *hal, const uint8_t *tx, uint8_t *rx, uint16_t len, spi_cb_t cb, void *ctx)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hal;
    int idx = instance_to_idx(hspi->Instance);
    if (idx < 0)
        return -1;

    hal_slots[idx].cb = cb;
    hal_slots[idx].ctx = ctx;

    if (tx && rx)
        return HAL_SPI_TransmitReceive_IT(hspi, (uint8_t *)tx, rx, len);
    if (tx)
        return HAL_SPI_Transmit_IT(hspi, (uint8_t *)tx, len);
    if (rx)
        return HAL_SPI_Receive_IT(hspi, rx, len);
    return -1;
}

/* ------------------------------------------------------------------ */
/* HAL weak-function overrides — called from SPI IRQ handler          */
/* ------------------------------------------------------------------ */

static void
spi_hal_complete(SPI_HandleTypeDef *hspi, spi_evt_t event, uint32_t error_flags)
{
    int idx = instance_to_idx(hspi->Instance);
    if (idx < 0)
        return;

    spi_cb_t cb = hal_slots[idx].cb;
    void *ctx = hal_slots[idx].ctx;

    /* clear before calling cb so a re-entrant transfer_it works */
    hal_slots[idx].cb = NULL;
    hal_slots[idx].ctx = NULL;

    if (cb)
        cb(ctx, event, error_flags);
}

void
HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_hal_complete(hspi, SPI_EVT_TX_DONE, SPI_ERR_NONE);
}

void
HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_hal_complete(hspi, SPI_EVT_RX_DONE, SPI_ERR_NONE);
}

void
HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_hal_complete(hspi, SPI_EVT_TXRX_DONE, SPI_ERR_NONE);
}

void
HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    spi_hal_complete(hspi, SPI_EVT_ERROR, map_hal_error(hspi->ErrorCode));
}
