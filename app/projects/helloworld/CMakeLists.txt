set(P helloworld)
project(${P} C ASM)

add_executable(${P}.elf
    main.c
)

target_link_libraries(${P}.elf
    ${MCU_TOOLCHAIN_LIB}
    mcu_core
    board_core
    board_console
)

build_app(${P})
