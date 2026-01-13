#pragma once

void board_init(void);

#ifdef BOARD_HAS_LED
/* LEDs */
void board_led_on(void);
void board_led_off(void);
void board_led_toggle(void);
#endif

#ifdef BOARD_HAS_BUTTON
bool board_button_is_pressed(void);
#endif

#ifdef BOARD_HAS_CONSOLE
void board_console_init(void);
#endif
