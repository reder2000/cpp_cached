#include "../cpp_cached/disk.h"

#define CATCH_CONFIG_ALL_PARTS
#include <gtest/gtest.h>
#if defined(PREFERED_SERIALIZATION_cereal)
#include <cereal/types/vector.hpp>
#endif

#if 0
TEST(cpp_cached_tests,disk)
{
  DiskCache c;
  using vi = std::vector<int>;
  vi v(75);
  for (size_t i = 0; i < 75; i++)
    v[i] = i;
  c.push("1", v);
  c.push("2", v);
  std::string big_name;
  for (size_t i = 0; i < 750; i++)
    big_name.push_back("abcde01234"[i % 10]);
  c.push(big_name, v);
  auto d = c.get<vi>("2");
  c.get<vi>("1");
  c.get<vi>(big_name);
}

TEST(cpp_cached_tests,disk2", "[cache][.hide])
{
  DiskCache c;
  using vi = std::vector<int>;
  vi v(75);
  c.push("1", v);
  c.push("2", v);
  c.push("3", v);
  auto d = c.get<vi>("3");
  c.get<vi>("2");
}

#endif