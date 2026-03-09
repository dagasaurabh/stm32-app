set(FW_ROOT ${CMAKE_SOURCE_DIR})

add_library(iis2mdc_obj OBJECT iis2mdc.c)
target_include_directories(iis2mdc_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap
    ${FW_ROOT}/platform/common/include
)
target_link_libraries(iis2mdc_obj PRIVATE ${MCU_TOOLCHAIN_LIB})
set_property(TARGET iis2mdc_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

add_library(iis2mdc INTERFACE)
target_sources(iis2mdc INTERFACE $<TARGET_OBJECTS:iis2mdc_obj>)
target_include_directories(iis2mdc INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap)
