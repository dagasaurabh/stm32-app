set(P board_core)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/board_core.c
)


target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
    PRIVATE gpio_driver
    PRIVATE board_common
    PRIVATE platform_common
)

# Interface library
add_library(${P} INTERFACE)
target_link_libraries(${P} INTERFACE
    board_common
    platform_common
    gpio_driver)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)
