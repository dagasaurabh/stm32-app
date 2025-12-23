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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/boards/${BOARD}/cmake"
)


# Load board configuration
include(board)
include(platform_flags)
include(FindOpenOCD)

set(OPENOCD_CFG
    ${CMAKE_SOURCE_DIR}/platform/boards/${BOARD}/openocd.cfg
)

find_openocd()

add_platform_flags(platform_mcu)


# Now build platform layers
add_subdirectory(platform/common)
add_subdirectory(platform/mcu/${MCU_FAMILY})
add_subdirectory(platform/boards/${BOARD})
add_subdirectory(app)
