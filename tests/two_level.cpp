#include "../cpp_cached/two_level.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#if defined(PREFERED_SERIALIZATION_cereal)
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#endif

#if defined(WITH_POSTGRES)
TEST_CASE("twolevel", "[cache][hide]")
{
  auto& two = *MemAndDbCache::get_default();
  auto  mem = two._level1_cache;
  auto  sql = two._level2_cache;

  auto erase = [&two]()
  {
    two.erase("1");
    two.erase("2");
    two.erase("3");
  };
  erase();
  sql->set("1", 1);
  CHECK(two.has("1"));
  auto const& i       = two.get<int>("1");
  const_cast<int&>(i) = 2;
  CHECK(two.get<int>("1") == 2);
  two.set("2", 2., "");
  CHECK(sql->has("2"));
  auto const& j = two.get("3", []() { return 3.; });
  CHECK(mem->has("3"));
  const_cast<double&>(j) = 4.;
  CHECK(two.get<double>("3") == 4);
  CHECK(two.get<double>("2") == 2.);
  erase();
}
#endif
