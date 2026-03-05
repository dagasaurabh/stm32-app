# --------------------------------------------------
# Board: B-U585I-IOT02A (STM32U585AI Discovery Kit)
# --------------------------------------------------

set(MCU_FAMILY stm32u5 CACHE INTERNAL "")
set(MCU_VARIANT STM32U585xx CACHE INTERNAL "")

set(MCU_CPU cortex-m33)
set(MCU_FPU fpv5-sp-d16)
set(MCU_FLOAT_ABI hard)

set(MCU_CPU_FLAGS
    -mcpu=${MCU_CPU}
    -mthumb
    CACHE
    INTERNAL
    ""
)

set(MCU_ABI_FLAGS
    -mfloat-abi=${MCU_FLOAT_ABI}
    CACHE
    INTERNAL
    ""
)

set(MCU_FPU_FLAGS
    -mfpu=${MCU_FPU}
    CACHE
    INTERNAL
    ""
)

set(MCU_TRUSTZONE OFF)

set(LINKER_DIR
    ${CMAKE_SOURCE_DIR}/platform/boards/b_u585i_iot02a/linker
    CACHE
    INTERNAL
    ""
)

set(MCU_LINKER_SCRIPT ${LINKER_DIR}/flash.ld)

set(MCU_DEFINES
    ${MCU_VARIANT}
    USE_HAL_DRIVER
    CACHE
    INTERNAL
    ""
)

set(MCU_COMPILE_OPTIONS
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
    ${MCU_ABI_FLAGS}
    CACHE
    INTERNAL
    ""
)

set(MCU_LINK_OPTIONS
    ${MCU_COMPILE_OPTIONS}
    -T${MCU_LINKER_SCRIPT}
    CACHE
    INTERNAL
    ""
)

set(MCU_HAL_ROOT
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeU5
    CACHE
    INTERNAL
    ""
)

set(MCU_HAL_INCLUDES
    ${CMAKE_SOURCE_DIR}/platform/mcu/${MCU_FAMILY}/include
    ${MCU_HAL_ROOT}/Drivers/CMSIS/Include
    ${MCU_HAL_ROOT}/Drivers/CMSIS/Device/ST/STM32U5xx/Include
    ${MCU_HAL_ROOT}/Drivers/STM32U5xx_HAL_Driver/Inc
    CACHE
    INTERNAL
    ""
)
