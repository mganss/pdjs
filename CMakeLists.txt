# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.13)

set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

include(pd.build/pd.cmake)

project("pdjs")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
	set(TARGET_CPU x64)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
	set(TARGET_CPU arm64)
else()
	set(TARGET_CPU ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(TARGET_OS macos)
else()
	string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_OS)
endif()

set(TARGET_TRIPLET ${TARGET_CPU}-${TARGET_OS})

message(STATUS "Target: ${TARGET_TRIPLET}")

set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/binaries/${TARGET_TRIPLET}/")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-cast-function-type -std=gnu++14)
	add_link_options(-static-libstdc++)
	if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
		add_link_options(-s)
	endif()
	if(${COVERAGE})
		add_compile_options(--coverage)
		add_link_options(--coverage)
	endif()
	if(${ASAN})
		add_compile_options(-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
		add_link_options(-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter -std=c++14)
elseif(MSVC)
	add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
	add_compile_options(/MT)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_compile_definitions("V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH")
endif()

add_compile_definitions(VERSION=${VERSION})

add_pd_external(pdjs_project js ${PROJECT_SOURCE_DIR}/pdjs/js.cpp)

configure_file(pdjs/js-help.pd ${PD_OUTPUT_PATH}/js-help.pd COPYONLY)
configure_file(pdjs/js-help.js ${PD_OUTPUT_PATH}/js-help.js COPYONLY)

target_include_directories(pdjs_project PRIVATE v8/include)
target_link_directories(pdjs_project PRIVATE v8/lib/${TARGET_TRIPLET})
target_link_libraries(pdjs_project v8_monolith)

if(MSVC)
	target_link_libraries(pdjs_project winmm dbghelp)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND ${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_custom_command(TARGET pdjs_project POST_BUILD COMMAND strip -x $<TARGET_FILE:pdjs_project>)
endif()