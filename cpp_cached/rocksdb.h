#pragma once

#include "cache_imp_exp.h"

#if ! defined(WITH_ROCKSDB)
#error("WITH_ROCKSDB must be defined")
#endif
#include <filesystem>

#include "serialization.h"

#include <rocksdb/db.h>
#include <cpp_rutils/literals.h>
#include <cpp_rutils/memory_size.h>

#include "time_point.h"
#include "cache_names.h"
#include "is_a_cache.h"

#define ROCKSDB_NAME CACHE_NAME_PREFIX CACHE_NAME_POSTFIX

// expire data in minutes, hours or days
enum class cpp_cached_API RocksdbCacheGranularity
{
  minutes,
  hours,
  days
};

// round time point to granularity
size_t cpp_cached_API to_duration(std__chrono::utc_clock::time_point tp,
                                 RocksdbCacheGranularity            rcg);

// auxiliary metadata
struct cpp_cached_API RocksdbValueMetaData
{
  size_t      _duration;
  std::string _type;
  size_t      _size;
  std::string _symbol;
  //template <class T>
  template <class T>
  static RocksdbValueMetaData get(const T&                t,
                                  std::string_view        symbol,
                                  RocksdbCacheGranularity rocksdb_cache_granularity);
  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(_duration, _type, _size, _symbol);
  }
};



class cpp_cached_API RocksDbCache
{
 public:
  template <class T>
  using return_type = std::decay_t<T>;

  // to be enforced
  static constexpr size_t default_max_size = 8_GB;

  static std::filesystem::path default_root_path();

  RocksDbCache(std::filesystem::path   root_path                 = default_root_path(),
               uint64_t                max_size                  = default_max_size,
               RocksdbCacheGranularity rocksdb_cache_granularity = RocksdbCacheGranularity::days);

  bool has(const std::string& key);
  void erase(const std::string& key);
  template <class T>
  std::decay_t<T> get(const std::string& key);
  template <class T>
  void set(const std::string&     key,
           const T&               value,
           std::string_view       symbol = {},
           cpp_cached::time_point d      = {});

  // gets a value, compute it if necessary
  //template <class F>
  //std::invoke_result_t<F> get(const std::string& key, F callback, cpp_cached::time_point d = {});

  [[nodiscard]] size_t total_size() const;
  void   clean_expired();
  void   clean_older(int64_t need_to_free);

  static std::shared_ptr<RocksDbCache> get_default();

  uint64_t                _max_size = 256_GB;
  RocksdbCacheGranularity _rocksdb_cache_granularity;

  void create_db();

  std::unique_ptr<rocksdb::DB> _connection;

  void really_erase(const std::string& key);

  void erase_symbol(std::string_view symbol);

  template <class T>
  std::string set_value(const T& t) const;
  template <class T>
  std::decay_t<T> get_value(const std::string& s_key) const;
};

static_assert(is_a_cache<RocksDbCache>);


template <class T>
RocksdbValueMetaData RocksdbValueMetaData::get(const T&                t,
                                               std::string_view        symbol,
                                               RocksdbCacheGranularity rocksdb_cache_granularity)
{
  size_t                             sz       = memory_size(t);
  std::string                        type     = std::string(type_name_short<T>().c);
  std__chrono::utc_clock::time_point now      = std__chrono::utc_clock::now();
  size_t                             duration = to_duration(now, rocksdb_cache_granularity);
  return RocksdbValueMetaData{duration, type, sz, std::string{symbol}};
}

template <class T>
std::string RocksDbCache::set_value(const T& t) const
{
  return cpp_cached_serialization::set_value(t);
}

template <class T>
std::decay_t<T> RocksDbCache::get_value(const std::string& value) const
{
  return cpp_cached_serialization::get_value<T>(value);
}


template <class T>
void RocksDbCache::set(const std::string& key,
                       const T&           value,
                       std::string_view   symbol,
                       cpp_cached::time_point /*d*/)
{
  MREQUIRE(! has(key));
  auto                  meta = RocksdbValueMetaData::get(value, symbol, _rocksdb_cache_granularity);
  auto                  meta_key = std::string("meta_") + key;
  rocksdb::WriteOptions woptions{};
  _connection->Put(woptions, meta_key, set_value(meta));
  _connection->Put(woptions, key, set_value(value));
}


template <class T>
std::decay_t<T> RocksDbCache::get(const std::string& key)
{
  MREQUIRE(has(key), "{} not found in DB", key);
  std::string value;
  _connection->Get(rocksdb::ReadOptions{}, key, &value);
  return get_value<T>(value);
}


//template <class F>
//std::invoke_result_t<F> RocksDbCache::get(const std::string&     key,
//                                          F                      callback,
//                                          cpp_cached::time_point d)
//{
//  using T      = std::invoke_result_t<F>;
//  bool has_key = has(key);
//  if (has_key) return get<T>(key);
//  T res = callback();
//  set(key, res, "", d);
//  return res;
//}
