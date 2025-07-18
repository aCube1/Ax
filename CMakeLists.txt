cmake_minimum_required(VERSION 3.23)
project(
	ax
	VERSION 0.1.0
	LANGUAGES C
)

option(ENABLE_ASAN "Enable Address Sanitizer" ON)

include(cmake/base.cmake)
include(cmake/warnings.cmake)

add_executable(${PROJECT_NAME})

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		c_std_17
)

target_sources(
	${PROJECT_NAME}
	PRIVATE
		src/lex.c
		src/main.c
		src/utf8.c
		src/util.c
)

target_sources(
	${PROJECT_NAME}
	PRIVATE
	FILE_SET HEADERS
	TYPE HEADERS
	BASE_DIRS ${CMAKE_SOURCE_DIR}/include
	FILES
		include/lex.h
		include/types.h
		include/utf8.h
		include/util.h
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	$<$<CONFIG:Debug>:_DEBUG>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# Enable CCACHE.
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		message(STATUS "CCache found!")

		set_target_properties(
			${PROJECT_NAME}
			PROPERTIES
				CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
				CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
		)
	endif()

	# Use address sanitizer flags.
	if (ENABLE_ASAN)
		message(STATUS "Using Address Sanitizer.")

		target_compile_options(
			${PROJECT_NAME}
			PRIVATE
				$<$<COMPILE_LANGUAGE:C,CXX>:
				-fsanitize=address
				-fsanitize=leak
				-fsanitize=undefined
				-fno-omit-frame-pointer>
		)
		target_link_options(
			${PROJECT_NAME}
			PRIVATE
				$<$<COMPILE_LANGUAGE:C,CXX>:
				-fsanitize=address
				-fsanitize=leak
				-fsanitize=undefined
				-fno-omit-frame-pointer>
		)
	endif()
endif()

set_default_warnings(${PROJECT_NAME})
