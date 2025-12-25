include_guard(GLOBAL)

function(find_openocd)
    # ------------------------------------------------------------
    # Find OpenOCD executable
    # ------------------------------------------------------------
    find_program(OPENOCD_BIN
        NAMES openocd
        DOC "OpenOCD executable"
    )

    if(NOT OPENOCD_BIN)
        message(FATAL_ERROR
            "OpenOCD not found.\n"
            "Make sure it is installed into a prefix listed in CMAKE_PREFIX_PATH.\n"
            "Current CMAKE_PREFIX_PATH:\n  ${CMAKE_PREFIX_PATH}"
        )
    endif()

    # ------------------------------------------------------------
    # Derive prefix from binary path
    #   <prefix>/bin/openocd
    # ------------------------------------------------------------
    get_filename_component(_openocd_bin_dir
        "${OPENOCD_BIN}" DIRECTORY
    )

    get_filename_component(_openocd_prefix
        "${_openocd_bin_dir}" DIRECTORY
    )

    # ------------------------------------------------------------
    # Derive scripts directory
    #   <prefix>/share/openocd/scripts
    # ------------------------------------------------------------
    set(_openocd_scripts
        "${_openocd_prefix}/share/openocd/scripts"
    )

    if(NOT EXISTS "${_openocd_scripts}")
        message(FATAL_ERROR
            "OpenOCD scripts directory not found.\n"
            "Expected at:\n  ${_openocd_scripts}\n"
            "Derived from:\n  OPENOCD_BIN = ${OPENOCD_BIN}"
        )
    endif()

    set(OPENOCD_BIN
        "${OPENOCD_BIN}"
        CACHE FILEPATH "OpenOCD executable"
        FORCE
    )

    set(OPENOCD_SCRIPTS
        "${_openocd_scripts}"
        CACHE PATH "OpenOCD scripts directory"
        FORCE
    )

    message(STATUS "Found OpenOCD:")
    message(STATUS "  OPENOCD_BIN     = ${OPENOCD_BIN}")
    message(STATUS "  OPENOCD_SCRIPTS = ${OPENOCD_SCRIPTS}")
endfunction()

