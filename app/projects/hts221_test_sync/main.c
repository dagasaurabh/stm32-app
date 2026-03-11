#include <stdio.h>
#include <stdint.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "timer.h"
#include "systime.h"
#include "hts221.h"
#include "pfmt.h"

/* Board-specific I2C configuration
 * HTS221 has a fixed I2C address (0x5F) — no SA0 pin.
 * b_u585i_iot02a: on-board MEMS bus is I2C2 (PH4/PH5).
 * All other boards: connect breakout to I2C1; address is the same. */
#if defined(BOARD_B_U585I_IOT02A)
#define HTS221_BUS "i2c2"
#define HTS221_SLAVE "i2c2.hts221"
#define HTS221_ADDR 0x5F
#else
/* Breakout board default: I2C1, fixed address 0x5F */
#define HTS221_BUS "i2c1"
#define HTS221_SLAVE "i2c1.hts221"
#define HTS221_ADDR 0x5F
#endif

static regmap_i2c_t g_map;

static void
delay_ms(uint32_t ms)
{
    mstimer_t t;
    mstimer_start(&t, ms);
    while (!mstimer_expired(&t))
    {
    }
}

int
main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(HTS221_BUS, HTS221_SLAVE, HTS221_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_t *map = regmap_i2c_init(&g_map, i2c_open(HTS221_SLAVE));
    sensor_t *dev = hts221_init(map);

    if (!dev)
    {
        printf("HTS221 init failed\r\n");
        while (1)
            delay_ms(1000);
    }

    while (1)
    {
        hts221_data_t data = {0};
        sensor_read_sync(dev, &data);
        printf("T=");
        pf1(data.temp_c);
        printf(" C  H=");
        pf1(data.humidity_pct);
        printf("%%\r\n");
        delay_ms(500);
    }
}
