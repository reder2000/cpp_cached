#pragma once

#include "is_a_cache.h"

#include <cpp_rutils/assign_m.h>

#include "cache_imp_exp.h"
#include "lru.h"
#include "mock.h"


template <is_a_cache Level1Cache, is_a_cache Level2Cache>
class cpp_cached_API TwoLevelCache
{

 public:
  template <class T>
  using return_type = typename Level1Cache::template return_type<T>;

  TwoLevelCache(std::shared_ptr<Level1Cache> level1_cache,
                std::shared_ptr<Level2Cache> level2_cache);

  bool has(const std::string& key);

  template <class T>
  decltype(std::declval<Level1Cache>().template get<T>("")) get(const std::string& key);

  void erase(const std::string& key);
  void erase_symbol(std::string_view symbol);
  template <class T>
  void set(const std::string& key, const T& value, std::string_view symbol = "");
  // gets a value, compute it if necessary
  //template <class F>
  //decltype(std::declval<Level1Cache>().template get<std::invoke_result_t<F>>("")) get(
  //    const std::string& key,
  //    F                  callback);

  static std::shared_ptr<TwoLevelCache> get_default();

  std::shared_ptr<Level1Cache> _level1_cache;
  std::shared_ptr<Level2Cache> _level2_cache;
};

static_assert(is_a_cache<TwoLevelCache<MockCache, MockCache>>);

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
TwoLevelCache<Level1Cache, Level2Cache>::TwoLevelCache(std::shared_ptr<Level1Cache> level1_cache,
                                                       std::shared_ptr<Level2Cache> level2_cache)
    : AM_(level1_cache), AM_(level2_cache)
{
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
bool TwoLevelCache<Level1Cache, Level2Cache>::has(const std::string& key)
{
  return _level1_cache->has(key) || _level2_cache->has(key);
}


template <is_a_cache Level1Cache, is_a_cache Level2Cache>
template <class T>
decltype(std::declval<Level1Cache>().template get<T>(""))
TwoLevelCache<Level1Cache, Level2Cache>::get(const std::string& key)
{
  if (_level1_cache->has(key)) return _level1_cache->template get<T>(key);
  MREQUIRE(_level2_cache->has(key), "key {} not found", key);
  T res = _level2_cache->template get<T>(key);
  _level1_cache->set(key, res);
  return _level1_cache->template get<T>(key);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
void TwoLevelCache<Level1Cache, Level2Cache>::erase(const std::string& key)
{
  _level1_cache->erase(key);
  _level2_cache->erase(key);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
void TwoLevelCache<Level1Cache, Level2Cache>::erase_symbol(const std::string_view symbol)
{
  _level1_cache->erase_symbol(symbol);
  _level2_cache->erase_symbol(symbol);
}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
template <class T>
void TwoLevelCache<Level1Cache, Level2Cache>::set(const std::string& key,
                                                  const T&           value,
                                                  std::string_view   symbol)
{
  _level1_cache->set(key, value, symbol);
  _level2_cache->set(key, value, symbol);
}

//template <is_a_cache Level1Cache, is_a_cache Level2Cache>
//template <class F>
//decltype(std::declval<Level1Cache>().template get<std::invoke_result_t<F>>(""))
//TwoLevelCache<Level1Cache, Level2Cache>::get(const std::string& key, F callback)
//{
//  using T = std::invoke_result_t<F>;
//
//  if (_level1_cache->has(key)) return _level1_cache->template get<T>(key);
//  if (_level2_cache->has(key))
//  {
//    T res = _level2_cache->template get<T>(key);
//    _level1_cache->set(key, res);
//    return _level1_cache->template get<T>(key);
//  }
//  T res = callback();
//  _level2_cache->set(key, res);
//  _level1_cache->set(key, res);
//  return _level1_cache->template get<T>(key);
//}

template <is_a_cache Level1Cache, is_a_cache Level2Cache>
std::shared_ptr<TwoLevelCache<Level1Cache, Level2Cache>>
TwoLevelCache<Level1Cache, Level2Cache>::get_default()
{
  static auto res =
      std::make_shared<TwoLevelCache>(Level1Cache::get_default(), Level2Cache::get_default());
  return res;
}

#if defined(PREFERRED_SECONDARY_CACHE_rocksdb)
#include "rocksdb.h"
using MemAndDbCache = TwoLevelCache<LRUCache, RocksDbCache>;
#elif defined(PREFERRED_SECONDARY_CACHE_postgres)
#include "postgresql.h"
using MemAndDbCache = TwoLevelCache<LRUCache, PostgresCache>;
#elif defined(PREFERRED_SECONDARY_CACHE_sqlite)
#include "sqlite.h"
using MemAndDbCache = TwoLevelCache<LRUCache, SQLite>;
#endif
