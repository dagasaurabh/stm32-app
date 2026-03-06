set(P gpio_driver)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/gpio_driver.c
    ${CMAKE_CURRENT_LIST_DIR}/gpio_hal.c
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}/platform/common/include  # for gpio_types.h
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB})

if (NOT TARGET hal_gpio)
    message(FATAL_ERROR "uart_driver requires HAL GPIO support")
endif()

# Interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}/platform/common/include  # for gpio_types.h
)
target_link_libraries(${P} INTERFACE hal_gpio)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

#This prevents someone from doing: target_link_libraries(app gpio_driver_obj)
set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
