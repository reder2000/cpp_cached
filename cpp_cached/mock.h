#pragma once 

// mocking cache

#include <memory>
#include <string>
#include "cache_imp_exp.h"
#include <fmt/format.h>
#include <cpp_rutils/name_short.h>

class cpp_cached_API MockCache {

public:

	MockCache() = default;

	template <class T>
	void set(const std::string& key, const T& value);

	bool has(const std::string& key);

	void erase(const std::string& key);

	template <class T>
	T get(const std::string& key);

	// gets a value, compute it if necessary
	template <class F>
	std::invoke_result_t<F> get(const std::string& key, F callback);

	static std::shared_ptr<MockCache> get_default();

};


template<class T>
void MockCache::set(const std::string& key, const T& value)
{
	fmt::print("MockCache::set key {} value of type {}\n", key, type_name_short<T>());
}

template<class T>
T MockCache::get(const std::string& key)
{
	fmt::print("MockCache::get key {}\n", key);
	return T{};
}

template <class F>
std::invoke_result_t<F> MockCache::get(const std::string& key, F callback)
{
	using T = std::invoke_result_t<F>;
	return get<T>(key);
}
