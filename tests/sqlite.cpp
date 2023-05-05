#include "../cpp_cached/sqlite.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("sqlite", "[cache][hide]") {
	//SqliteCache cache{ SqliteCache::default_root_path(), 200  };
	//for (int i = 0; i < 20; i++) {
	//	std::vector<int> toto = { i,i + 1 };
	//	std::string key = std::to_string(i);
	//	cache.set(key, toto);
	//}
	//int ikey=10;
	//auto s_key = to_string(ikey);
	//auto res = cache.get< std::vector<int>>(s_key);
	//CHECK(res.at(0)==ikey);
	//cache.erase(s_key);
	//CHECK(!cache.has(s_key));
	//auto fun = [ikey](){return std::vector<int>{ikey,ikey+1}; };
	//res = cache.get(s_key, fun);
	//CHECK(res.at(0)==ikey);
}