#include <stdio.h>
#include <stdint.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "timer.h"
#include "systime.h"
#include "iis2mdc.h"
#include "pfmt.h"

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

    board_i2c_add_slave(IIS2MDC_BUS, IIS2MDC_SLAVE, IIS2MDC_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    regmap_t  *map = regmap_i2c_init(&g_map, i2c_open(IIS2MDC_SLAVE));
    sensor_t  *dev = iis2mdc_init(map);
    if (!dev) {
        printf("IIS2MDC init failed\r\n");
        while (1) delay_ms(1000);
    }

    while (1) {
        iis2mdc_data_t data = {0};
        sensor_read_sync(dev, &data);
        printf("M="); pf2(data.mx); printf(" "); pf2(data.my); printf(" "); pf2(data.mz);
        printf(" gauss\r\n");
        delay_ms(500);
    }
}
