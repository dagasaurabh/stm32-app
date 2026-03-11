#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "i2c.h"
#include "regmap_i2c.h"
#include "iis2mdc.h"
#include "sensor_mgr.h"
#include "pfmt.h"
#include "platform_barrier.h"

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

#define BUS_ERR_THRESHOLD  3

/*
 * Bus recovery bookkeeping.
 *
 * bus_sensor_t — per-sensor slot: own error count + back-pointer to the bus.
 *   Passed as `ctx` to sensor_mgr_subscribe so each callback updates only
 *   its own counter and sets the correct bus's needs_recovery flag.
 *
 * i2c_bus_t — per-bus state: slave fd, sensor slots, recovery flag.
 *   i2c_reset(fd) resolves the bus from any slave fd on it and aborts all
 *   in-flight transfers.  All sensor slots must be restarted together.
 *
 * To add a sensor on the same bus: increase sensors[] size, populate the
 * new slot in main(), and subscribe with &g_bus.sensors[n].
 */
typedef struct i2c_bus_s i2c_bus_t;
typedef struct {
    i2c_bus_t *bus;
    int        handle;
    int        err_count;
} bus_sensor_t;
struct i2c_bus_s {
    int           fd;
    bus_sensor_t  sensors[1];    /* one entry per sensor on this bus */
    int           n_sensors;
    volatile bool needs_recovery;
};

static i2c_bus_t      g_bus;
static regmap_i2c_t   g_map;
static iis2mdc_data_t g_sample;   /* written from ISR via struct copy */
static volatile bool  g_ready;    /* set last in ISR, checked first in main */

static void on_iis2mdc_data(int handle, const void *data, int status, void *ctx)
{
    bus_sensor_t *s = (bus_sensor_t *)ctx;
    (void)handle;
    if (status != 0) {
        /* Only set the flag — i2c_reset/HAL_DeInit must not be called from ISR */
        if (++s->err_count >= BUS_ERR_THRESHOLD)
            s->bus->needs_recovery = true;
        return;
    }
    s->err_count = 0;
    g_sample = *(const iis2mdc_data_t *)data;  /* struct copy */
    PLATFORM_RELEASE_STORE();   /* all stores above visible before g_ready */
    g_ready = true;
}

int main(void)
{
    board_init();
    board_console_init();
    board_i2c_init();

    board_i2c_add_slave(IIS2MDC_BUS, IIS2MDC_SLAVE, IIS2MDC_ADDR, I2C_ADDR_7BIT, I2C_SPEED_FAST);

    g_bus.fd        = i2c_open(IIS2MDC_SLAVE);
    g_bus.n_sensors = 1;

    sensor_t *ops = iis2mdc_init(regmap_i2c_init(&g_map, g_bus.fd));
    if (!ops) {
        printf("IIS2MDC init failed\r\n");
        while (1) {}
    }

    int h = sensor_mgr_register(SENSOR_IIS2MDC, ops, 100, 1);
    if (h < 0) {
        printf("sensor_mgr_register failed: %d\r\n", h);
        while (1) {}
    }
    g_bus.sensors[0] = (bus_sensor_t){ .bus = &g_bus, .handle = h };
    if (sensor_mgr_subscribe(h, on_iis2mdc_data, &g_bus.sensors[0]) < 0) {
        printf("sensor_mgr_subscribe failed\r\n");
        while (1) {}
    }
    if (sensor_mgr_stream_start(h) < 0) {
        printf("sensor_mgr_stream_start failed\r\n");
        while (1) {}
    }

    while (1) {
        i2c_poll();
        sensor_mgr_poll();

        if (g_bus.needs_recovery) {
            i2c_reset(g_bus.fd);          /* abort all in-flight transfers;
                                             abort callbacks may bump err_counts */
            g_bus.needs_recovery = false; /* clear AFTER i2c_reset */
            for (int i = 0; i < g_bus.n_sensors; i++) {
                g_bus.sensors[i].err_count = 0;
                sensor_mgr_reset(g_bus.sensors[i].handle);
                sensor_mgr_stream_start(g_bus.sensors[i].handle);
            }
        }

        if (g_ready) {
            g_ready = false;
            PLATFORM_ACQUIRE_LOAD();   /* g_ready cleared before reading g_sample */
            printf("M="); pf2(g_sample.mx); printf(" "); pf2(g_sample.my); printf(" "); pf2(g_sample.mz);
            printf(" gauss\r\n");
        }
    }
}
