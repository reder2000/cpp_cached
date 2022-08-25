#include "../cpp_cached/sqlite.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("sqlite", "[cache][hide]") {
	SqliteCache cache{ SqliteCache::default_root_path(), 200  };
	for (int i = 0; i < 20; i++) {
		std::vector<int> toto = { i,i + 1 };
		std::string key = std::to_string(i);
		cache.set(key.c_str(), toto);
	}
	cache.get< std::vector<int>>("1");
	cache.erase("1");
	auto fun = [](){return std::vector{1,2}; };
	cache.get("1", fun);
}