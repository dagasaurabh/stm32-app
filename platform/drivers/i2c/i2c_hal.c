#include "i2c_hal_if.h"
#include <stddef.h>

#if defined(STM32L552xx)
#include "stm32l5xx_hal.h"
#elif defined(STM32U585xx)
#include "stm32u5xx_hal.h"
#else
#error "i2c_hal.c: unsupported MCU family — define STM32L552xx or STM32U585xx"
#endif

/* ------------------------------------------------------------------ */
/* Per-instance callback storage (supports I2C1 / I2C2 / I2C3)       */
/* ------------------------------------------------------------------ */

typedef struct {
    I2C_HandleTypeDef *handle;
    i2c_cb_t           cb;
    void              *ctx;
} i2c_hal_slot_t;

#define HAL_SLOTS 3
static i2c_hal_slot_t hal_slots[HAL_SLOTS];

static i2c_hal_slot_t *s_find_slot(I2C_HandleTypeDef *h)
{
    for (int i = 0; i < HAL_SLOTS; i++) {
        if (hal_slots[i].handle == h) return &hal_slots[i];
    }
    return NULL;
}

static i2c_hal_slot_t *s_alloc_slot(I2C_HandleTypeDef *h)
{
    i2c_hal_slot_t *s = s_find_slot(h);
    if (s) return s;
    for (int i = 0; i < HAL_SLOTS; i++) {
        if (!hal_slots[i].handle) {
            hal_slots[i].handle = h;
            return &hal_slots[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Error mapping                                                        */
/* ------------------------------------------------------------------ */

static uint32_t map_hal_error(uint32_t hal_err)
{
    uint32_t flags = I2C_ERR_NONE;
    if (hal_err & HAL_I2C_ERROR_AF)      flags |= I2C_ERR_NACK;
    if (hal_err & HAL_I2C_ERROR_TIMEOUT) flags |= I2C_ERR_TIMEOUT;
    if (hal_err & HAL_I2C_ERROR_BERR)    flags |= I2C_ERR_BERR;
    if (hal_err & HAL_I2C_ERROR_ARLO)    flags |= I2C_ERR_ARLO;
    if (hal_err & HAL_I2C_ERROR_DMA)     flags |= I2C_ERR_DMA;
    return flags ? flags : I2C_ERR_NONE;
}

/* ------------------------------------------------------------------ */
/* xfer_options mapping                                                 */
/* ------------------------------------------------------------------ */

static uint32_t map_xfer_options(i2c_xfer_opt_t opt)
{
    switch (opt) {
        case I2C_XFER_FIRST_AND_LAST: return I2C_FIRST_AND_LAST_FRAME;
        case I2C_XFER_FIRST:          return I2C_FIRST_FRAME;
        case I2C_XFER_NEXT:           return I2C_NEXT_FRAME;
        case I2C_XFER_LAST:           return I2C_LAST_FRAME;
    }
    return I2C_FIRST_AND_LAST_FRAME;
}

/* ------------------------------------------------------------------ */
/* HAL weak callback overrides                                          */
/* ------------------------------------------------------------------ */

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_hal_slot_t *s = s_find_slot(hi2c);
    if (s && s->cb) {
        i2c_cb_t cb  = s->cb;
        void    *ctx = s->ctx;
        s->cb  = NULL;
        s->ctx = NULL;
        cb(ctx, I2C_EVT_TX_DONE, I2C_ERR_NONE);
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_hal_slot_t *s = s_find_slot(hi2c);
    if (s && s->cb) {
        i2c_cb_t cb  = s->cb;
        void    *ctx = s->ctx;
        s->cb  = NULL;
        s->ctx = NULL;
        cb(ctx, I2C_EVT_RX_DONE, I2C_ERR_NONE);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_hal_slot_t *s = s_find_slot(hi2c);
    if (s && s->cb) {
        uint32_t flags = map_hal_error(hi2c->ErrorCode);
        i2c_cb_t cb  = s->cb;
        void    *ctx = s->ctx;
        s->cb  = NULL;
        s->ctx = NULL;
        cb(ctx, I2C_EVT_ERROR, flags);
    }
}

/* ------------------------------------------------------------------ */
/* Public HAL interface                                                 */
/* ------------------------------------------------------------------ */

int i2c_hal_init(void *hal)
{
    return (HAL_I2C_Init((I2C_HandleTypeDef *)hal) == HAL_OK) ? 0 : -1;
}

int i2c_hal_deinit(void *hal)
{
    return (HAL_I2C_DeInit((I2C_HandleTypeDef *)hal) == HAL_OK) ? 0 : -1;
}

int i2c_hal_abort(void *hal)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)hal;
    HAL_I2C_Master_Abort_IT(h, 0x00);
    /* Re-init to restore a clean state */
    HAL_I2C_DeInit(h);
    return (HAL_I2C_Init(h) == HAL_OK) ? 0 : -1;
}

int i2c_hal_master_tx(void *hal, uint16_t addr, const uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)hal, addr, (uint8_t *)buf,
                len, HAL_MAX_DELAY) == HAL_OK) ? 0 : -1;
}

int i2c_hal_master_rx(void *hal, uint16_t addr, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)hal, addr, buf, len,
                HAL_MAX_DELAY) == HAL_OK) ? 0 : -1;
}

/*
 * IMPORTANT — same-direction (TX→TX) sequential chaining limitation:
 * HAL_I2C_Master_Seq_Transmit_IT does not reliably suppress the repeated
 * START between same-direction segments on STM32L5/U5.  Callers must NOT
 * split a register-address + data write into two TX calls; instead combine
 * them into one buffer and call this once with I2C_FIRST_AND_LAST_FRAME.
 * See i2c_mem_write() in i2c.c for the correct implementation.
 * TX→RX direction changes (i2c_mem_read) work correctly because the HAL
 * generates the proper repeated START on direction reversal.
 */
int i2c_hal_master_seq_tx_it(void *hal, uint16_t addr, uint8_t *buf, uint16_t len,
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)hal;
    i2c_hal_slot_t *s = s_alloc_slot(h);
    if (!s) return -1;

    s->cb  = cb;
    s->ctx = ctx;

    HAL_StatusTypeDef rc = HAL_I2C_Master_Seq_Transmit_IT(h, addr, buf, len, map_xfer_options(opt));

    if (rc != HAL_OK) {
        s->cb  = NULL;
        s->ctx = NULL;
        return -1;
    }
    return 0;
}

int i2c_hal_master_seq_rx_it(void *hal, uint16_t addr, uint8_t *buf, uint16_t len,
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef *)hal;
    i2c_hal_slot_t *s = s_alloc_slot(h);
    if (!s) return -1;

    s->cb  = cb;
    s->ctx = ctx;

    HAL_StatusTypeDef rc = HAL_I2C_Master_Seq_Receive_IT(h, addr, buf, len, map_xfer_options(opt));

    if (rc != HAL_OK) {
        s->cb  = NULL;
        s->ctx = NULL;
        return -1;
    }
    return 0;
}

int i2c_hal_master_seq_tx_dma(void *hal, uint16_t addr, uint8_t *buf, uint16_t len, 
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx)
{
    /* Fall back to IT — DMA channels require board-level DMA init */
    return i2c_hal_master_seq_tx_it(hal, addr, buf, len, opt, cb, ctx);
}

int i2c_hal_master_seq_rx_dma(void *hal, uint16_t addr, uint8_t *buf, uint16_t len, 
        i2c_xfer_opt_t opt, i2c_cb_t cb, void *ctx)
{
    return i2c_hal_master_seq_rx_it(hal, addr, buf, len, opt, cb, ctx);
}
