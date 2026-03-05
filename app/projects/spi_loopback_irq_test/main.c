#include <stdint.h>
#include "board_api.h"
#include "spi.h"
#include "pinmap.h"
#include <unistd.h>

/* SPI Loopback test with spi interrupt
 * connect MISO to MOSI signal to test loopback functionality 
 * */

/* This test assumes:
 * 1. MISO <--> MOSI are shorted
 * 2. SPI is in full-duplex mode
 * 3. SPI is in master mode.
 * 4. Data Size  = 8 bits
 * */

struct Env {
    uint8_t out;
    uint8_t in;
    bool loop_complete;
    bool error;
    struct spi_message async_msg;
    struct spi_transfer async_xfer;
}env;

static void on_transfer_done(void *ctx, spi_evt_t event, uint32_t error_flags)
{
    struct Env * self = (struct Env*) ctx;

    if(event == SPI_EVT_TXRX_DONE) {
        self->loop_complete = true;
        self->error = false;
    }
    else if(event == SPI_EVT_ERROR) {
        self->loop_complete = false;
        self->error = true;
    }

    if(event == SPI_EVT_TXRX_DONE || event == SPI_EVT_ERROR) {
        if(self->out >= 'Z') self->out = 'A';
        else self->out += 1;
    }
}

static void s_init_msg(struct Env * env) {
    env->async_xfer = (struct spi_transfer){
        .tx_buf = &env->out,
            .rx_buf = &env->in,
            .len    = 1,
    };

    spi_message_init(&env->async_msg);
    spi_message_add_transfer(&env->async_msg, &env->async_xfer);
    env->async_msg.complete = on_transfer_done;
    env->async_msg.context  = env;
}

int main(void)
{
    board_init();
    board_spi_init();

    /*
     * Register slaves on a specific bus - bus_name + CS pin per device.
     * Multiple slaves on the same bus, or on different buses, are each
     * registered with the appropriate bus_name.
     */

    board_spi_add_slave("spi1", "spi1.0", SPI1_CS_PIN, SPI_MODE_0, SPI_CLKDIV_16, SPI_DATASIZE_8);

    gpio_init(RED_LED, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW);
    gpio_init(GREEN_LED, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW);

    gpio_write(GREEN_LED, GPIO_LOW);
    gpio_write(RED_LED, GPIO_LOW);

    int fd = spi_open("spi1.0");

    if(fd < 0) {
        while(1) {
            __asm volatile("nop");
        }
    }

    env.out = 'A';
    env.in = 0;

    s_init_msg(&env);
    spi_async(fd, &env.async_msg);

    while(1) {
        if(env.loop_complete) {
            env.loop_complete = false;
            gpio_write(GREEN_LED, GPIO_HIGH);
            gpio_write(RED_LED, GPIO_LOW);
            sleep(2);
            gpio_write(GREEN_LED, GPIO_LOW);
            sleep(1);
            s_init_msg(&env);
            spi_async(fd, &env.async_msg);
        }
        else if(env.error) {
            env.error = false;
            gpio_write(GREEN_LED, GPIO_LOW);
            gpio_write(RED_LED, GPIO_HIGH);
            sleep(2);
            gpio_write(RED_LED, GPIO_LOW);
            sleep(1);
            s_init_msg(&env);
            spi_async(fd, &env.async_msg);
        }
        spi_poll();
    }

    spi_close(fd);
    while(1) {
        __asm volatile("nop");
    }
}
