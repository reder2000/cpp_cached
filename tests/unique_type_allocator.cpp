#include "../cpp_cached/unique_type_allocator.h"

#include <gtest/gtest.h>

TEST(cpp_cached_tests,type_allocator) {
    [[maybe_unused]] int* p_int = unique_type_new<int>(2);
    int* p_int_3 = unique_type_new<int>(20);
    EXPECT_TRUE(*p_int_3 == 2);
}