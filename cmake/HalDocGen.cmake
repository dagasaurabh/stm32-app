
function(generate_hal_docs MCU_REGISTRY OUT_DIR)
    file(MAKE_DIRECTORY ${OUT_DIR})

    set(MD_FILE  ${OUT_DIR}/hal_dependencies.md)
    set(DOT_FILE ${OUT_DIR}/hal_dependencies.dot)

    file(WRITE ${MD_FILE}
        "# HAL Dependency Documentation\n\n"
        "Auto-generated from CMake HAL registry.\n\n"
        "| Peripheral | Depends On |\n"
        "|------------|------------|\n"
    )

    file(WRITE ${DOT_FILE}
        "digraph HAL {\n"
        "  rankdir=TB;\n"
        "  node [shape=box, style=rounded];\n"
        "  CORE [label=\"HAL CORE\"];\n"
    )

    get_property(PERIPHERALS TARGET ${MCU_REGISTRY}
                 PROPERTY INTERFACE_HAL_PERIPHERALS)

    if(NOT PERIPHERALS)
        message(FATAL_ERROR
            "HAL registry '${MCU_REGISTRY}' does not define "
            "INTERFACE_HAL_PERIPHERALS"
        )
    endif()

    foreach(P ${PERIPHERALS})
        get_property(DEPS TARGET ${MCU_REGISTRY}
                     PROPERTY INTERFACE_HAL_DEP_${P})

        if(DEPS)
            string(REPLACE ";" ", " DEPS_STR "${DEPS}")
        else()
            set(DEPS_STR "-")
        endif()

        file(APPEND ${MD_FILE}
            "| ${P} | ${DEPS_STR} |\n"
        )

        foreach(D ${DEPS})
            file(APPEND ${DOT_FILE}
                "  ${P} -> ${D};\n"
            )
        endforeach()
    endforeach()

    file(APPEND ${DOT_FILE} "}\n")

    message(STATUS "HAL docs generated in ${OUT_DIR}")
endfunction()

