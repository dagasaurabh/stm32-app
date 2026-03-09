set(FW_ROOT ${CMAKE_SOURCE_DIR})

add_library(ism330dhcx_obj OBJECT ism330dhcx.c)
target_include_directories(ism330dhcx_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap
    ${FW_ROOT}/platform/common/include
)
target_link_libraries(ism330dhcx_obj PRIVATE ${MCU_TOOLCHAIN_LIB})
set_property(TARGET ism330dhcx_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

add_library(ism330dhcx INTERFACE)
target_sources(ism330dhcx INTERFACE $<TARGET_OBJECTS:ism330dhcx_obj>)
target_include_directories(ism330dhcx INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap)
