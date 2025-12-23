
# Core HAL (always required)
set(HAL_CORE_SOURCES
    stm32l5xx_hal.c
    stm32l5xx_hal_cortex.c
)

# Peripheral groups
set(HAL_GPIO_SOURCES
    stm32l5xx_hal_gpio.c
)

set(HAL_RCC_SOURCES
    stm32l5xx_hal_rcc.c
    stm32l5xx_hal_rcc_ex.c
)

set(HAL_UART_SOURCES
    stm32l5xx_hal_uart.c
    stm32l5xx_hal_uart_ex.c
)

set(HAL_I2C_SOURCES
    stm32l5xx_hal_i2c.c
    stm32l5xx_hal_i2c_ex.c
)

set(HAL_DMA_SOURCES
    stm32l5xx_hal_dma.c
)

set(HAL_FLASH_SOURCES
    stm32l5xx_hal_flash.c
    stm32l5xx_hal_flash_ex.c
)

set(HAL_PWR_SOURCES
    stm32l5xx_hal_pwr.c
    stm32l5xx_hal_pwr_ex.c
)

# Always include core HAL
set(HAL_SOURCES ${HAL_CORE_SOURCES})

# --------------------------------------------------
# HAL peripheral dependency rules
# --------------------------------------------------

# Format:
# set(HAL_DEP_<PERIPHERAL> <dep1> <dep2> ...)

set(HAL_DEP_UART GPIO RCC)
set(HAL_DEP_I2C  GPIO RCC)
set(HAL_DEP_SPI  GPIO RCC)
set(HAL_DEP_ADC  RCC)
set(HAL_DEP_DAC  RCC)
set(HAL_DEP_DMA  RCC)
set(HAL_DEP_FLASH RCC)
set(HAL_DEP_PWR  RCC)

# Core peripherals (no deps or implicit)
set(HAL_DEP_GPIO RCC)
set(HAL_DEP_RCC  "")
set(HAL_DEP_CORTEX "")

set(_HAL_PERIPHERALS_NORMALIZED "")
foreach(P ${HAL_PERIPHERALS})
    string(TOUPPER ${P} P_UP)
    list(APPEND _HAL_PERIPHERALS_NORMALIZED ${P_UP})
endforeach()
set(HAL_PERIPHERALS ${_HAL_PERIPHERALS_NORMALIZED})


# --------------------------------------------------
# Validate HAL dependencies
# --------------------------------------------------

foreach(PERIPH ${HAL_PERIPHERALS})

    if(NOT DEFINED HAL_DEP_${PERIPH})
        message(FATAL_ERROR
            "HAL validation error:\n"
            "  Unknown HAL peripheral '${PERIPH}'\n"
            "  Check hal_dependencies.cmake"
        )
    endif()

    foreach(DEP ${HAL_DEP_${PERIPH}})
        list(FIND HAL_PERIPHERALS ${DEP} DEP_INDEX)
        if(DEP_INDEX EQUAL -1)
            message(FATAL_ERROR
                "HAL dependency error:\n"
                "  Peripheral '${PERIPH}' requires '${DEP}'\n"
                "  Add '${DEP}' to HAL_PERIPHERALS in board.cmake"
            )
        endif()
    endforeach()

endforeach()

# Expand peripheral groups
foreach(PERIPH ${HAL_PERIPHERALS})
    list(APPEND HAL_SOURCES ${HAL_${PERIPH}_SOURCES})
endforeach()

set(HAL_SRC_DIR
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/STM32L5xx_HAL_Driver/Src
)

# Prepend full paths
list(TRANSFORM HAL_SOURCES PREPEND ${HAL_SRC_DIR}/ )

set(HAL_SOURCES ${HAL_SOURCES} CACHE INTERNAL "" FORCE)
