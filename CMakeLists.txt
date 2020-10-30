# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

include(pd.build/pd.cmake)

project("pdjs")

message(STATUS "Target: ${VCPKG_TARGET_TRIPLET}")

find_package(V8 CONFIG REQUIRED)

set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/binaries/${VCPKG_TARGET_TRIPLET}/")

if(${VCPKG_TARGET_TRIPLET} STREQUAL "x64-linux")
	add_compile_options(-Wall -Wextra -pedantic -Werror)
	if(${COVERAGE})
		add_compile_options(--coverage)
		add_link_options(--coverage)
	endif()
	if(${ASAN})
		add_compile_options(-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
		add_link_options(-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address)
	endif()
endif()

add_compile_definitions(VERSION=${VERSION})

add_pd_external(pdjs_project js ${PROJECT_SOURCE_DIR}/pdjs/js.cpp)

if(${VCPKG_TARGET_TRIPLET} STREQUAL "x64-windows")
add_compile_definitions(WIN32_SHARED)
elseif(${VCPKG_TARGET_TRIPLET} STREQUAL "x64-windows-static")
set_property(TARGET pdjs_project PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$CONFIG:Debug:Debug>")
elseif(${VCPKG_TARGET_TRIPLET} STREQUAL "x64-linux")
endif()

if(V8_SNAPSHOT_BLOB)
configure_file(${V8_SNAPSHOT_BLOB}> ${PD_OUTPUT_PATH}/snapshot_blob.bin COPYONLY)
endif()

configure_file(pdjs/js-help.pd ${PD_OUTPUT_PATH}/js-help.pd COPYONLY)
configure_file(pdjs/js-help.js ${PD_OUTPUT_PATH}/js-help.js COPYONLY)

target_link_libraries(pdjs_project V8::V8)
