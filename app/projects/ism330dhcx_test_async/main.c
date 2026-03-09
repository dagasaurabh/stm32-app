#include <stdio.h>
#include <stdbool.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "ism330dhcx.h"

/* Board-specific I2C configuration
 * ISM330DHCX address is selected by the SA0 pin: SA0=HIGH → 0x6B, SA0=LOW → 0x6A.
 * b_u585i_iot02a: on-board MEMS bus is I2C2, SA0 tied HIGH → 0x6B.
 * All other boards: connect breakout to I2C1; tie SA0 low (float/GND) → 0x6A. */
#if defined(BOARD_B_U585I_IOT02A)
#  define ISM330DHCX_BUS    "i2c2"
#  define ISM330DHCX_SLAVE  "i2c2.ism330dhcx"
#  define ISM330DHCX_ADDR   0x6B
#else
/* Breakout board default: I2C1, SA0 low → 0x6A */
#  define ISM330DHCX_BUS    "i2c1"
#  define ISM330DHCX_SLAVE  "i2c1.ism330dhcx"
#  define ISM330DHCX_ADDR   0x6A
#endif

static regmap_i2c_t   g_map;
static ism330dhcx_t   g_dev;

static volatile bool  g_ready;
static volatile float g_ax, g_ay, g_az, g_gx, g_gy, g_gz;

static void on_done(void *ctx,
                    float ax, float ay, float az,
                    float gx, float gy, float gz)
{
    (void)ctx;
    g_ax = ax; g_ay = ay; g_az = az;
    g_gx = gx; g_gy = gy; g_gz = gz;
    g_ready = true;
}

int main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(ISM330DHCX_BUS, ISM330DHCX_SLAVE, ISM330DHCX_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_i2c_init(&g_map, i2c_open(ISM330DHCX_SLAVE));
    if (ism330dhcx_init(&g_dev, &g_map.map) < 0)
        printf("ISM330DHCX init failed\r\n");

    ism330dhcx_read_async(&g_dev, on_done, NULL);

    while (1) {
        i2c_poll();
        if (g_ready) {
            g_ready = false;
            printf("A=%.3f %.3f %.3f g   G=%.2f %.2f %.2f dps\r\n",
                   g_ax, g_ay, g_az, g_gx, g_gy, g_gz);
            ism330dhcx_read_async(&g_dev, on_done, NULL);
        }
    }
}
