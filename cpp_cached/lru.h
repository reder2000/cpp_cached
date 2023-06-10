#pragma once

// memory cache with maximum allocated memory
// objects to be stored should provide some help
// in measuring how much memory they use


#include <unordered_map>
#include <any>
#include <list>
#include <mutex>

#include <cpp_rutils/memory_size.h>
#include <cpp_rutils/require.h>
#include <cpp_rutils/literals.h>
#include <cpp_rutils/unordered_map.h>

#include "cache_imp_exp.h"
#include "is_a_cache.h"

class cpp_cached_API LRUCache
{

 public:
  template <class T>
  using return_type = const std::decay_t<T>&;

  LRUCache(size_t maximum_memory = 1_GB);

  void resize(size_t maximum_memory);

  template <class T>
  void set(const std::string& key, const T& value, std::string_view symbol = {});

  bool has(const std::string& key);

  void erase(const std::string& key);
  void erase_symbol(std::string_view symbol);

  template <class T>
  const std::decay_t<T>& get(const std::string& key);

  // gets a value, compute it if necessary
  //template <class F>
  //const std::decay_t<std::invoke_result_t<F>>& get(const std::string& key, F callback);

  static std::shared_ptr<LRUCache> get_default();

 private:
  struct entry
  {
    entry() : _mem_used(0) {}
    template <class T>
    explicit entry(const T& value);

    std::any _value;
    size_t   _mem_used;
  };

  using map_type        = std::unordered_map<std::string, entry>;
  using map_symbol_type = hl_unordered_map<std::unordered_set<std::string>>;
  using lru_type        = std::list<std::string>;

  size_t _maximum_memory;
  size_t _current_memory;

  const entry& _get(const std::string& key);

  void reclaim(size_t memory);

  map_type        _map;
  lru_type        _list;
  map_symbol_type _map_symbol;

  std::mutex _mutex;
};

static_assert(is_a_cache<LRUCache>);

inline const LRUCache::entry& LRUCache::_get(const std::string& key)
{
  MREQUIRE(has(key), "{} does not exits", key);
  return const_cast<map_type*>(&_map)->operator[](key);
}


template <class T>
LRUCache::entry::entry(const T& value) : _value(value)
{
  _mem_used = sizeof(std::any) + memory_size(value);
}

template <class T>
void LRUCache::set(const std::string& key, const T& value, std::string_view symbol)
{
  MREQUIRE(! has(key), " {} already exists ", key);
  std::lock_guard<std::mutex> guard(_mutex);
  entry                       new_entry(value);
  reclaim(new_entry._mem_used);
  _current_memory += new_entry._mem_used;
  _map[key] = new_entry;
  auto w    = _map_symbol.find(symbol);
  if (w == _map_symbol.end())
    _map_symbol.insert({std::string(symbol), std::unordered_set<std::string>{key}});
  else
    w->second.insert(key);
  _list.push_front(key);
}

template <class T>
const std::decay_t<T>& LRUCache::get(const std::string& key)
{
  using R = typename std::decay_t<T>;
  return std::any_cast<const R&>(_get(key)._value);
}

//template <class F>
//const std::decay_t<std::invoke_result_t<F>>& LRUCache::get(const std::string& key, F callback)
//{
//  using T = std::invoke_result_t<F>;
//  if (this->has(key)) return get<T>(key);
//  set(key, callback());
//  return get<T>(key);
//}
