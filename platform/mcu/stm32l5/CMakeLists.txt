set(P mcu_stm32l5)

add_library(${P} OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/system_stm32l5.c
    ${CMAKE_CURRENT_LIST_DIR}/startup/startup_stm32l552xx.s
)

# Inherit MCU flags from platform_mcu
target_link_libraries(${P}
    PRIVATE ${MCU_TOOLCHAIN_LIB} 
    PRIVATE ${MCU_HAL_LIB})

# MCU Interface library
add_library(mcu_core INTERFACE)
target_sources(mcu_core INTERFACE $<TARGET_OBJECTS:${P}>)

#tell newlic don't expect OS calls, its bare-metal code
target_link_options(${MCU_TOOLCHAIN_LIB} INTERFACE
    --specs=nano.specs
    --specs=nosys.specs
)

add_subdirectory(hal)
