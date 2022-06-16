#include "../cpp_cached/lru.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>

TEST_CASE("lru", "[cache][hide]") {
	LRU_cache c(1024);
	using vi = std::vector<int>;
	vi v(75);
	c.push("1", v);
	c.push("2", v);
	c.push("3", v);
	auto d = c.get<vi>("3");
	c.get<vi>("2");
	auto vo = std::make_shared<int>(12);
	c.push("4", vo);
}
