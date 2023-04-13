cmake_minimum_required(VERSION 3.22)
project(cached_tests VERSION 0.1.0)

find_vcpkg_install_missing(Catch2)

file (GLOB srcs CONFIGURE_DEPENDS *.cpp *.h cached_tests.manifest)
if (NOT WITH_SQLITE)
list(FILTER srcs EXCLUDE REGEX .*sqlite.*)
endif()
if (NOT WITH_POSTGRES)
list(FILTER srcs EXCLUDE REGEX .*postgresql.*)
endif()
if (NOT WITH_ROCKSDB)
list(FILTER srcs EXCLUDE REGEX .*rocksdb.*)
endif()

#add_compile_definitions(PQXX_AUTOLINK)

include_directories(..)

add_executable(cached_tests ${srcs})

target_link_libraries(cached_tests Catch2::Catch2WithMain cpp_cached) 

target_precompile_headers(cached_tests PRIVATE stdafx.h)
add_definitions(-DUSING_PCH)

enable_testing()
add_test(NAME cached_main COMMAND cached_tests)
