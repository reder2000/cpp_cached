cmake_minimum_required (VERSION 3.22)

project ("cpp_cached" VERSION 0.1.0
		DESCRIPTION "simple versatile cpp memory cache"
             HOMEPAGE_URL "https://github.com/reder2000/cpp_cached")


message("Configuring cpp_cached ...")

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
find_vcpkg_install_missing(SQLiteCpp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB srcs CONFIGURE_DEPENDS  cpp_cached/*.h cpp_cached/*.cpp)

add_library (cpp_cached STATIC  ${srcs})

target_link_libraries(cpp_cached PUBLIC cpp_rutils::cpp_rutils PRIVATE cereal::cereal SQLiteCpp)

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
