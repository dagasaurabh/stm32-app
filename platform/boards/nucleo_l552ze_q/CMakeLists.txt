add_library(board OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/src/board.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hal_msp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/stm32l5xx_it.c
)

target_include_directories(board PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_definitions(board PUBLIC BOARD_HAS_LED BOARD_HAS_BUTTON)

target_link_libraries(board
    mcu_stm32l5
    stm32l5_hal
    platform_common
)

