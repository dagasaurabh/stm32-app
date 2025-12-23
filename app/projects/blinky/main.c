#include "board_api.h"

int main(void)
{
    board_init();

    while (1) {
        board_led_toggle();
        for (volatile int i = 0; i < 200000; i++);
    }
}

