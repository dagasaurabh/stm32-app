set(P spi_loopback_test)
project(${P} C ASM)

add_executable(${P}.elf
    main.c
)

target_link_libraries(${P}.elf
    ${MCU_TOOLCHAIN_LIB}
    mcu_core
    board_core
    board_spi
)

build_app(${P})
