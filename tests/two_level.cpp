#include "../cpp_cached/two_level.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>

TEST_CASE("twolevel", "[cache][hide]")
{
	auto sql = std::make_shared<SqliteCache>();
	auto mem = std::make_shared<LRUCache>();
	MemAndSQLiteCache two(mem, sql);
	auto erase = [&two]() {
		two.erase("1");
		two.erase("2");
		two.erase("3"); };
	erase();
	sql->set("1", 1);
	CHECK(two.has("1"));
	two.set("2", 2.);
	CHECK(sql->has("2"));
	two.get("3", []() {return 3.; });
	CHECK(mem->has("3"));
	CHECK(mem->has("3"));
	CHECK(two.get<double>("2") == 2.);
	erase();
}