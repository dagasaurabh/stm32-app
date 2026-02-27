#include <stdint.h>
#include "board_api.h"
#include "spi.h"
#include "pinmap.h"
#include <unistd.h>

/* SPI Loopback test
 * connect MISO to MOSI signal to test loopback functionality 
 * */

/* This test assumes:
 * 1. MISO <--> MOSI are shorted
 * 2. SPI is in full-duplex mode
 * 3. SPI is in master mode.
 * 4. Data Size  = 8 bits
 * */

int main(void)
{
    board_init();
    board_spi_init(SPI1_CS_PIN);

	gpio_init(RED_LED, GPIO_MODE_OUTPUT, GPIO_PULL_NONE);
	gpio_init(GREEN_LED, GPIO_MODE_OUTPUT, GPIO_PULL_NONE);

	gpio_write(GREEN_LED, GPIO_LOW);
	gpio_write(RED_LED, GPIO_LOW);

    int fd = spi_open("spi1");

	if(fd < 0) {
		while(1);
		__asm volatile("nop");
	}

	uint8_t out = 'A';
	uint8_t in = 0;


	sleep(2);

	while(1) {
		int rc = spi_transfer(fd, &out, &in, 1);
		if(rc == 1 && out == in) {
			gpio_write(GREEN_LED, GPIO_HIGH);
			gpio_write(RED_LED, GPIO_LOW);
		}
		else {
			gpio_write(GREEN_LED, GPIO_LOW);
			gpio_write(RED_LED, GPIO_HIGH);
		}
		sleep(2);

		gpio_write(GREEN_LED, GPIO_LOW);
		gpio_write(RED_LED, GPIO_LOW);

		if(out >= 'Z') out = 'A';
		else out += 1;

		sleep(2);
	}

    spi_close(fd);
}
