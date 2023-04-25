cmake_minimum_required (VERSION 3.22)

project ("cpp_cached" VERSION 0.1.0
		DESCRIPTION "simple versatile cpp memory and disk cache"
             HOMEPAGE_URL "https://github.com/reder2000/cpp_cached")

message("Configuring cpp_cached ...")

# chose at least one of the following disk cache
option(WITH_SQLITE "use sqlite" OFF)
option(WITH_POSTGRES "use postgres" OFF)
option(WITH_ROCKSDB "use rocksdb" ON)
# chose the default secondary cache
set(PREFERED_SECONDARY_CACHE "rocksdb" CACHE STRING "secondary cache")

# chose your favorite serialization framework
option(WITH_CEREAL "use cereal" ON)
# ZPP_BITS is still missing some bits 
# option(WITH_ZPP_BITS "use zpp_bits" OFF)

# our cmake facility
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

# for MREQUIRE
git_include(cpp_rutils "https://github.com/reder2000/cpp_rutils.git")

if (WITH_CEREAL)
find_vcpkg_install_missing(cereal)
list(APPEND libs cereal::cereal)
elseif (WITH_ZPP_BITS)
 find_path(ZPP_BITS_INCLUDE_DIRS "zpp_bits.h")
 if ("${ZPP_BITS_INCLUDE_DIRS}" STREQUAL "ZPP_BITS_INCLUDE_DIRS-NOTFOUND")
  vcpkg_install(zpp_bits)
  find_path(ZPP_BITS_INCLUDE_DIRS "zpp_bits.h")
 endif()
 message(FATAL_ERROR "zpp_bits does not work yet")
else()
 message(FATAL_ERROR "no serialization choosen")
endif()

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

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB srcs CONFIGURE_DEPENDS  cpp_cached/*.h cpp_cached/*.cpp)
if (NOT WITH_SQLITE)
list(FILTER srcs EXCLUDE REGEX .*sqlite.*)
endif()
if (NOT WITH_POSTGRES)
list(FILTER srcs EXCLUDE REGEX .*postgresql.*)
endif()
if (NOT WITH_ROCKSDB)
list(FILTER srcs EXCLUDE REGEX .*rocksdb.*)
endif()

add_library (cpp_cached STATIC  ${srcs} README.md)

target_link_libraries(cpp_cached PUBLIC cpp_rutils::cpp_rutils ${libs} )

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

if (${PREFERED_SECONDARY_CACHE} STREQUAL "rocksdb")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_rocksdb)
elseif(${PREFERED_SECONDARY_CACHE} STREQUAL "postgres")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_postgres)
elseif(${PREFERED_SECONDARY_CACHE} STREQUAL "sqlite")
target_compile_definitions(cpp_cached PUBLIC PREFERED_SECONDARY_CACHE_sqlite)
endif()

if (WITH_CEREAL)
target_compile_definitions(cpp_cached PUBLIC PREFERED_SERIALIZATION_cereal)
elseif (WITH_ZPP_BITS)
target_include_directories(cpp_cached PRIVATE ${ZPP_BITS_INCLUDE_DIRS})
target_compile_definitions(cpp_cached PUBLIC PREFERED_SERIALIZATION_zpp_bits)
else ()
message(FATAL_ERROR "no serialization choosen")
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
