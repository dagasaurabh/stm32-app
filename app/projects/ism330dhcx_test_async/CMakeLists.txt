set(P ism330dhcx_test_async)
project(${P} C ASM)

add_executable(${P}.elf
    main.c
)

target_link_libraries(${P}.elf
    ${MCU_TOOLCHAIN_LIB}
    mcu_core
    board_core
    board_console
    board_i2c
    ism330dhcx
)

build_app(${P})
