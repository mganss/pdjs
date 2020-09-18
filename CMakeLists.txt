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

find_path(V8_INCLUDE_DIR v8.h PATH_SUFFIXES include)
find_library(V8_LIBRARY v8.dll)
find_library(V8_LIBPLATFORM_LIBRARY v8_libplatform.dll)
find_library(V8_LIBBASE_LIBRARY v8_libbase.dll)
#find_file(SNAPSHOT_BLOB snapshot_blob.bin NO_SYSTEM_ENVIRONMENT_PATH)

set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/binaries/${VCPKG_TARGET_TRIPLET}/")

add_pd_external(pdjs_project js ${PROJECT_SOURCE_DIR}/pdjs/js.cpp)

#set_property(TARGET pdjs_project APPEND_STRING PROPERTY COMPILE_FLAGS "-DV8_COMPRESS_POINTERS -DV8_31BIT_SMIS_ON_64BIT_ARCH")
target_compile_definitions(pdjs_project PRIVATE V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
target_include_directories(pdjs_project PRIVATE ${V8_INCLUDE_DIR})
target_link_libraries(pdjs_project ${V8_LIBRARY} ${V8_LIBPLATFORM_LIBRARY} ${V8_LIBBASE_LIBRARY})
#configure_file(${SNAPSHOT_BLOB} ${PROJECT_BINARY_DIR}/snapshot_blob.bin COPYONLY)
