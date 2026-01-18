#pragma once

void board_init(void);

#ifdef BOARD_HAS_CONSOLE
void board_console_init(void);
#endif
