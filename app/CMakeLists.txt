
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
    )

    #gdb client
    add_custom_target(${APP}_gdb
        COMMAND ${CMAKE_GDB}
            $<TARGET_FILE:${APP}.elf>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL
    )

endfunction(build_app)

add_subdirectory(projects/blinky)
add_subdirectory(projects/helloworld)
