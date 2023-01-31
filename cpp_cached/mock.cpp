#include "mock.h"

bool MockCache::has(const std::string& key) {
	fmt::print("MockCache::has key {}\n", key);
	return true;
}

void MockCache::erase(const std::string& key) {
	fmt::print("MockCache::erase key {}\n", key);
}

std::shared_ptr<MockCache> MockCache::get_default()
{
	static auto res = std::make_shared<MockCache>();
	return res;
}


