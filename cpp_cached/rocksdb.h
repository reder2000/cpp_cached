#pragma once

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

#define ROCKSDB_NAME CACHE_NAME_PREFIX CACHE_NAME_POSTFIX

enum class RocksdbCacheGranularity
{
  minutes,
  hours,
  days
};

size_t to_duration(std__chrono::utc_clock::time_point tp, RocksdbCacheGranularity rcg);

struct RocksdbValueMetaData
{
  size_t      _duration;
  std::string _type;
  size_t      _size;
  std::string _symbol;
  //template <class T>
  //static RocksdbValueMetaData set(const T& t, RocksdbCacheGranularity rocksdb_cache_granularity);
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



class RocksDbCache
{

 public:
  //using time_point = std__chrono::utc_clock::time_point;

  static std::filesystem::path default_root_path()
  {
    return std::filesystem::temp_directory_path().append("cache");
  }
  static constexpr size_t default_max_size = 8_GB;

  RocksDbCache(std::filesystem::path   root_path                 = default_root_path(),
               uint64_t                max_size                  = default_max_size,
               RocksdbCacheGranularity rocksdb_cache_granularity = RocksdbCacheGranularity::days);

  bool has(const std::string& key);
  void erase(const std::string& key);
  template <class T>
  T get(const std::string& key);
  template <class T>
  void set(const std::string&     key,
           const T&               value,
           std::string_view       symbol = {},
           cpp_cached::time_point d      = {});

  // gets a value, compute it if necessary
  template <class F>
  std::invoke_result_t<F> get(const std::string& key, F callback, cpp_cached::time_point d = {});

  size_t total_size() const;
  void   clean_expired();
  void   clean_older(int64_t need_to_free);

  static std::shared_ptr<RocksDbCache> get_default();

  //private:
  //std::filesystem::path _root_path, _db_name;
  uint64_t                _max_size = 256_GB;
  RocksdbCacheGranularity _rocksdb_cache_granularity;

  void create_db();

  std::unique_ptr<rocksdb::DB> _connection;
  //SQLite::Database get_db(bool can_write);
  //bool             is_expired(const std::string& key);
  void really_erase(const std::string& key);

  void erase_symbol(const std::string_view symbol);

  template <class T>
  std::string set_value(const T& t) const;
  template <class T>
  T get_value(const std::string& t) const;
};


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
T RocksDbCache::get_value(const std::string& value) const
{
  //T                          res;
  //imemstream                 sin(value.data(),
  //                               value.size());  //reinterpret_cast<const char*>(col_blob.data()), col_blob.size());
  //cereal::BinaryInputArchive archive(sin);
  //archive(res);
  //return res;
  return cpp_cached_serialization::get_value<T>(value);
}


template <class T>
void RocksDbCache::set(const std::string&     key,
                       const T&               value,
                       std::string_view       symbol,
                       cpp_cached::time_point d)
{
  MREQUIRE(! has(key));
  auto                  meta = RocksdbValueMetaData::get(value, symbol, _rocksdb_cache_granularity);
  auto                  meta_key = std::string("meta_") + key;
  rocksdb::WriteOptions woptions{};
  _connection->Put(woptions, meta_key, set_value(meta));
  _connection->Put(woptions, key, set_value(value));
  ////using namespace SQLite;
  ////cache_row_value values{};
  //values.store_time = std__chrono::utc_clock::now().time_since_epoch().count();
  //if (d == cpp_cached::time_point())  // standard expiration is today at midnight
  //{
  //  d = cpp_cached::last_point_of_today();
  //}
  //values.expire_time = d.time_since_epoch().count();
  //values.access_time = values.store_time;
  //////auto                        db = get_db(true);
  //////Statement                   query(db,
  //////	"INSERT OR REPLACE INTO "
  //////	"cache(key,store_time,expire_time,access_time,access_count,size,value) "
  //////	"VALUES(?,?,?,?,?,?,?)");
  //std::ostringstream          out;
  //cereal::BinaryOutputArchive archive(out);
  ////cereal::JSONOutputArchive archive(out);
  //archive(value);
  //size_t sz = out.str().size();
  //size_t ts = total_size();
  //if ((ts + sz) > _max_size)  // reclaim expired
  //  clean_expired();
  //ts = total_size();
  //if ((ts + sz) > _max_size)
  //{  // reclaim old
  //  size_t need_to_free = (ts + sz) - _max_size;
  //  clean_older(need_to_free);
  //}
  //std::basic_string<std::byte> blob(reinterpret_cast<const std::byte*>(out.str().c_str()),
  //                                  out.str().size());
  //pqxx::work                   work(_connection);
  //work.exec_prepared0("set", key, std::string{symbol}, values.store_time, values.expire_time,
  //                    values.access_time, values.accesss_count, sz, blob);
  ////int i_sz = static_cast<int>(sz);
  ////bind(query, key.c_str(), values.store_time, values.expire_time, values.access_time,
  ////	values.accesss_count, i_sz);
  ////query.bind(7, reinterpret_cast<const void*>(out.str().c_str()), i_sz);
  ////query.exec();
  //work.commit();
}


template <class T>
T RocksDbCache::get(const std::string& key)
{
  MREQUIRE(has(key), "{} not found in DB", key);
  std::string value;
  _connection->Get(rocksdb::ReadOptions{}, key, &value);
  return get_value<T>(value);
}

//  //using namespace SQLite;
//  using duration = std__chrono::utc_clock::duration;
//  T           res;
//  std::string where = "get_db";
//  try
//  {
//    auto key = skey.c_str();
//    MREQUIRE(has(skey), "{} not found in DB", skey);
//    {
//      // get blob
//      where = "blob get_db";
//      //	auto db = get_db(true);
//      where = "blob query";
//      where = "blob bind";
//      //query.bind(1, key);
//      //where = "blob executeStep";
//      //auto success = executeStep_retry(db, query);
//      pqxx::work work(_connection);
//      auto       r = work.exec_prepared("get", key);
//      work.commit();
//      auto [col_blob] = r[0].as<std::basic_string<std::byte>>();
//      //if (! success) return res;
//      //MREQUIRE(success, "getting blob failed for {}", skey);
//      //where = "blob getColumn";
//      //auto col_blob = query.getColumn(6);
//      //where = "blob getblob";
//      imemstream sin(reinterpret_cast<const char*>(col_blob.data()), col_blob.size());
//      cereal::BinaryInputArchive archive(sin);
//      //cereal::JSONInputArchive archive(sin);
//      //T                          tmp;
//      where = "blob archive";
//      archive(res);
//      //res = tmp;
//    }
//    {
//      // update counters
//      where = "get2";
//      //auto db = get_db(true);
//      //where = "query2";
//      pqxx::work work(_connection);
//      work.exec_params0("UPDATE " pgCache " SET access_count = access_count + 1 WHERE key=$1", key);
//      //where = "bind2";
//      //query2.bind(1, key);
//      //where = "exec_retry2";
//      //exec_retry(db, query2);
//      ////query2.exec();
//      //where = "query2";
//      std__chrono::utc_clock::time_point now  = std__chrono::utc_clock::now();
//      duration                           dnow = now.time_since_epoch();
//      work.exec_params0("UPDATE " pgCache " SET access_time = $1 WHERE key=$2", dnow.count(), key);
//      //where = "bind3";
//      //bind(query3, dnow.count(), key);
//      //where = "exec_retry3";
//      //exec_retry(db, query3);
//      work.commit();
//    }
//  }
//  catch (std::exception& e)
//  {
//    //fmt::print("failed reading blob : {} \n", where);
//    throw std::runtime_error(fmt::format("failed reading blob at {} since {}\n", where, e.what()));
//  }
//  return res;
//}
//


////
template <class F>
std::invoke_result_t<F> RocksDbCache::get(const std::string&     key,
                                          F                      callback,
                                          cpp_cached::time_point d)
{
  using T      = std::invoke_result_t<F>;
  bool has_key = has(key);
  if (has_key) return get<T>(key);
  T res = callback();
  set(key, res, "", d);
  return res;
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
//  set(key, res, d);
//  return res;
//}
