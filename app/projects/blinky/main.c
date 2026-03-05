#include "gpio.h"
#include "pinmap.h"
#include "board_api.h"
#include <unistd.h>

int main(void)
{
    board_init();

    gpio_init(LED_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW);

    while (1) {
        gpio_toggle(LED_PIN);
        sleep(1);
    }
}

