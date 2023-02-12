#include "../cpp_cached/sqlite_single.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("sqlite_single", "[cache][hide]") {
	SqliteSingleCache cache{ SqliteSingleCache::default_root_path(), 200  };
	for (int i = 0; i < 20; i++) {
		std::vector<int> toto = { i,i + 1 };
		std::string key = std::to_string(i);
		cache.set(key, toto);
	}
	int ikey=10;
	auto skey = to_string(ikey);
	auto res = cache.get< std::vector<int>>(skey);
	CHECK(res.at(0)==ikey);
	cache.erase(skey);
	CHECK(!cache.has(skey));
	auto fun = [ikey](){return std::vector<int>{ikey,ikey+1}; };
	res = cache.get(skey, fun);
	CHECK(res.at(0)==ikey);
}