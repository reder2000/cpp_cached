#pragma once

// disk cache data stored in sqlite db

#include <cpp_rutils/assign_m.h>

#include "cache_imp_exp.h"
#include "sqlite.h"
#include "lru.h"

template <typename T>
concept is_a_cache = requires(T a) { a.has(std::string{}); }
&& requires(T a) { a.get<int>(std::string{}); }
&& requires(T a) { a.erase(std::string{}); }
&& requires(T a) { a.set(std::string{}, int{}); }
&& requires(T a) { a.get(std::string{}, []()->int {return 0; }); };

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
class cpp_cached_API TwoLevelCache
{

public:
	using time_point = std__chrono::utc_clock::time_point;
	TwoLevelCache(std::shared_ptr<Level1Cache> level1_cache, std::shared_ptr<Level2Cache> level2_cache);

	bool has(const std::string& key);
	template <class T>
	T get(const std::string& key);
	void erase(const std::string& key);
	template <class T>
	void set(const std::string& key, const T& value);
	// gets a value, compute it if necessary
	template <class F>
	std::invoke_result_t<F> get(const std::string& key, F callback);

private:

	std::shared_ptr<Level1Cache> _level1_cache;
	std::shared_ptr<Level2Cache> _level2_cache;

};

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
TwoLevelCache<Level1Cache, Level2Cache>::TwoLevelCache(std::shared_ptr<Level1Cache> level1_cache,
	std::shared_ptr<Level2Cache> level2_cache) : AM_(level1_cache), AM_(level2_cache)
{
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
bool TwoLevelCache<Level1Cache, Level2Cache>::has(const std::string& key)
{
	return _level1_cache->has(key) || _level2_cache->has(key);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
template <class T>
T TwoLevelCache<Level1Cache, Level2Cache>::get(const std::string& key)
{
	if (_level1_cache->has(key))
		return _level1_cache->get<T>(key);
	MREQUIRE(_level2_cache->has(key), "key {} not found", key);
	T res = _level2_cache->get<T>(key);
	_level1_cache->set(key, res);
	return res;
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
void TwoLevelCache<Level1Cache, Level2Cache>::erase(const std::string& key)
{
	_level1_cache->erase(key);
	_level2_cache->erase(key);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
template <class T>
void TwoLevelCache<Level1Cache, Level2Cache>::set(const std::string& key, const T& value)
{
	_level1_cache->set(key, value);
	_level2_cache->set(key, value);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
template <class F>
std::invoke_result_t<F> TwoLevelCache<Level1Cache, Level2Cache>::get(const std::string& key, F callback)
{
	using T = std::invoke_result_t<F>;

	if (_level1_cache->has(key))
		return _level1_cache->get<T>(key);
	if (_level2_cache->has(key)) {
		T res = _level2_cache->get<T>(key);
		_level1_cache->set(key, res);
		return res;
	}
	T res = callback();
	_level1_cache->set(key, res);
	_level2_cache->set(key, res);
	return res;
}


using MemAndSQLiteCache = TwoLevelCache<LRUCache, SqliteCache>;
