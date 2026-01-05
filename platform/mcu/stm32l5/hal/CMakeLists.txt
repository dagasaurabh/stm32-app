set(P hal)

set(HAL_SRC_DIR
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/STM32L5xx_HAL_Driver/Src
)

# Core HAL (always required)
add_library(${P}_core_impl OBJECT
    ${HAL_SRC_DIR}/stm32l5xx_hal.c
    ${HAL_SRC_DIR}/stm32l5xx_hal_cortex.c
)
target_link_libraries(${P}_core_impl PRIVATE  ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_core INTERFACE)
target_sources(${P}_core INTERFACE $<TARGET_OBJECTS:${P}_core_impl>)


# cog.py -crU CMakeLists.txt
# [[[cog
# import cog
# hal_modules = {
#     "gpio":    {"srcs": ["gpio"], "deps": []},
#     "rcc":     {"srcs": ["rcc", "rcc_ex"], "deps": ["pwr"]},
#     "dma":     {"srcs": ["dma"], "deps": ["rcc"]},
#     "flash":   {"srcs": ["flash", "flash_ex"], "deps": ["rcc"]},
#     "uart":    {"srcs": ["uart", "uart_ex"], "deps": ["gpio", "rcc", "dma"]},
#     "i2c":     {"srcs": ["i2c", "i2c_ex"], "deps": ["gpio", "rcc", "dma"]},
#     "pwr":     {"srcs": ["pwr", "pwr_ex"], "deps":["rcc"]}
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
#             f"${{HAL_SRC_DIR}}/stm32l5xx_hal_{src}.c)"
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
list(APPEND ${P}_GPIO_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_gpio.c)
add_library(${P}_gpio_impl OBJECT ${${P}_GPIO_SRCS})
target_link_libraries(${P}_gpio_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_gpio INTERFACE)
target_link_libraries(${P}_gpio INTERFACE ${P}_core)
target_sources(${P}_gpio INTERFACE $<TARGET_OBJECTS:${P}_gpio_impl>)

# HAL RCC Library
set(${P}_RCC_SRCS)
list(APPEND ${P}_RCC_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_rcc.c)
list(APPEND ${P}_RCC_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_rcc_ex.c)
add_library(${P}_rcc_impl OBJECT ${${P}_RCC_SRCS})
target_link_libraries(${P}_rcc_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_rcc INTERFACE)
target_link_libraries(${P}_rcc INTERFACE ${P}_core ${P}_pwr)
target_sources(${P}_rcc INTERFACE $<TARGET_OBJECTS:${P}_rcc_impl>)

# HAL DMA Library
set(${P}_DMA_SRCS)
list(APPEND ${P}_DMA_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_dma.c)
add_library(${P}_dma_impl OBJECT ${${P}_DMA_SRCS})
target_link_libraries(${P}_dma_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_dma INTERFACE)
target_link_libraries(${P}_dma INTERFACE ${P}_core ${P}_rcc)
target_sources(${P}_dma INTERFACE $<TARGET_OBJECTS:${P}_dma_impl>)

# HAL FLASH Library
set(${P}_FLASH_SRCS)
list(APPEND ${P}_FLASH_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_flash.c)
list(APPEND ${P}_FLASH_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_flash_ex.c)
add_library(${P}_flash_impl OBJECT ${${P}_FLASH_SRCS})
target_link_libraries(${P}_flash_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_flash INTERFACE)
target_link_libraries(${P}_flash INTERFACE ${P}_core ${P}_rcc)
target_sources(${P}_flash INTERFACE $<TARGET_OBJECTS:${P}_flash_impl>)

# HAL UART Library
set(${P}_UART_SRCS)
list(APPEND ${P}_UART_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_uart.c)
list(APPEND ${P}_UART_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_uart_ex.c)
add_library(${P}_uart_impl OBJECT ${${P}_UART_SRCS})
target_link_libraries(${P}_uart_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_uart INTERFACE)
target_link_libraries(${P}_uart INTERFACE ${P}_core ${P}_gpio ${P}_rcc ${P}_dma)
target_sources(${P}_uart INTERFACE $<TARGET_OBJECTS:${P}_uart_impl>)

# HAL I2C Library
set(${P}_I2C_SRCS)
list(APPEND ${P}_I2C_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_i2c.c)
list(APPEND ${P}_I2C_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_i2c_ex.c)
add_library(${P}_i2c_impl OBJECT ${${P}_I2C_SRCS})
target_link_libraries(${P}_i2c_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_i2c INTERFACE)
target_link_libraries(${P}_i2c INTERFACE ${P}_core ${P}_gpio ${P}_rcc ${P}_dma)
target_sources(${P}_i2c INTERFACE $<TARGET_OBJECTS:${P}_i2c_impl>)

# HAL PWR Library
set(${P}_PWR_SRCS)
list(APPEND ${P}_PWR_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_pwr.c)
list(APPEND ${P}_PWR_SRCS ${HAL_SRC_DIR}/stm32l5xx_hal_pwr_ex.c)
add_library(${P}_pwr_impl OBJECT ${${P}_PWR_SRCS})
target_link_libraries(${P}_pwr_impl PRIVATE ${MCU_TOOLCHAIN_LIB} ${MCU_HAL_LIB})

add_library(${P}_pwr INTERFACE)
target_link_libraries(${P}_pwr INTERFACE ${P}_core ${P}_rcc)
target_sources(${P}_pwr INTERFACE $<TARGET_OBJECTS:${P}_pwr_impl>)

# [[[end]]] (checksum: 3e5cc990c47ef99d13c050c2ee8631aa)
