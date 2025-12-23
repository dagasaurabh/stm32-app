function(add_stm32_hal FAMILY)

    string(TOUPPER ${FAMILY} FAMILY_UP)

    set(HAL_DIR
        ${CMAKE_SOURCE_DIR}/hal/STM32Cube${FAMILY_UP}/Drivers
    )

    message(" -------- HAl_SRCS = ${ARGN}")
    add_library(stm32${FAMILY}_hal STATIC
	    ${ARGN}
    )

    target_include_directories(stm32${FAMILY}_hal PUBLIC
        ${HAL_DIR}/STM32${FAMILY_UP}xx_HAL_Driver/Inc
    )

    target_link_libraries(stm32${FAMILY}_hal PUBLIC
        mcu_stm32${FAMILY}
        platform_mcu
    )


    set_property(TARGET stm32${FAMILY}_hal PROPERTY HAL_FAMILY ${FAMILY})

endfunction()

