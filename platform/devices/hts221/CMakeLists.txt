set(FW_ROOT ${CMAKE_SOURCE_DIR})

add_library(hts221_obj OBJECT hts221.c)
target_include_directories(hts221_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap/include
    ${FW_ROOT}/platform/common/include
)
target_link_libraries(hts221_obj PRIVATE ${MCU_TOOLCHAIN_LIB})
set_property(TARGET hts221_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

add_library(hts221 INTERFACE)
target_sources(hts221 INTERFACE $<TARGET_OBJECTS:hts221_obj>)
target_include_directories(hts221 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap/include
    ${FW_ROOT}/platform/common/include)
