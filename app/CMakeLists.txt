get_filename_component(APP_DIR ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)

function(build_app APP)

    set(MAP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${APP}.map)

    target_link_options(${APP}.elf PRIVATE
        -Wl,-Map=${MAP_FILE}
    )

    add_custom_command(TARGET ${APP}.elf POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${APP}.elf>
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${APP}.elf> ${APP}.bin
    )

   # flash
    add_custom_target(${APP}_flash
        COMMAND ${OPENOCD_BIN}
	    -s ${OPENOCD_SCRIPTS}
            -f ${OPENOCD_CFG}
            -c "program $<TARGET_FILE:${APP}.elf> verify reset exit"
        DEPENDS ${APP}.elf
        USES_TERMINAL
    )
    
    # debug server
    add_custom_target(${APP}_debug
        COMMAND ${OPENOCD_BIN}
	    -s ${OPENOCD_SCRIPTS}
	    -f ${OPENOCD_CFG}
	USES_TERMINAL
	COMMENT "Starting OpenOCD GDB server"
    )

    #gdb client
    add_custom_target(${APP}_gdb
        COMMAND ${CMAKE_GDB}
	    $<TARGET_FILE:${APP}.elf>
	    -x ${APP_DIR}/scripts/gdb_startup.gdb
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	USES_TERMINAL
	COMMENT "Launching GDB with default startup commands"
    )

endfunction(build_app)

add_subdirectory(projects/blinky)
add_subdirectory(projects/helloworld)
add_subdirectory(projects/button_test)
