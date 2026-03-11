set(P uart_driver)

add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/uart_driver.c
    ${CMAKE_CURRENT_LIST_DIR}/uart_hal.c
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB})

if (NOT TARGET hal_uart)
    message(FATAL_ERROR "uart_driver requires HAL UART support")
endif()

#interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${P} INTERFACE hal_uart)
target_sources(${P} INTERFACE $<TARGET_OBJECTS:${P}_obj>)

#This prevents someone from doing: target_link_libraries(app uart_driver_obj)
set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
