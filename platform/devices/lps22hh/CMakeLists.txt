set(FW_ROOT ${CMAKE_SOURCE_DIR})

add_library(lps22hh_obj OBJECT lps22hh.c)
target_include_directories(lps22hh_obj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap
    ${FW_ROOT}/platform/common/include
)
target_link_libraries(lps22hh_obj PRIVATE ${MCU_TOOLCHAIN_LIB})
set_property(TARGET lps22hh_obj PROPERTY EXCLUDE_FROM_ALL TRUE)

add_library(lps22hh INTERFACE)
target_sources(lps22hh INTERFACE $<TARGET_OBJECTS:lps22hh_obj>)
target_include_directories(lps22hh INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${FW_ROOT}/platform/common/src/regmap)
