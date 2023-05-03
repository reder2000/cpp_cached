#pragma once
#include <string>
#include <memory>

template <typename T>
concept is_a_cache =
    requires(T a) { a.has(std::string{}); } &&
    requires(T a) { a.template get<int>(std::string{}); } &&
    requires(T a) { a.erase(std::string{}); } &&
    requires(T a) { a.set(std::string{}, int{}, std::string_view{}); } &&
    //    requires(T a) { a.get(std::string{}, []() -> int { return 0; }); } &&
    requires(T a) { a.erase_symbol(std::string_view{}); } && requires(T) { T::get_default(); } &&
    std::is_same_v<std::shared_ptr<T>, decltype(T::get_default())>;


template <is_a_cache cache, class F>
const std::decay_t<std::invoke_result_t<F>>& cache_get(cache&             a_cache,
                                                       const std::string& key,
                                                       F                  callback)
{
  using T = std::invoke_result_t<F>;
  if (a_cache.has(key)) return a_cache.get<T>(key);
  a_cache.set(key, callback());
  return a_cache.get<T>(key);
}