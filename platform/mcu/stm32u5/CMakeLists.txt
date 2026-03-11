set(P mcu_stm32u5)

add_library(${P} OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/system_stm32u5.c
    ${CMAKE_CURRENT_LIST_DIR}/src/time.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cpu_idle.c
    ${CMAKE_SOURCE_DIR}/hal/STM32CubeU5/Drivers/CMSIS/Device/ST/STM32U5xx/Source/Templates/gcc/startup_stm32u585xx.s
)

# Inherit MCU flags from platform_mcu
target_link_libraries(${P}
    PRIVATE ${MCU_TOOLCHAIN_LIB}
    PRIVATE ${MCU_HAL_LIB}
    PRIVATE mcu_syscalls)


# MCU Interface library
add_library(mcu_core INTERFACE)
target_sources(mcu_core INTERFACE
    $<TARGET_OBJECTS:mcu_syscalls>
    $<TARGET_OBJECTS:${P}>
)
target_link_libraries(mcu_core INTERFACE mcu_api)

# tell newlib: bare-metal, no OS calls
target_link_options(${MCU_TOOLCHAIN_LIB} INTERFACE
    --specs=nano.specs
    --specs=nosys.specs
    # NOTE: -u _printf_float is intentionally omitted.  newlib-nano's %f uses
    # double precision, pulling _dtoa_r + all __aeabi_d* soft-FP (~14 KB).
    # Use integer arithmetic with %ld for float output in size-sensitive builds.
)

add_subdirectory(hal)
