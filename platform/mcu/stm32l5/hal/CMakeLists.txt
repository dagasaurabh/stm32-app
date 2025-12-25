
add_library(stm32l5_hal_registry INTERFACE)

set(HAL_SRC_DIR
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/STM32L5xx_HAL_Driver/Src
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_PERIPHERALS
    GPIO RCC UART I2C DMA ADC FLASH PWR
)

# -------------------------------
# Core HAL (always required)
# -------------------------------
set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_CORE_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_cortex.c
)

# -------------------------------
# Peripheral HAL sources
# -------------------------------
set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_GPIO_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_gpio.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_RCC_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_rcc.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_rcc_ex.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_UART_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_uart.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_uart_ex.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_I2C_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_i2c.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_i2c_ex.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DMA_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_dma.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_FLASH_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_flash.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_flash_ex.c
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_PWR_SOURCES
    ${HAL_SRC_DIR}/stm32l5xx_hal_pwr.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_pwr_ex.c
)

# -------------------------------
# Dependency rules
# -------------------------------
set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_UART GPIO RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_I2C GPIO RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_SPI GPIO RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_ADC RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_DAC RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_DMA RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_FLASH RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_PWR RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_GPIO RCC
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_RCC ""
)

set_property(TARGET stm32l5_hal_registry PROPERTY
    INTERFACE_HAL_DEP_CORTEX ""
)

