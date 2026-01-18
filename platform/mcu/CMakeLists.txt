
set(SYSCALLS_TIME_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/syscalls_time.c)
set(SYSCALLS_TIME_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include)

#------------------------------------------------
add_library(mcu_syscalls OBJECT ${SYSCALLS_TIME_SRCS})

target_include_directories(mcu_syscalls PUBLIC ${SYSCALLS_TIME_INCLUDES})

target_link_libraries(mcu_syscalls
    PRIVATE ${MCU_TOOLCHAIN_LIB}
)
#------------------------------------------------

add_library(mcu_api INTERFACE)

target_include_directories(mcu_api INTERFACE
    ${SYSCALLS_TIME_INCLUDES}
)
#------------------------------------------------



add_subdirectory(${MCU_FAMILY})
