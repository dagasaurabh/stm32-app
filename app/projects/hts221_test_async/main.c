#include <stdio.h>
#include <stdbool.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "hts221.h"

/* Board-specific I2C configuration
 * HTS221 has a fixed I2C address (0x5F) — no SA0 pin.
 * b_u585i_iot02a: on-board MEMS bus is I2C2 (PH4/PH5).
 * All other boards: connect breakout to I2C1; address is the same. */
#if defined(BOARD_B_U585I_IOT02A)
#  define HTS221_BUS    "i2c2"
#  define HTS221_SLAVE  "i2c2.hts221"
#  define HTS221_ADDR   0x5F
#else
/* Breakout board default: I2C1, fixed address 0x5F */
#  define HTS221_BUS    "i2c1"
#  define HTS221_SLAVE  "i2c1.hts221"
#  define HTS221_ADDR   0x5F
#endif

static regmap_i2c_t  g_map;
static hts221_t      g_dev;

static volatile bool  g_ready;
static volatile float g_t, g_h;

static void on_done(void *ctx, float t, float h)
{
    (void)ctx;
    g_t = t;
    g_h = h;
    g_ready = true;
}

int main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(HTS221_BUS, HTS221_SLAVE, HTS221_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_i2c_init(&g_map, i2c_open(HTS221_SLAVE));
    if (hts221_init(&g_dev, &g_map.map) < 0)
        printf("HTS221 init failed\r\n");

    hts221_read_async(&g_dev, on_done, NULL);

    while (1) {
        i2c_poll();
        if (g_ready) {
            g_ready = false;
            printf("T=%.1f C  H=%.1f%%\r\n", g_t, g_h);
            hts221_read_async(&g_dev, on_done, NULL);
        }
    }
}
