set(P board_i2c)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/board_i2c.c
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/drivers/i2c
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
    PRIVATE i2c_driver
    PRIVATE platform_common
    PRIVATE board_common
)

if (NOT TARGET hal_i2c)
    message(FATAL_ERROR "board_i2c requires HAL I2C support")
endif()

# Interface library
add_library(${P} INTERFACE)
target_link_libraries(${P} INTERFACE platform_common board_common i2c_driver)
target_compile_definitions(${P} INTERFACE BOARD_HAS_I2C)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
