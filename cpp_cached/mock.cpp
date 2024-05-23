#include "mock.h"

bool MockCache::has(std::string_view key)
{
  std__print("MockCache::has key {}\n", key);
  return true;
}

void MockCache::erase(std::string_view key)
{
  std__print("MockCache::erase key {}\n", key);
}

void MockCache::erase_symbol(std::string_view key)
{
  std__print("MockCache::erase_symbol key {}\n", key);
}

std::shared_ptr<MockCache> MockCache::get_default()
{
  static auto res = std::make_shared<MockCache>();
  return res;
}
