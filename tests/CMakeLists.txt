cmake_minimum_required(VERSION 3.30)
project(cpp_cached_tests VERSION 0.1.0)

enable_testing()

find_vcpkg_install_missing(GTest gtest)
include(GoogleTest)


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

add_executable(cpp_cached_tests  ${srcs})

target_link_libraries(cpp_cached_tests  GTest::gtest GTest::gtest_main cpp_cached) 

target_precompile_headers(cpp_cached_tests  PRIVATE stdafx.h)
add_definitions(-DUSING_PCH)

gtest_add_tests(TARGET cpp_cached_tests)
