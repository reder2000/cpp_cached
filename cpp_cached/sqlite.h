#pragma once

// disk cache data stored in sqlite db

#include "cache_imp_exp.h"

#include <filesystem>
#include <optional>
#include <cpp_rutils/literals.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <cpp_rutils/require.h>
#include <cpp_rutils/memstream.h>
#include <cpp_rutils/date.h>
#include <cereal/archives/binary.hpp>

// sqlite cache parameters:
// - size: size of the cache in bytes
// - path: path to the cache directory
// - data max age : either in seconds or days (triggers at midnight in a given timezone)

class cpp_cached_API SqliteCache
{

  public:
  using time_point = std__chrono::utc_clock::time_point ;
  //using duration                            = std__chrono::utc_clock::duration;
  //static constexpr duration    max_duration = duration(std::numeric_limits<int64_t>::max());
  static std::filesystem::path default_root_path() { return std::filesystem::temp_directory_path().append("cache").append("sqlite"); }
  static constexpr size_t      default_max_size = 2_GB;

  SqliteCache(const std::filesystem::path& root_path = default_root_path(), uint64_t max_size = default_max_size);

  SQLite::Database get_db(bool create_tables = false);

  bool has(const std::string& key);

  void erase(const std::string& key);

  template <class T>
  void set(const std::string& key, const T& value, time_point d = time_point());

  template <class T>
  std::optional<T> get(const std::string& key);

  private:
  std::filesystem::path _root_path, _db_name;
  uint64_t              _max_size;

  size_t total_size(SQLite::Database& db) const;
  void   clean_expired(SQLite::Database& db);
  void   clean_older(SQLite::Database& db, int64_t need_to_free);
};

// today 23:59:59
SqliteCache::time_point last_point_of_today();

// struct for extracting all fields
struct cache_row_value
{
  std::string key;
  int64_t     store_time;
  int64_t     expire_time;
  int64_t     access_time;
  int64_t     accesss_count;
  int64_t     size;
};

template <class T>
inline std::optional<T> SqliteCache::get(const std::string& skey)
{
  using namespace SQLite;
  std::optional<T> res;
  auto             db  = get_db();
  auto             key = skey.c_str();
  Statement        query(db, "SELECT * FROM CACHE where key = ?");
  query.bind(1, key);
  auto success = query.executeStep();
  if (! success) return res;
  auto                               values      = query.getColumns<cache_row_value, 6>();
  using duration                            = std__chrono::utc_clock::duration;
  time_point expire_time                          = time_point (duration(values.expire_time));
  std__chrono::utc_clock::time_point now         = std__chrono::utc_clock::now();
  // remove row if expired
  if (now > expire_time)
  {
    Statement query2(db, "DELETE FROM cache WHERE key = ?");
    query2.bind(1, key);
    query2.exec();
    return res;
  }
  // get blob
  auto                       col_blob = query.getColumn(6);
  imemstream                 sin(reinterpret_cast<const char*>(col_blob.getBlob()), col_blob.size());
  cereal::BinaryInputArchive archive(sin);
  T                          tmp;
  archive(tmp);
  res = tmp;
  // update counters
  Statement query2(db, "UPDATE cache SET access_count = access_count + 1 WHERE key=?");
  query2.bind(1, key);
  query2.exec();
  Statement query3(db, "UPDATE cache SET access_time = ? WHERE key=?");
  duration                           dnow        = now.time_since_epoch();
  bind(query3, dnow.count(), key);
  query3.exec();
  return res;
}

template <class T>
inline void SqliteCache::set(const std::string& key, const T& value, time_point d)
{
  using namespace SQLite;
  cache_row_value values {};
  values.store_time     = std__chrono::utc_clock::now().time_since_epoch().count();
  if (d == time_point()) // standard expiration is today at midnight
  {
    d = last_point_of_today();
  }
  values.expire_time    = d.time_since_epoch().count();
  values.access_time    = values.store_time;
  auto               db = get_db();
  Statement          query(db, "INSERT OR REPLACE INTO cache(key,store_time,expire_time,access_time,access_count,size,value) VALUES(?,?,?,?,?,?,?)");
  std::ostringstream out;
  cereal::BinaryOutputArchive archive(out);
  archive(value);
  int sz = static_cast<int>(out.str().size());
  int ts = static_cast<int>(total_size(db));
  if (((size_t)ts + sz) > _max_size) // reclaim expired
    clean_expired(db);
  ts = total_size(db);
  if (((size_t)ts + sz) > _max_size)
  { // reclaim old
    int64_t need_to_free = (ts + sz) - _max_size;
    clean_older(db, need_to_free);
  }
  bind(query, key.c_str(), values.store_time, values.expire_time, values.access_time, values.accesss_count, sz);
  query.bind(7, reinterpret_cast<const void*>(out.str().c_str()), sz);
  query.exec();
}
