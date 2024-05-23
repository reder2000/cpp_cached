#pragma once

// mocking cache

#include <memory>
#include <string>
#include "cache_imp_exp.h"
#include <cpp_rutils/format.h>
#include <cpp_rutils/name_short.h>

#include "is_a_cache.h"

class cpp_cached_API MockCache
{

 public:
  MockCache() = default;

  template <class T>
  using return_type = std::decay_t<T>;

  template <class T>
  void set(const std::string& key, const T& value, std::string_view = {});

  bool has(std::string_view key);

  void erase(std::string_view key);

  void erase_symbol(std::string_view key);

  template <class T>
  T get(const std::string& key);

  // gets a value, compute it if necessary
  //template <class F>
  //std::invoke_result_t<F> get(const std::string& key, F callback);

  static std::shared_ptr<MockCache> get_default();
};

static_assert(is_a_cache<MockCache>);

template <class T>
void MockCache::set(const std::string& key, const T& /*value*/, std::string_view)
{
  std__print("MockCache::set key {} value of type {}\n", key, type_name_short<T>());
}

template <class T>
T MockCache::get(const std::string& key)
{
  std__print("MockCache::get key {}\n", key);
  return T{};
}

//template <class F>
//std::invoke_result_t<F> MockCache::get(const std::string& key, F /*callback*/)
//{
//  using T = std::invoke_result_t<F>;
//  return get<T>(key);
//}
