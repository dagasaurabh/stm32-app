#include "board_api.h"
#include "spi.h"
#include "spi_driver.h"
#include "gpio.h"
#include "stm32l5xx_hal.h"

static SPI_HandleTypeDef hspi1;

typedef struct {
    spi_t      drv;
    gpio_pin_t cs_pin;
} board_spi_ctx_t;

static board_spi_ctx_t spi1_ctx = {
    .drv = {
        .hal  = &hspi1,
        .name = "spi1",
    },
};

static int board_spi_open(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_spi_close(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_spi_write(void *ctx, const uint8_t *buf, uint16_t len)
{
    board_spi_ctx_t *c = (board_spi_ctx_t *)ctx;

    gpio_write(c->cs_pin, GPIO_LOW);

    int ret = spi_drv_tx(&c->drv, buf, len);

    gpio_write(c->cs_pin, GPIO_HIGH);

    return ret;
}

static int board_spi_read(void *ctx, uint8_t *buf, uint16_t len)
{
    board_spi_ctx_t *c = (board_spi_ctx_t *)ctx;

    gpio_write(c->cs_pin, GPIO_LOW);

    int ret = spi_drv_rx(&c->drv, buf, len);

    gpio_write(c->cs_pin, GPIO_HIGH);

    return ret;
}

static int board_spi_transfer(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    board_spi_ctx_t *c = (board_spi_ctx_t *)ctx;

    gpio_write(c->cs_pin, GPIO_LOW);

    int ret = spi_drv_transfer(&c->drv, tx, rx, len);

    gpio_write(c->cs_pin, GPIO_HIGH);

    return ret;
}

static const struct spi_bus_ops spi1_ops = {
    .open     = board_spi_open,
    .close    = board_spi_close,
    .write    = board_spi_write,
    .read     = board_spi_read,
    .transfer = board_spi_transfer,
};

static struct spi_device spi1_dev = {
    .name = "spi1",
    .ops  = &spi1_ops,
    .ctx  = &spi1_ctx,
};

void board_spi_init(gpio_pin_t cs_pin)
{
    spi1_ctx.cs_pin = cs_pin;

    /* Drive CS high (idle) before init */
    gpio_init(cs_pin, GPIO_MODE_OUTPUT, GPIO_PULL_NONE);
    gpio_write(cs_pin, GPIO_HIGH);

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

    spi_drv_init(&spi1_ctx.drv);
    spi_register(&spi1_dev);
}
