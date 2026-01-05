# Guard against double inclusion
include_guard(GLOBAL)

function(add_platform_flags TARGET_NAME)
    # Create interface library
    add_library(${TARGET_NAME} INTERFACE)

    if(DEFINED MCU_DEFINES)
        target_compile_definitions(${TARGET_NAME} INTERFACE ${MCU_DEFINES})
    endif()

    if(DEFINED MCU_COMPILE_OPTIONS)
        target_compile_options(${TARGET_NAME} INTERFACE ${MCU_COMPILE_OPTIONS})
    endif()

    if(DEFINED MCU_LINK_OPTIONS)
        target_link_options(${TARGET_NAME} INTERFACE ${MCU_LINK_OPTIONS})
    endif()

    # Build-type specific flags
    target_compile_options(${TARGET_NAME} INTERFACE
        $<$<CONFIG:Debug>:-Og -g3>
        $<$<CONFIG:Release>:-Os -ffunction-sections -fdata-sections>
        $<$<CONFIG:RelWithDebInfo>:-Os -g2 -ffunction-sections -fdata-sections>
        $<$<CONFIG:MinSizeRel>:-Os -ffunction-sections -fdata-sections>
    )

    target_link_options(${TARGET_NAME} INTERFACE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,--gc-sections>
    )

endfunction()

