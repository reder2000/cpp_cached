#include "../cpp_cached/mock.h"

#define CATCH_CONFIG_ALL_PARTS
#include <gtest/gtest.h>

TEST(cpp_cached_tests,mock) {
	MockCache c;
	c.set("1", 1);
}
