cmake_minimum_required(VERSION 3.16)
project(stm32_app C ASM)

if(NOT BOARD)
    message(FATAL_ERROR "BOARD not specified")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Build type: Debug, Release, RelWithDebInfo, MinSizeRel"
        FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/boards/${BOARD}/cmake"
)

# Load board configuration
include(board)

# Validate
if(NOT DEFINED MCU_FAMILY)
    message(FATAL_ERROR "Board did not define MCU_FAMILY")
endif()

include(McuIface)
include(FindOpenOCD)

set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/platform/boards/${BOARD}/openocd.cfg)

find_openocd()

# exports mcu compile,linker and build type specific flags
set(MCU_TOOLCHAIN_LIB mcu_compile_iface)

# exports HAL headers
set(MCU_HAL_LIB mcu_hal_iface)

add_mcu_compile_iface(${MCU_TOOLCHAIN_LIB})
add_mcu_hal_iface(${MCU_HAL_LIB})

option(CONFIG_BOARD_CONSOLE "Enable board console (STDIO)" ON)

# Load MCU HAL registry
add_subdirectory(platform/mcu)
add_subdirectory(platform/drivers)
add_subdirectory(platform/common)
add_subdirectory(platform/boards)
add_subdirectory(platform/devices)
add_subdirectory(app)
