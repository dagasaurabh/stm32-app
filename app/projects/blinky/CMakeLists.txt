set(P blinky)
project(${P} C ASM)

add_executable(${P}.elf
    main.c
)

target_link_libraries(${P}.elf
    board
)

build_app(${P})
