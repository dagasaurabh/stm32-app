#include <stdio.h>
#include "board_api.h"

int main(void) {

    board_init();
    board_console_init();

    while(1) {
        fprintf(stderr, "Hello World");
        //msleep(100);
    }
}
