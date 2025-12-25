include_guard(GLOBAL)

include(HalDocGen)

# Registry target must be defined by MCU layer
if(NOT TARGET ${MCU_FAMILY}_hal_registry)
    message(FATAL_ERROR
        "HAL registry target not found.\n"
        "MCU layer must define ${MCU_FAMILY}_hal_registry"
    )
endif()

generate_hal_docs(${MCU_FAMILY}_hal_registry ${CMAKE_BINARY_DIR}/docs/hal)


function(_hal_resolve OUT_LIST PERIPH)
    string(TOUPPER ${PERIPH} P)

    get_property(SRCS TARGET ${MCU_FAMILY}_hal_registry
                 PROPERTY INTERFACE_HAL_${P}_SOURCES)

    if(NOT SRCS)
        message(FATAL_ERROR
            "HAL '${P}' not provided by current MCU HAL registry"
        )
    endif()

    list(FIND ${OUT_LIST} ${P} FOUND)
    if(NOT FOUND EQUAL -1)
        return()
    endif()

    list(APPEND ${OUT_LIST} ${P})
    set(${OUT_LIST} ${${OUT_LIST}} PARENT_SCOPE)

    get_property(DEPS TARGET ${MCU_FAMILY}_hal_registry
                 PROPERTY INTERFACE_HAL_DEP_${P})

    foreach(DEP ${DEPS})
        _hal_resolve(${OUT_LIST} ${DEP})
    endforeach()
endfunction()

function(use_hal APP)
    set(_HAL_ALL "")

    foreach(P ${ARGN})
        _hal_resolve(_HAL_ALL ${P})
    endforeach()

    # Core HAL
    get_property(CORE_SRCS TARGET ${MCU_FAMILY}_hal_registry
                 PROPERTY INTERFACE_HAL_CORE_SOURCES)

    target_sources(${APP}.elf PRIVATE ${CORE_SRCS})

    # Peripheral HAL
    foreach(P ${_HAL_ALL})
        get_property(SRCS TARGET ${MCU_FAMILY}_hal_registry
                     PROPERTY INTERFACE_HAL_${P}_SOURCES)
        target_sources(${APP}.elf PRIVATE ${SRCS})
    endforeach()
endfunction()

