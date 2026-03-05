#include "board_api.h"
#include "i2c.h"
#include "i2c_driver.h"
#include "gpio.h"
#include "stm32u5xx_hal.h"

/* SCL = PB8, SDA = PB9 */
#define I2C1_SCL_PIN  GPIO_PIN_ENCODE(1, 8)
#define I2C1_SDA_PIN  GPIO_PIN_ENCODE(1, 9)

/* Timing values for 160 MHz PCLK1 (from STM32 I2C timing tool) */
#define I2C1_TIMING_STANDARD   0x30909DECU  /* 100 kHz */
#define I2C1_TIMING_FAST       0x10C0ECFFU  /* 400 kHz */
#define I2C1_TIMING_FAST_PLUS  0x00802172U  /* 1 MHz   */

static I2C_HandleTypeDef hi2c1;

static i2c_t i2c1_drv = {
    .hal     = &hi2c1,
    .name    = "i2c1",
    .scl_pin = I2C1_SCL_PIN,
    .sda_pin = I2C1_SDA_PIN,
};

/* ------------------------------------------------------------------ */
/* Slave pool                                                           */
/* ------------------------------------------------------------------ */

#define BOARD_I2C_MAX_SLAVES 8
static struct i2c_slave slave_pool[BOARD_I2C_MAX_SLAVES];
static int slave_count;

/* ------------------------------------------------------------------ */
/* Bus ops                                                              */
/* ------------------------------------------------------------------ */

static int board_i2c1_open(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_i2c1_close(void *ctx)
{
    (void)ctx;
    return 0;
}

static int board_i2c1_transfer_one(void *ctx, uint16_t addr, i2c_dir_t dir,
                                   uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt)
{
    return i2c_drv_transfer_one((i2c_t *)ctx, addr, dir, buf, len, opt);
}

static int board_i2c1_transfer_one_it(void *ctx, uint16_t addr, i2c_dir_t dir,
                                      uint8_t *buf, uint16_t len, i2c_xfer_opt_t opt,
                                      uint8_t use_dma, i2c_cb_t cb, void *cb_ctx)
{
    return i2c_drv_transfer_one_it((i2c_t *)ctx, addr, dir, buf, len, opt,
                                   use_dma, cb, cb_ctx);
}

static int board_i2c1_apply_config(void *ctx, i2c_speed_t speed, i2c_addr_mode_t addr_mode)
{
    i2c_t *dev = (i2c_t *)ctx;

    uint32_t timing;
    switch (speed) {
    case I2C_SPEED_FAST:      timing = I2C1_TIMING_FAST;      break;
    case I2C_SPEED_FAST_PLUS: timing = I2C1_TIMING_FAST_PLUS; break;
    default:                  timing = I2C1_TIMING_STANDARD;  break;
    }

    uint32_t addr_mode_hal = (addr_mode == I2C_ADDR_10BIT) ?
        I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;

    i2c_drv_deinit(dev);
    hi2c1.Init.Timing         = timing;
    hi2c1.Init.AddressingMode = addr_mode_hal;
    i2c_drv_init(dev);
    return 0;
}

static int board_i2c1_recover(void *ctx, uint32_t error_flags)
{
    (void)error_flags;
    i2c_t *dev = (i2c_t *)ctx;
    i2c_drv_abort(dev);
    return 0;
}

/*
 * board_i2c1_bus_recover — GPIO bit-bang 9-clock recovery.
 */
static int board_i2c1_bus_recover(void *ctx)
{
    i2c_t *dev = (i2c_t *)ctx;

    i2c_drv_deinit(dev);

    gpio_init(I2C1_SCL_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_UP);
    gpio_init(I2C1_SDA_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_UP);

    gpio_write(I2C1_SDA_PIN, GPIO_HIGH);
    gpio_write(I2C1_SCL_PIN, GPIO_HIGH);

    for (int i = 0; i < 9; i++) {
        gpio_write(I2C1_SCL_PIN, GPIO_LOW);
        for (volatile int d = 0; d < 100; d++) __asm volatile("nop");
        gpio_write(I2C1_SCL_PIN, GPIO_HIGH);
        for (volatile int d = 0; d < 100; d++) __asm volatile("nop");

        if (gpio_read(I2C1_SDA_PIN) == GPIO_HIGH) break;
    }

    /* STOP condition */
    gpio_write(I2C1_SDA_PIN, GPIO_LOW);
    for (volatile int d = 0; d < 100; d++) __asm volatile("nop");
    gpio_write(I2C1_SCL_PIN, GPIO_HIGH);
    for (volatile int d = 0; d < 100; d++) __asm volatile("nop");
    gpio_write(I2C1_SDA_PIN, GPIO_HIGH);

    return i2c_drv_init(dev);
}

static const struct i2c_bus_ops i2c1_ops = {
    .open            = board_i2c1_open,
    .close           = board_i2c1_close,
    .transfer_one    = board_i2c1_transfer_one,
    .transfer_one_it = board_i2c1_transfer_one_it,
    .apply_config    = board_i2c1_apply_config,
    .recover         = board_i2c1_recover,
    .bus_recover     = board_i2c1_bus_recover,
};

static struct i2c_bus i2c1_bus = {
    .name = "i2c1",
    .ops  = &i2c1_ops,
    .ctx  = &i2c1_drv,
};

/* ------------------------------------------------------------------ */
/* IRQ handlers                                                         */
/* ------------------------------------------------------------------ */

void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&hi2c1);
}

/* ------------------------------------------------------------------ */
/* Init                                                                 */
/* ------------------------------------------------------------------ */

void board_i2c_init(void)
{
    hi2c1.Instance              = I2C1;
    hi2c1.Init.Timing           = I2C1_TIMING_STANDARD;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    i2c_drv_init(&i2c1_drv);
    i2c_bus_register(&i2c1_bus);
}

int board_i2c_add_slave(const char *bus_name, const char *name,
                         uint16_t addr, i2c_addr_mode_t addr_mode,
                         i2c_speed_t speed)
{
    if (slave_count >= BOARD_I2C_MAX_SLAVES) return -1;

    struct i2c_slave *s = &slave_pool[slave_count++];
    s->name      = name;
    s->bus_name  = bus_name;
    s->addr      = addr;
    s->addr_mode = addr_mode;
    s->speed     = speed;

    return i2c_slave_register(s);
}
