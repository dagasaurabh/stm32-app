set(SERIAL_UART_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/serial/src/serial.c
    ${CMAKE_CURRENT_LIST_DIR}/src/serial/src/syscalls.c
)

set(SERIAL_UART_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/serial/include)

set(P platform_common)
add_library(${P}_obj OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${SERIAL_UART_SRCS}
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${SERIAL_UART_INCLUDE_DIR}
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
)

# Interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
	${SERIAL_UART_INCLUDE_DIR}
)
target_sources(platform_common INTERFACE $<TARGET_OBJECTS:${P}_obj>)

set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

