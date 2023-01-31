#include "../cpp_cached/mock.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>

TEST_CASE("mock", "[cache][hide]") {
	MockCache c;
	c.set("1", 1);
}
