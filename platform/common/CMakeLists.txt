add_library(platform_common STATIC
	${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
)

target_include_directories(platform_common PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/include
)

