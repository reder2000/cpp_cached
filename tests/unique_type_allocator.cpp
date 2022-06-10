#include "../unique_type_allocator.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("type allocator") {
    int* p_int = unique_type_new<int>(2);
    int* p_int_3 = unique_type_new<int>(20);
    REQUIRE(*p_int_3 == 2);
}