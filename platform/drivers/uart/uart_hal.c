#include "uart_hal_if.h"
#if defined(STM32L552xx)
#include "stm32l5xx_hal.h"
#elif defined(STM32U585xx)
#include "stm32u5xx_hal.h"
#else
#error "uart_hal.c: unsupported MCU family - define STM32L552xx or STM32U585xx"
#endif

/* ------------------------------------------------------------------ */
/* Per-instance callback storage                                       */
/*                                                                     */
/* TX and RX callbacks are stored separately: TX is one-shot (cleared */
/* after firing) while RX is persistent (re-armed from driver layer). */
/* ------------------------------------------------------------------ */

#define UART_HAL_MAX_INSTANCES 4

typedef struct
{
    USART_TypeDef    *instance; /* NULL = free slot */
    uart_hal_tx_cb_t  tx_cb;
    void             *tx_ctx;
    uart_hal_rx_cb_t  rx_cb;
    void             *rx_ctx;
} uart_hal_slot_t;

static uart_hal_slot_t hal_slots[UART_HAL_MAX_INSTANCES];

static uart_hal_slot_t *
s_find_slot(const USART_TypeDef *inst)
{
    for (int i = 0; i < UART_HAL_MAX_INSTANCES; i++)
    {
        if (hal_slots[i].instance == inst)
            return &hal_slots[i];
    }
    return NULL;
}

static uart_hal_slot_t *
s_alloc_slot(USART_TypeDef *inst)
{
    uart_hal_slot_t *s = s_find_slot(inst);
    if (s) return s;
    for (int i = 0; i < UART_HAL_MAX_INSTANCES; i++)
    {
        if (!hal_slots[i].instance)
        {
            hal_slots[i].instance = inst;
            return &hal_slots[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* HAL word-length / stop-bit / parity mapping                        */
/* ------------------------------------------------------------------ */

static uint32_t
map_wordlen(uint32_t w)
{
    if (w == 7) return UART_WORDLENGTH_7B;
    if (w == 9) return UART_WORDLENGTH_9B;
    return UART_WORDLENGTH_8B;
}

static uint32_t
map_stopbits(uint32_t s)
{
    return (s == 2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
}

static uint32_t
map_parity(uint32_t p)
{
    if (p == 1) return UART_PARITY_ODD;
    if (p == 2) return UART_PARITY_EVEN;
    return UART_PARITY_NONE;
}

/* ------------------------------------------------------------------ */
/* Blocking init / deinit / configure                                  */
/* ------------------------------------------------------------------ */

int
uart_hal_init(void *hal)
{
    return (HAL_UART_Init((UART_HandleTypeDef *)hal) == HAL_OK) ? 0 : -1;
}

void
uart_hal_deinit(void *hal)
{
    HAL_UART_DeInit((UART_HandleTypeDef *)hal);
}

int
uart_hal_configure(void *hal, uint32_t baud, uint32_t wordlen,
                   uint32_t stop, uint32_t parity)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)hal;

    huart->Init.BaudRate   = baud;
    huart->Init.WordLength = map_wordlen(wordlen);
    huart->Init.StopBits   = map_stopbits(stop);
    huart->Init.Parity     = map_parity(parity);

    return (HAL_UART_Init(huart) == HAL_OK) ? 0 : -1;
}

/* ------------------------------------------------------------------ */
/* IT transmit                                                         */
/* ------------------------------------------------------------------ */

int
uart_hal_tx_it(void *hal, const uint8_t *buf, uint16_t len,
               uart_hal_tx_cb_t cb, void *ctx)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)hal;
    uart_hal_slot_t *s = s_alloc_slot(huart->Instance);
    if (!s) return -1;

    s->tx_cb  = cb;
    s->tx_ctx = ctx;

    HAL_StatusTypeDef rc = HAL_UART_Transmit_IT(huart, (uint8_t *)buf, len);
    if (rc != HAL_OK)
    {
        s->tx_cb  = NULL;
        s->tx_ctx = NULL;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* IT receive (ReceiveToIdle)                                          */
/* ------------------------------------------------------------------ */

int
uart_hal_rx_it(void *hal, uint8_t *buf, uint16_t buf_len,
               uart_hal_rx_cb_t cb, void *ctx)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)hal;
    uart_hal_slot_t *s = s_alloc_slot(huart->Instance);
    if (!s) return -1;

    s->rx_cb  = cb;
    s->rx_ctx = ctx;

    HAL_StatusTypeDef rc = HAL_UARTEx_ReceiveToIdle_IT(huart, buf, buf_len);
    if (rc != HAL_OK)
    {
        s->rx_cb  = NULL;
        s->rx_ctx = NULL;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* DMA transmit                                                        */
/* ------------------------------------------------------------------ */

int
uart_hal_tx_dma(void *hal, const uint8_t *buf, uint16_t len,
                uart_hal_tx_cb_t cb, void *ctx)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)hal;

    /* Fall back to IT if no DMA handle was wired up in MspInit */
    if (!huart->hdmatx)
        return uart_hal_tx_it(hal, buf, len, cb, ctx);

    uart_hal_slot_t *s = s_alloc_slot(huart->Instance);
    if (!s)
        return -1;

    s->tx_cb  = cb;
    s->tx_ctx = ctx;

    HAL_StatusTypeDef rc = HAL_UART_Transmit_DMA(huart, (uint8_t *)buf, len);
    if (rc != HAL_OK)
    {
        s->tx_cb  = NULL;
        s->tx_ctx = NULL;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* DMA receive (ReceiveToIdle)                                         */
/* ------------------------------------------------------------------ */

int
uart_hal_rx_dma(void *hal, uint8_t *buf, uint16_t buf_len,
                uart_hal_rx_cb_t cb, void *ctx)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)hal;

    /* Fall back to IT if no DMA handle was wired up in MspInit */
    if (!huart->hdmarx)
        return uart_hal_rx_it(hal, buf, buf_len, cb, ctx);

    uart_hal_slot_t *s = s_alloc_slot(huart->Instance);
    if (!s)
        return -1;

    s->rx_cb  = cb;
    s->rx_ctx = ctx;

    HAL_StatusTypeDef rc = HAL_UARTEx_ReceiveToIdle_DMA(huart, buf, buf_len);
    if (rc != HAL_OK)
    {
        s->rx_cb  = NULL;
        s->rx_ctx = NULL;
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* HAL weak-function overrides — called from UART IRQ handler         */
/* ------------------------------------------------------------------ */

void
HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_hal_slot_t *s = s_find_slot(huart->Instance);
    if (!s || !s->tx_cb)
        return;

    uart_hal_tx_cb_t cb  = s->tx_cb;
    void            *ctx = s->tx_ctx;

    /* Clear before calling so a re-entrant uart_hal_tx_it within the
     * callback can safely reuse the same slot. */
    s->tx_cb  = NULL;
    s->tx_ctx = NULL;

    cb(ctx);
}

void
HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    uart_hal_slot_t *s = s_find_slot(huart->Instance);
    if (!s || !s->rx_cb) return;

    /* RX callback is NOT cleared here — the driver re-arms from within
     * the callback, which updates the slot in-place. */
    s->rx_cb(s->rx_ctx, Size);
}

void
HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uart_hal_slot_t *s = s_find_slot(huart->Instance);
    if (!s) return;

    /* Only signal TX completion for TX-side faults.
     *
     * RX-line errors (PE/FE/NE/ORE) can happen while TX is still in-flight;
     * treating those as "TX done" can make the driver drop queued bytes.
     * DMA error is the only HAL error class that can be TX-related here, and
     * it is considered TX-side only when the TX DMA stream reports an error.
     */
    const uint32_t tx_dma_fault =
        ((huart->ErrorCode & HAL_UART_ERROR_DMA) != 0U) &&
        (huart->hdmatx != NULL) &&
        (huart->hdmatx->ErrorCode != HAL_DMA_ERROR_NONE);

    if (s->tx_cb && tx_dma_fault)
    {
        uart_hal_tx_cb_t cb  = s->tx_cb;
        void            *ctx = s->tx_ctx;
        s->tx_cb  = NULL;
        s->tx_ctx = NULL;
        cb(ctx);
    }

    /* Clear RX slot; driver will re-arm on next poll / re-init. */
    s->rx_cb  = NULL;
    s->rx_ctx = NULL;
}
