cmake_minimum_required (VERSION 3.22)

project ("cpp_cached" VERSION 0.1.0
		DESCRIPTION "simple versatile cpp memory cache"
             HOMEPAGE_URL "https://github.com/reder2000/cpp_cached")


message("Configuring cpp_cached ...")

option(WITH_SQLITE "use sqlite" OFF)
option(WITH_POSTGRES "use postgres" ON)
option(WITH_ROCKSDB "use rocksdb" ON)
set(PREFERED_SECONDARY_CACHE "rocksdb" CACHE STRING "secondary cache")

add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELWITHDEBINFO>>:/Od>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELWITHDEBINFO>>:/Ob0>")

include(FetchContent)
FetchContent_Declare(
  rr_cmake
  GIT_REPOSITORY "https://github.com/reder2000/rr_cmake.git"
  GIT_TAG origin/master
  GIT_SHALLOW TRUE
  GIT_REMOTE_UPDATE_STRATEGY REBASE_CHECKOUT
)
FetchContent_MakeAvailable(rr_cmake)
include(${rr_cmake_SOURCE_DIR}/rr_cmake/common.cmake)

git_include(cpp_rutils "https://github.com/reder2000/cpp_rutils.git")

find_vcpkg_install_missing(cereal)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB srcs CONFIGURE_DEPENDS  cpp_cached/*.h cpp_cached/*.cpp)

if (WITH_SQLITE)
find_vcpkg_install_missing(SQLiteCpp)
list(APPEND libs SQLiteCpp)
else()
list(FILTER srcs EXCLUDE REGEX .*sqlite.*)
endif()
if (WITH_POSTGRES)
message("adding postgress")
find_vcpkg_install_missing(libpqxx)
add_compile_definitions(PQXX_HAVE_CONCEPTS)
list(APPEND libs libpqxx::pqxx)
else()
list(FILTER srcs EXCLUDE REGEX .*postgresql.*)
endif()
if (WITH_ROCKSDB)
find_vcpkg_install_missing(RocksDB rocksdb[zstd])
list(APPEND libs RocksDB::rocksdb)
endif()

add_library (cpp_cached STATIC  ${srcs})

target_link_libraries(cpp_cached PUBLIC cpp_rutils::cpp_rutils   cereal::cereal ${libs} )

if (WITH_SQLITE)
target_compile_definitions(cpp_cached PUBLIC WITH_SQLITE)
endif()
if (WITH_POSTGRES)
target_compile_definitions(cpp_cached PUBLIC PQXX_HAVE_CONCEPTS)
target_compile_definitions(cpp_cached PUBLIC WITH_POSTGRES)
endif()
if (WITH_ROCKSDB)
target_compile_definitions(cpp_cached PUBLIC WITH_ROCKSDB)
endif()

#message("PREFERED_SECONDARY_CACHE ${PREFERED_SECONDARY_CACHE}")
if (${PREFERED_SECONDARY_CACHE} STREQUAL "rocksdb")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_rocksdb)
elseif(${PREFERED_SECONDARY_CACHE} STREQUAL "postgres")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_postgres)
elseif(${PREFERED_SECONDARY_CACHE} STREQUAL "sqlite")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_sqlite)
endif()


target_precompile_headers(cpp_cached PRIVATE stdafx.h)

if ("${BUILD_SHARED_LIBS}" STREQUAL "ON")
    message("Building shared ${CMAKE_PROJECT_NAME}")
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out )
	add_compile_definitions(BUILD_SHARED_LIBS)
endif()

if (NOT "${cpp_cached_BUILD_TESTS}" STREQUAL "OFF")
    add_subdirectory(tests)
    enable_testing()
endif()

	
message("... cpp_cached configuring done")
