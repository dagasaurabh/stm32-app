# --------------------------------------------------
# Board: NUCLEO-L552ZE-Q
# --------------------------------------------------

set(MCU_FAMILY stm32l5)
set(MCU_VARIANT STM32L552xx)

set(MCU_CPU cortex-m33)
set(MCU_FPU fpv5-sp-d16)
set(MCU_FLOAT_ABI hard)

set(MCU_CPU_FLAGS
    -mcpu=${MCU_CPU}
    -mthumb
)

set(MCU_ABI_FLAGS
    -mfloat-abi=${MCU_FLOAT_ABI}
)

set(MCU_FPU_FLAGS
    -mfpu=${MCU_FPU}
)

set(MCU_TRUSTZONE OFF)

set(LINKER_DIR
    ${CMAKE_SOURCE_DIR}/platform/boards/nucleo_l552ze_q/linker
)

# Default linker script
set(MCU_LINKER_SCRIPT ${LINKER_DIR}/flash.ld)

# Allow RAM build for debug
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(MCU_LINKER_SCRIPT ${LINKER_DIR}/ram.ld)
#endif()

set(MCU_DEFINES
    ${MCU_VARIANT}
    USE_HAL_DRIVER
)

if(MCU_TRUSTZONE)
    list(APPEND MCU_DEFINES STM32_TRUSTZONE_ENABLED)
endif()

set(MCU_COMPILE_OPTIONS
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
    ${MCU_ABI_FLAGS}
)

set(MCU_LINK_OPTIONS
    ${MCU_COMPILE_OPTIONS}
    -T${MCU_LINKER_SCRIPT}
)

set(HAL_PERIPHERALS
    gpio
    rcc
    cortex
    uart
)
