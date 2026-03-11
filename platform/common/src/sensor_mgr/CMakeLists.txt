set(FW_ROOT ${CMAKE_SOURCE_DIR})

add_library(sensor_mgr_obj OBJECT sensor_mgr.c)
target_include_directories(sensor_mgr_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/time/include
    ${FW_ROOT}/platform/common/include
)
target_link_libraries(sensor_mgr_obj PRIVATE
    ${MCU_TOOLCHAIN_LIB}
    board_common
)
set_property(TARGET sensor_mgr_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

add_library(sensor_mgr INTERFACE)
target_sources(sensor_mgr INTERFACE $<TARGET_OBJECTS:sensor_mgr_obj>)
target_include_directories(sensor_mgr INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/include)
