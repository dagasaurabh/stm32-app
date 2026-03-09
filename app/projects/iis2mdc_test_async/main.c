#include <stdio.h>
#include <stdbool.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "iis2mdc.h"

/* Board-specific I2C configuration
 * IIS2MDC has a fixed I2C address (0x1E) — no SA0 pin.
 * b_u585i_iot02a: on-board MEMS bus is I2C2 (PH4/PH5).
 * All other boards: connect breakout to I2C1; address is the same. */
#if defined(BOARD_B_U585I_IOT02A)
#  define IIS2MDC_BUS    "i2c2"
#  define IIS2MDC_SLAVE  "i2c2.iis2mdc"
#  define IIS2MDC_ADDR   0x1E
#else
/* Breakout board default: I2C1, fixed address 0x1E */
#  define IIS2MDC_BUS    "i2c1"
#  define IIS2MDC_SLAVE  "i2c1.iis2mdc"
#  define IIS2MDC_ADDR   0x1E
#endif

static regmap_i2c_t  g_map;
static iis2mdc_t     g_dev;

static volatile bool  g_ready;
static volatile float g_mx, g_my, g_mz;

static void on_done(void *ctx, float mx, float my, float mz)
{
    (void)ctx;
    g_mx = mx;
    g_my = my;
    g_mz = mz;
    g_ready = true;
}

int main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(IIS2MDC_BUS, IIS2MDC_SLAVE, IIS2MDC_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_i2c_init(&g_map, i2c_open(IIS2MDC_SLAVE));
    if (iis2mdc_init(&g_dev, &g_map.map) < 0)
        printf("IIS2MDC init failed\r\n");

    iis2mdc_read_async(&g_dev, on_done, NULL);

    while (1) {
        i2c_poll();
        if (g_ready) {
            g_ready = false;
            printf("M=%.2f %.2f %.2f gauss\r\n", g_mx, g_my, g_mz);
            iis2mdc_read_async(&g_dev, on_done, NULL);
        }
    }
}
