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
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/boards/${BOARD}/cmake"
)


# Load board configuration
include(board)

# Validate
if(NOT DEFINED MCU_FAMILY)
    message(FATAL_ERROR "Board did not define MCU_FAMILY")
endif()

include(platform_flags)
include(FindOpenOCD)

set(OPENOCD_CFG
    ${CMAKE_SOURCE_DIR}/platform/boards/${BOARD}/openocd.cfg
)

find_openocd()
add_platform_flags(platform_mcu)

# Load MCU HAL registry
add_subdirectory(platform/mcu/${MCU_FAMILY})
include(HalSelection)

# Now build platform layers
add_subdirectory(platform/common)
add_subdirectory(platform/boards/${BOARD})
add_subdirectory(app)
