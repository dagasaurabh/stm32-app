#include "board_api.h"
#include "spi.h"
#include "spi_driver.h"
#include "gpio.h"
#include "stm32l5xx_hal.h"

static SPI_HandleTypeDef hspi1;

static spi_t spi1_drv = {
    .hal  = &hspi1,
    .name = "spi1",
};

static int board_spi1_open(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_spi1_close(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_spi1_transfer_one(void *ctx,
                                    const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return spi_drv_transfer_one((spi_t *)ctx, tx, rx, len);
}

static int board_spi1_transfer_one_it(void *ctx,
                                       const uint8_t *tx, uint8_t *rx, uint16_t len,
                                       spi_cb_t cb, void *cb_ctx)
{
    /* spi_cb_t and the driver's callback type share the same definition from spi_types.h */
    return spi_drv_transfer_one_it((spi_t *)ctx, tx, rx, len, cb, cb_ctx);
}

static void map_mode(spi_mode_t mode, uint32_t *polarity, uint32_t *phase)
{

    switch (mode) {
        case SPI_MODE_0:
            *polarity = SPI_POLARITY_LOW;
            *phase    = SPI_PHASE_1EDGE;
            break;

        case SPI_MODE_1:
            *polarity = SPI_POLARITY_LOW;
            *phase    = SPI_PHASE_2EDGE;
            break;

        case SPI_MODE_2:
            *polarity = SPI_POLARITY_HIGH;
            *phase    = SPI_PHASE_1EDGE;
            break;

        case SPI_MODE_3:
            *polarity = SPI_POLARITY_HIGH;
            *phase    = SPI_PHASE_2EDGE;
            break;
    }
}

static uint32_t map_clkdiv(spi_clkdiv_t div)
{
    switch (div) {
        case SPI_CLKDIV_2:   return SPI_BAUDRATEPRESCALER_2;
        case SPI_CLKDIV_4:   return SPI_BAUDRATEPRESCALER_4;
        case SPI_CLKDIV_8:   return SPI_BAUDRATEPRESCALER_8;
        case SPI_CLKDIV_16:  return SPI_BAUDRATEPRESCALER_16;
        case SPI_CLKDIV_32:  return SPI_BAUDRATEPRESCALER_32;
        case SPI_CLKDIV_64:  return SPI_BAUDRATEPRESCALER_64;
        case SPI_CLKDIV_128: return SPI_BAUDRATEPRESCALER_128;
        case SPI_CLKDIV_256: return SPI_BAUDRATEPRESCALER_256;
    }
    return SPI_BAUDRATEPRESCALER_8; /* default */
}

static uint32_t map_datasize(spi_datasize_t ds)
{
    switch (ds) {
        case SPI_DATASIZE_8:  return SPI_DATASIZE_8BIT;
        case SPI_DATASIZE_16: return SPI_DATASIZE_16BIT;
    }
    return SPI_DATASIZE_8BIT; /* default */
}

static int board_spi1_apply_config(void *ctx, spi_mode_t mode, spi_clkdiv_t clkdiv,
        spi_datasize_t datasize)
{
    spi_t             *drv  = (spi_t *)ctx;
    SPI_HandleTypeDef *hspi = drv->hal;

    uint32_t polarity = SPI_POLARITY_LOW;
    uint32_t phase = SPI_PHASE_1EDGE;
    uint32_t prescaler = SPI_BAUDRATEPRESCALER_16;
    uint32_t datawidth = SPI_DATASIZE_8BIT;

    map_mode(mode, &polarity, &phase);
    prescaler = map_clkdiv(clkdiv);
    datawidth = map_datasize(datasize);

    if(polarity != hspi->Init.CLKPolarity ||
            phase != hspi->Init.CLKPhase ||
            prescaler != hspi->Init.BaudRatePrescaler ||
            datawidth != hspi->Init.DataSize) {

        spi_drv_deinit((spi_t *)ctx);

        hspi->Init.CLKPolarity       = polarity;
        hspi->Init.CLKPhase          = phase;
        hspi->Init.BaudRatePrescaler = prescaler;
        hspi->Init.DataSize          = datawidth;

        return spi_drv_init((spi_t *)ctx);
    }
    return 0;
}

static int board_spi1_recover(void *ctx, uint32_t error_flags)
{
    spi_t             *drv  = (spi_t *)ctx;
    SPI_HandleTypeDef *hspi = drv->hal;

    (void)error_flags;

    /* Abort ongoing transfer */
    HAL_SPI_Abort(hspi);

    spi_drv_deinit(drv);
    return spi_drv_init(drv);
}

static const struct spi_bus_ops spi1_bus_ops = {
    .open           = board_spi1_open,
    .close          = board_spi1_close,
    .transfer_one   = board_spi1_transfer_one,
    .transfer_one_it = board_spi1_transfer_one_it,
    .apply_config   = board_spi1_apply_config,
    .recover = board_spi1_recover,
};

static struct spi_bus spi1_bus = {
    .name = "spi1",
    .ops  = &spi1_bus_ops,
    .ctx  = &spi1_drv,
};

/* SPI1 IRQ handler — owned here because hspi1 lives here */
void SPI1_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&hspi1);
}

/* Board-level slave pool (shared across all buses on this board)      */

#define BOARD_SPI_MAX_SLAVES 8
static struct spi_slave slave_pool[BOARD_SPI_MAX_SLAVES];
static int slave_count;

void board_spi_init(void)
{
    /* SPI1: master, CPOL=low, CPHA=1-edge, 8-bit, prescaler=16 */
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 7;

    spi_drv_init(&spi1_drv);
    spi_bus_register(&spi1_bus);
}

/*
 * board_spi_add_slave — register a slave device on a named bus.
 *
 * bus_name : must match a registered spi_bus ("spi1", "spi2", …),bus_name should be
 * available through out the program.
 * name     : unique slave name, e.g. "spi1.0", "spi1.1". name should be
 * available through out the program.
 * cs_pin   : GPIO pin used as chip select (active-low, idle-high)
 * mode     : spi mode
 * prescaler: clock prescaler
 * datasize : data size
 */
int board_spi_add_slave(const char *bus_name, const char *name, gpio_pin_t cs_pin,
        spi_mode_t mode, spi_clkdiv_t prescaler, spi_datasize_t datasize)
{
    if (!bus_name || !name || slave_count >= BOARD_SPI_MAX_SLAVES) return -1;

    struct spi_slave *s = &slave_pool[slave_count];
    s->name     = name;
    s->bus_name = bus_name;
    s->cs_pin   = cs_pin;
    s->mode     = mode;
    s->prescaler = prescaler;
    s->datasize = datasize;

    /* Drive CS idle-high before the first transfer */
    gpio_init(cs_pin, GPIO_MODE_OUTPUT, GPIO_PULL_NONE);
    gpio_write(cs_pin, GPIO_HIGH);

    int rc = spi_slave_register(s);
    if (rc == 0) slave_count++;
    return rc;
}
