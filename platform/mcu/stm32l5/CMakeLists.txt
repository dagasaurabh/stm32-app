
add_subdirectory(hal)

include(stm32_hal)

add_library(mcu_stm32l5 OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/system_stm32l5.c
    ${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32l552xx.s
)

target_include_directories(mcu_stm32l5 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/CMSIS/Device/ST/STM32L5xx/Include
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeL5/Drivers/STM32L5xx_HAL_Driver/Inc
)

#tell newlic don't expect OS calls, its bare-metal code
target_link_options(platform_mcu INTERFACE
    --specs=nano.specs
    --specs=nosys.specs
)

# Inherit MCU flags from platform_mcu
target_link_libraries(mcu_stm32l5 PUBLIC platform_mcu)

add_stm32_hal(l5 ${HAL_SOURCES})
