set(P platform_common)


#---------------- Serial -------------------------
set(SERIAL_UART_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/serial/include)
set(SERIAL_UART_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/serial/src/serial.c
    ${CMAKE_CURRENT_LIST_DIR}/src/serial/src/syscalls.c
)
#-------------------------------------------------

#---------------- GPIO ---------------------------

set(GPIO_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/gpio/include)
set(GPIO_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/gpio/src/gpio.c)

#-------------------------------------------------


#-------- Time Utility Object library ------------
set(TIME_UTILS_SRCS 
    ${CMAKE_CURRENT_LIST_DIR}/src/time/src/systime.c
    ${CMAKE_CURRENT_LIST_DIR}/src/time/src/time_math.c
    ${CMAKE_CURRENT_LIST_DIR}/src/time/src/timer.c
)
set(TIME_UTILS_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/time/include)
#-------------------------------------------------

#---------------- SPI ----------------------------

set(SPI_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/spi/include ${TIME_UTILS_INCLUDE_DIR})
set(SPI_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/spi/src/spi.c)

#-------------------------------------------------

#---------------- I2C ----------------------------

set(I2C_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/i2c/include)
set(I2C_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/i2c/src/i2c.c)

#-------------------------------------------------

#---------------- Regmap -------------------------

set(REGMAP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/regmap)
set(REGMAP_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/regmap/regmap_i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/regmap/regmap_spi.c
)

#-------------------------------------------------

#----- Object library ----------------------------

add_library(${P}_time_utils_obj OBJECT ${TIME_UTILS_SRCS})

target_include_directories(${P}_time_utils_obj PRIVATE ${TIME_UTILS_INCLUDE_DIR})

target_link_libraries(${P}_time_utils_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE mcu_api
)

#-------------------------------------------------

add_library(${P}_obj OBJECT
    ${SERIAL_UART_SRCS}
    ${GPIO_SRCS}
    ${SPI_SRCS}
    ${I2C_SRCS}
    ${REGMAP_SRCS}
)

target_include_directories(${P}_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${SERIAL_UART_INCLUDE_DIR}
    ${GPIO_INCLUDE_DIR}
    ${SPI_INCLUDE_DIR}
    ${I2C_INCLUDE_DIR}
    ${REGMAP_INCLUDE_DIR}
    ${MCU_HAL_INCLUDES}
)

target_link_libraries(${P}_obj
    PRIVATE ${MCU_TOOLCHAIN_LIB}
)
#-------------------------------------------------

# Interface library
add_library(${P} INTERFACE)
target_include_directories(${P} INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
	${SERIAL_UART_INCLUDE_DIR}
	${GPIO_INCLUDE_DIR}
	${SPI_INCLUDE_DIR}
	${I2C_INCLUDE_DIR}
	${REGMAP_INCLUDE_DIR}
	${TIME_UTILS_INCLUDE_DIR}
)

target_sources(${P} INTERFACE 
	$<TARGET_OBJECTS:${P}_obj>
	$<TARGET_OBJECTS:${P}_time_utils_obj>)

set_property(TARGET ${P}_obj PROPERTY EXCLUDE_FROM_ALL TRUE)
set_property(TARGET ${P}_time_utils_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

