set(P board_common)
add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/hal_msp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/stm32u5xx_it.c
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
)

# Interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_compile_definitions(${P} INTERFACE
    BOARD_HAS_LED
    BOARD_HAS_BUTTON
    BOARD_HAS_CONSOLE
)

target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)


add_subdirectory(core)
if(CONFIG_BOARD_CONSOLE)
add_subdirectory(console)
endif()
add_subdirectory(spi)
add_subdirectory(i2c)
