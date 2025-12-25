set(P blinky)
project(${P} C ASM)

add_executable(${P}.elf
    main.c
)

use_mcu(${P})
use_board(${P})
use_hal(${P} gpio)

# board is needed for header files, object libraries do not propagate header
# files
target_link_libraries(${P}.elf
    board
)

build_app(${P})
