#include "../cpp_cached/lru.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>

TEST_CASE("lru", "[cache][hide]")
{
  LRUCache c(1024);
  using vi = std::vector<int>;
  vi v(75);
  c.set("1", v);
  c.set("2", v);
  c.set("3", v);
  auto d = c.get<vi>("3");
  c.get<vi>("2");
  auto vo = std::make_shared<int>(12);
  c.set("4", vo);
  c.set("5", 5);
  auto const& i       = c.get<int>("5");
  const_cast<int&>(i) = 6;
  CHECK(c.get<int>("5") == 6);
  auto const& j       = cache_get(c, "6", []() { return 6; });
  const_cast<int&>(j) = 7;
  CHECK(c.get<int>("6") == 7);
}
