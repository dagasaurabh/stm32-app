#include <stdio.h>
#include <stdint.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "timer.h"
#include "systime.h"
#include "lps22hh.h"
#include "pfmt.h"

/* Board-specific I2C configuration
 * LPS22HH address is selected by the SA0 pin: SA0=HIGH → 0x5D, SA0=LOW → 0x5C.
 * b_u585i_iot02a: on-board MEMS bus is I2C2, SA0 tied HIGH → 0x5D.
 * All other boards: connect breakout to I2C1; tie SA0 low (float/GND) → 0x5C. */
#if defined(BOARD_B_U585I_IOT02A)
#  define LPS22HH_BUS    "i2c2"
#  define LPS22HH_SLAVE  "i2c2.lps22hh"
#  define LPS22HH_ADDR   0x5D
#else
/* Breakout board default: I2C1, SA0 low → 0x5C */
#  define LPS22HH_BUS    "i2c1"
#  define LPS22HH_SLAVE  "i2c1.lps22hh"
#  define LPS22HH_ADDR   0x5C
#endif

static regmap_i2c_t g_map;

static void delay_ms(uint32_t ms)
{
    mstimer_t t;
    mstimer_start(&t, ms);
    while (!mstimer_expired(&t)) {}
}

int main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(LPS22HH_BUS, LPS22HH_SLAVE, LPS22HH_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_t  *map = regmap_i2c_init(&g_map, i2c_open(LPS22HH_SLAVE));
    sensor_t  *dev = lps22hh_init(map);
    if (!dev)
        printf("LPS22HH init failed\r\n");

    while (1) {
        lps22hh_data_t data = {0};
        sensor_read_sync(dev, &data);
        printf("P="); pf2(data.pressure_hpa);
        printf(" hPa  T="); pf1(data.temp_c); printf(" C\r\n");
        delay_ms(500);
    }
}
