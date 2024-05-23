#pragma once

#if ! defined(WITH_POSTGRES)
#error("WITH_POSTGRES must be defined")
#endif

#include <cpp_rutils/date.h>
#include <pqxx/pqxx>
#include <cereal/archives/binary.hpp>
//#include <cereal/archives/json.hpp>
#include <cpp_rutils/literals.h>
#include <cpp_rutils/memstream.h>

#include "cache_entry.h"
#include "cache_names.h"
#include "time_point.h"

#define pgCache " pgCache" CACHE_NAME_PREFIX CACHE_NAME_POSTFIX " "



class PostgresCache
{

 public:
  //using time_point = std__chrono::utc_clock::time_point;

  static std::vector<std::pair<std::string, std::string>> get_pg_connection_config();

  PostgresCache();
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

  static std::shared_ptr<PostgresCache> get_default();

  //private:
  //std::filesystem::path _root_path, _db_name;
  uint64_t _max_size = 256_GB;


  //static int               exec_retry(SQLite::Database& db, const char* apQueries);
  //static int               exec_retry(SQLite::Database& db, SQLite::Statement& st);
  //static bool              executeStep_retry(SQLite::Database& db, SQLite::Statement& st);
  //static SQLite::Statement query_retry(const SQLite::Database& aDatabase, const char* apQuery);

  void create_db();

  pqxx::connection _connection;
  //SQLite::Database get_db(bool can_write);
  //bool             is_expired(const std::string& key);
  void really_erase(const std::string& key);

  void erase_symbol(const std::string_view symbol);
};

template <class T>
T PostgresCache::get(const std::string& s_key)
{
  //using namespace SQLite;
  using duration = std__chrono::utc_clock::duration;
  T           res;
  std::string where = "get_db";
  try
  {
    auto key = s_key.c_str();
    MREQUIRE(has(s_key), "{} not found in DB", s_key);
    {
      // get blob
      where = "blob get_db";
      //	auto db = get_db(true);
      where = "blob query";
      where = "blob bind";
      //query.bind(1, key);
      //where = "blob executeStep";
      //auto success = executeStep_retry(db, query);
      pqxx::work work(_connection);
      auto       r = work.exec_prepared("get", key);
      work.commit();
      auto [col_blob] = r[0].as<std::basic_string<std::byte>>();
      //if (! success) return res;
      //MREQUIRE(success, "getting blob failed for {}", s_key);
      //where = "blob getColumn";
      //auto col_blob = query.getColumn(6);
      //where = "blob get_blob";
      imemstream sin(reinterpret_cast<const char*>(col_blob.data()), col_blob.size());
      cereal::BinaryInputArchive archive(sin);
      //cereal::JSONInputArchive archive(sin);
      //T                          tmp;
      where = "blob archive";
      archive(res);
      //res = tmp;
    }
    {
      // update counters
      where = "get2";
      //auto db = get_db(true);
      //where = "query2";
      pqxx::work work(_connection);
      work.exec_params0("UPDATE " pgCache " SET access_count = access_count + 1 WHERE key=$1", key);
      //where = "bind2";
      //query2.bind(1, key);
      //where = "exec_retry2";
      //exec_retry(db, query2);
      ////query2.exec();
      //where = "query2";
      std__chrono::utc_clock::time_point now  = std__chrono::utc_clock::now();
      duration                           d_now = now.time_since_epoch();
      work.exec_params0("UPDATE " pgCache " SET access_time = $1 WHERE key=$2", d_now.count(), key);
      //where = "bind3";
      //bind(query3, d_now.count(), key);
      //where = "exec_retry3";
      //exec_retry(db, query3);
      work.commit();
    }
  }
  catch (std::exception& e)
  {
    //std__print("failed reading blob : {} \n", where);
    throw std::runtime_error(fmt::format("failed reading blob at {} since {}\n", where, e.what()));
  }
  return res;
}


template <class T>
void PostgresCache::set(const std::string&     key,
                        const T&               value,
                        std::string_view       symbol,
                        cpp_cached::time_point d)
{
  //using namespace SQLite;
  cache_row_value values{};
  values.store_time = std__chrono::utc_clock::now().time_since_epoch().count();
  if (d == cpp_cached::time_point())  // standard expiration is today at midnight
  {
    d = cpp_cached::last_point_of_today();
  }
  values.expire_time = d.time_since_epoch().count();
  values.access_time = values.store_time;
  ////auto                        db = get_db(true);
  ////Statement                   query(db,
  ////	"INSERT OR REPLACE INTO "
  ////	"cache(key,store_time,expire_time,access_time,access_count,size,value) "
  ////	"VALUES(?,?,?,?,?,?,?)");
  std::ostringstream          out;
  cereal::BinaryOutputArchive archive(out);
  //cereal::JSONOutputArchive archive(out);
  archive(value);
  size_t sz = out.str().size();
  size_t ts = total_size();
  if ((ts + sz) > _max_size)  // reclaim expired
    clean_expired();
  ts = total_size();
  if ((ts + sz) > _max_size)
  {  // reclaim old
    size_t need_to_free = (ts + sz) - _max_size;
    clean_older(need_to_free);
  }
  std::basic_string<std::byte> blob(reinterpret_cast<const std::byte*>(out.str().c_str()),
                                    out.str().size());
  pqxx::work                   work(_connection);
  work.exec_prepared0("set", key, std::string{symbol}, values.store_time, values.expire_time,
                      values.access_time, values.access_count, sz, blob);
  //int i_sz = static_cast<int>(sz);
  //bind(query, key.c_str(), values.store_time, values.expire_time, values.access_time,
  //	values.access_count, i_sz);
  //query.bind(7, reinterpret_cast<const void*>(out.str().c_str()), i_sz);
  //query.exec();
  work.commit();
}
//
//template <class F>
//std::invoke_result_t<F> SqliteCache::get(const std::string& key, F callback, time_point d)
//{
//  using T      = std::invoke_result_t<F>;
//  bool has_key = has(key);
//  if (has_key) return get<T>(key);
//  T res = callback();
//  set(key, res, d);
//  return res;
//}
template <class F>
std::invoke_result_t<F> PostgresCache::get(const std::string&     key,
                                           F                      callback,
                                           cpp_cached::time_point d)
{
  using T      = std::invoke_result_t<F>;
  bool has_key = has(key);
  if (has_key) return get<T>(key);
  T res = callback();
  set(key, res, d);
  return res;
}
