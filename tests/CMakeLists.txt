cmake_minimum_required(VERSION 3.22)
project(cached_tests VERSION 0.1.0)

find_vcpkg_install_missing(Catch2)

file (GLOB srcs CONFIGURE_DEPENDS *.cpp *.h cached_tests.manifest)

include_directories(..)

add_executable(cached_tests ${srcs})

target_link_libraries(cached_tests Catch2::Catch2WithMain cpp_cached)

target_precompile_headers(cached_tests PUBLIC    stdafx.h)
add_definitions(-DUSING_PCH)

enable_testing()
add_test(NAME cached_main COMMAND cached_tests)

