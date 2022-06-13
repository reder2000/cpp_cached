#include "../sqlite.cpp"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("sqlite", "[cache][hide]") {
	SqliteCache cache{};
	cache.get_db();
	//auto start = std::chrono::utc_clock::now();

	int value = 2;
	cache.set("toto", value);

	auto res = cache.get<int>("toto");
}