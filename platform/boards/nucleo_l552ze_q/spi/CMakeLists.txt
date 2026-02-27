set(P board_spi)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/board_spi.c
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
    PRIVATE spi_driver
    PRIVATE platform_common
    PRIVATE board_common
)

if (NOT TARGET hal_spi)
    message(FATAL_ERROR "board_spi requires HAL SPI support")
endif()

# Interface library
add_library(${P} INTERFACE)
target_link_libraries(${P} INTERFACE platform_common board_common spi_driver)
target_compile_definitions(${P} INTERFACE BOARD_HAS_SPI)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
