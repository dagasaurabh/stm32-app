set(P board_console)
add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/board_console.c
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
    PRIVATE uart_driver
    PRIVATE platform_common
    PRIVATE board_common
)

if (NOT TARGET hal_uart)
    message(FATAL_ERROR "board_console requires HAL UART support")
endif()

# Interface library
add_library(${P} INTERFACE)
target_link_libraries(${P} INTERFACE platform_common board_common uart_driver)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

# Expose BOARD_HAS_CONSOLE so application code can guard console-only paths
target_compile_definitions(${P} INTERFACE BOARD_HAS_CONSOLE=1)


set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
