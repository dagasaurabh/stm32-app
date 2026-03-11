set(P hal)

set(HAL_SRC_DIR
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeU5/Drivers/STM32U5xx_HAL_Driver/Src
)

# Core HAL (always required)
add_library(${P}_core_impl OBJECT
    ${HAL_SRC_DIR}/stm32u5xx_hal.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_pwr.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_pwr_ex.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_rcc.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_rcc_ex.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_cortex.c
    ${HAL_SRC_DIR}/stm32u5xx_hal_icache.c
)
target_link_libraries(${P}_core_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_core INTERFACE)
target_sources(${P}_core INTERFACE $<TARGET_OBJECTS:${P}_core_impl>)

# cog.py -crU CMakeLists.txt
# [[[cog
# import cog
# hal_modules = {
#     "gpio":    {"srcs": ["gpio", "exti"], "deps": []},
#     "dma":     {"srcs": ["dma", "dma_ex"], "deps": []},
#     "flash":   {"srcs": ["flash", "flash_ex"], "deps": []},
#     "uart":    {"srcs": ["uart", "uart_ex"], "deps": ["gpio", "dma"]},
#     "i2c":     {"srcs": ["i2c", "i2c_ex"], "deps": ["gpio", "dma"]},
#     "spi":     {"srcs": ["spi", "spi_ex"], "deps": ["gpio", "dma"]}
#  }
# cog.outl(f"")
# DEFAULT_DEPS = ["core"]
# for module, cfg in hal_modules.items():
#     srcs = cfg["srcs"]
#     deps = cfg["deps"]
#
#     cog.outl(f"# HAL {module.upper()} Library")
#     cog.outl(f"set(${{P}}_{module.upper()}_SRCS)")
#
#     for src in srcs:
#        cog.outl(f"list(APPEND ${{P}}_{module.upper()}_SRCS "
#             f"${{HAL_SRC_DIR}}/stm32u5xx_hal_{src}.c)"
#         )
#
#     cog.outl(f"add_library(${{P}}_{module}_impl OBJECT ${{${{P}}_{module.upper()}_SRCS}})")
#     cog.outl(f"target_link_libraries(${{P}}_{module}_impl PRIVATE ${{MCU_TOOLCHAIN_LIB}} ${{MCU_HAL_LIB}})")
#     cog.outl(f"")
#
#     cog.outl(f"add_library(${{P}}_{module} INTERFACE)")
#     effective_deps = []
#     if not deps:
#         effective_deps = DEFAULT_DEPS
#     else:
#         effective_deps = DEFAULT_DEPS + deps
#     
#     dep_targets = " ".join([f"${{P}}_{d}" for d in effective_deps])
#     cog.outl(f"target_link_libraries(${{P}}_{module} INTERFACE {dep_targets})")
#     cog.outl(f"target_sources(${{P}}_{module} INTERFACE $<TARGET_OBJECTS:${{P}}_{module}_impl>)")
#     cog.outl(f"")
#
# ]]]

# HAL GPIO Library
set(${P}_GPIO_SRCS)
list(APPEND ${P}_GPIO_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_gpio.c)
list(APPEND ${P}_GPIO_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_exti.c)
add_library(${P}_gpio_impl OBJECT ${${P}_GPIO_SRCS})
target_link_libraries(${P}_gpio_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_gpio INTERFACE)
target_link_libraries(${P}_gpio INTERFACE ${P}_core)
target_sources(${P}_gpio INTERFACE $<TARGET_OBJECTS:${P}_gpio_impl>)

# HAL DMA Library
set(${P}_DMA_SRCS)
list(APPEND ${P}_DMA_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_dma.c)
list(APPEND ${P}_DMA_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_dma_ex.c)
add_library(${P}_dma_impl OBJECT ${${P}_DMA_SRCS})
target_link_libraries(${P}_dma_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_dma INTERFACE)
target_link_libraries(${P}_dma INTERFACE ${P}_core)
target_sources(${P}_dma INTERFACE $<TARGET_OBJECTS:${P}_dma_impl>)

# HAL FLASH Library
set(${P}_FLASH_SRCS)
list(APPEND ${P}_FLASH_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_flash.c)
list(APPEND ${P}_FLASH_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_flash_ex.c)
add_library(${P}_flash_impl OBJECT ${${P}_FLASH_SRCS})
target_link_libraries(${P}_flash_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_flash INTERFACE)
target_link_libraries(${P}_flash INTERFACE ${P}_core)
target_sources(${P}_flash INTERFACE $<TARGET_OBJECTS:${P}_flash_impl>)

# HAL UART Library
set(${P}_UART_SRCS)
list(APPEND ${P}_UART_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_uart.c)
list(APPEND ${P}_UART_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_uart_ex.c)
add_library(${P}_uart_impl OBJECT ${${P}_UART_SRCS})
target_link_libraries(${P}_uart_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_uart INTERFACE)
target_link_libraries(${P}_uart INTERFACE ${P}_core ${P}_gpio ${P}_dma)
target_sources(${P}_uart INTERFACE $<TARGET_OBJECTS:${P}_uart_impl>)

# HAL I2C Library
set(${P}_I2C_SRCS)
list(APPEND ${P}_I2C_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_i2c.c)
list(APPEND ${P}_I2C_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_i2c_ex.c)
add_library(${P}_i2c_impl OBJECT ${${P}_I2C_SRCS})
target_link_libraries(${P}_i2c_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_i2c INTERFACE)
target_link_libraries(${P}_i2c INTERFACE ${P}_core ${P}_gpio ${P}_dma)
target_sources(${P}_i2c INTERFACE $<TARGET_OBJECTS:${P}_i2c_impl>)

# HAL SPI Library
set(${P}_SPI_SRCS)
list(APPEND ${P}_SPI_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_spi.c)
list(APPEND ${P}_SPI_SRCS ${HAL_SRC_DIR}/stm32u5xx_hal_spi_ex.c)
add_library(${P}_spi_impl OBJECT ${${P}_SPI_SRCS})
target_link_libraries(${P}_spi_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_spi INTERFACE)
target_link_libraries(${P}_spi INTERFACE ${P}_core ${P}_gpio ${P}_dma)
target_sources(${P}_spi INTERFACE $<TARGET_OBJECTS:${P}_spi_impl>)

# [[[end]]] (checksum: 9f73a99633925dc7ddca4833825cae85)
