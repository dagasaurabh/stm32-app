set(P i2c_driver)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/i2c_driver.c
    ${CMAKE_CURRENT_LIST_DIR}/i2c_hal.c
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}/platform/common/include            # for i2c_types.h
    ${CMAKE_SOURCE_DIR}/platform/common/src/gpio/include   # for gpio.h
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB})

if (NOT TARGET hal_i2c)
    message(FATAL_ERROR "i2c_driver requires HAL I2C support")
endif()

# Interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}/platform/common/include        # for i2c_types.h
    ${CMAKE_SOURCE_DIR}/platform/common/src/gpio/include  # for gpio.h
)
target_link_libraries(${P} INTERFACE hal_i2c)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
