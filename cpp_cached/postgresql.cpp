#include "postgresql.h"

#include "is_a_cache.h"


std::vector<std::pair<std::string, std::string>> PostgresCache::get_pg_connection_config()
{
  std::vector<std::pair<std::string, std::string>> res;
  auto add = [&res](std::string_view name, std::string_view value, std::string_view env_name)
  {
    auto env_value = std::getenv(env_name.data());
    res.push_back({std::string(name), env_value ? std::string(env_value) : std::string(value)});
  };
  add("dbname", std::string("cpp_cached"), "PGDATABASE");
  add("password", "qustrat", "PGPASSWORD");
  add("user", "postgres", "PGUSER");
  add("host", "localhost", "PGHOST");
  add("port", "32769", "PGPORT");
  return res;
}

PostgresCache::PostgresCache() : _connection(get_pg_connection_config())
{
  create_db();
}

void PostgresCache::create_db()
{
  try
  {
    pqxx::work w(_connection);
    w.exec0("CREATE TABLE IF NOT EXISTS " pgCache
            " ("
            " key TEXT PRIMARY KEY,"
            " symbol TEXT,"
            " store_time bigint,"
            " expire_time bigint,"
            " access_time bigint,"
            " access_count bigint DEFAULT 0,"
            " size bigint DEFAULT 0,"
            " value BYTEA)");
    w.exec0("CREATE INDEX IF NOT EXISTS Cache_key ON " pgCache "(key)");
    w.exec0("CREATE INDEX IF NOT EXISTS Cache_symbol ON " pgCache "(symbol)");
    w.exec0("CREATE INDEX IF NOT EXISTS Cache_expire_time ON" pgCache "(expire_time)");
    w.exec0("CREATE INDEX IF NOT EXISTS Cache_access_time ON" pgCache "(access_time)");
    w.exec0("CREATE INDEX IF NOT EXISTS Cache_access_count ON" pgCache "(access_count)");
    w.commit();
  }
  catch (std::exception& e)
  {
    throw std::runtime_error(fmt::format("create_db failed {}\n", e.what()));
  }
  _connection.prepare("set",
                      "INSERT INTO " pgCache
                      "(key,symbol,store_time,expire_time,access_time,access_count,size,value) "
                      "VALUES($1,$2,$3,$4,$5,$6,$7,$8)");
  _connection.prepare("get", "SELECT value FROM " pgCache " where key = $1");
}
//
//Database PostgresCache::get_db(bool can_write)
//{
//  try
//  {
//    if (can_write)
//    {
//      Database db(_db_name.string(), OPEN_READWRITE);
//      return db;
//    }
//    Database db(_db_name.string(), OPEN_READONLY);
//    return db;
//  }
//  catch (std::exception& e)
//  {
//    throw std::runtime_error(fmt::format("get_db failed {}\n", e.what()));
//  }
//}
//
bool PostgresCache::has(const std::string& key)
{
  std::string where = "has:get";
  try
  {
    {
      pqxx::work w(_connection);
      auto       res = w.exec_params("SELECT key FROM " pgCache "WHERE key = $1", key);
      auto       s   = res.size();
      //pqxx::row r = w.exec1("SELECT 1, 2, 'Hello'");
      //Statement query = query_retry(db, "SELECT key FROM cache WHERE key = ?");
      //where = "has:bind";
      //query.bind(1, key);
      //where = "has::executeStep_retry";
      //if (!executeStep_retry(db, query)) return false;
      return s == 1;
    }
    //if (is_expired(key))
    //{
    //	auto db = get_db(true);
    //	really_erase(db, key);
    //	return false;
    //}
    //return true;
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(fmt::format("has failed : {} : {}", where, e.what()));
  }
}
//
//bool PostgresCache::is_expired(const std::string& skey)
//{
//  // asuming has
//  using duration = std__chrono::utc_clock::duration;
//  time_point  expire_time;
//  std::string where = "get_db";
//  auto        db    = get_db(false);
//  where             = "query0";
//  Statement query   = query_retry(db, "SELECT expire_time FROM CACHE where key = ?");
//  where             = "bind0";
//  auto key          = skey.c_str();
//  query.bind(1, key);
//  where        = "executeStep_retry";
//  auto success = executeStep_retry(db, query);
//  //if (! success) return res;
//  MREQUIRE(success, "{} not in db", skey);
//  where                 = "getColumns";
//  int64_t i_expire_time = query.getColumn(0).getInt64();
//  //auto    values = query.getColumns<cache_row_value, 6>();
//  //  expire_time = time_point(duration(values.expire_time));
//  expire_time = time_point(duration(i_expire_time));
//
//  std__chrono::utc_clock::time_point now = std__chrono::utc_clock::now();
//  // remove row if expired
//  return now > expire_time;
//}
//
void PostgresCache::erase(const std::string& key)
{
  if (! has(key)) return;
  //auto db = get_db(true);
  really_erase(key);
}
//
void PostgresCache::really_erase(const std::string& key)
{
  pqxx::work w(_connection);
  w.exec_params0("DELETE FROM " pgCache "where key=$1", key);
  w.commit();
}

void PostgresCache::erase_symbol(const std::string_view symbol)
{
  pqxx::work w(_connection);
  w.exec_params0("DELETE FROM " pgCache "where symbol=$1", symbol);
  w.commit();
}

//
size_t PostgresCache::total_size() const
{
  return _max_size;
  //  //Statement query(db, "SELECT SUM(size) FROM cache");
  //  //query.executeStep();
  //  //size_t res = query.getColumn(0).getInt();
  //  Statement query(db, "PRAGMA page_count");
  //  query.executeStep();
  //  size_t    pc = query.getColumn(0).getInt();
  //  Statement query0(db, "PRAGMA page_size");
  //  query0.executeStep();
  //  size_t ps  = query0.getColumn(0).getInt();
  //  size_t res = pc * ps;
  //  return res;
}
//
void PostgresCache::clean_expired()
{
  //  int64_t cnow = std__chrono::utc_clock::now().time_since_epoch().count();
  //
  //  std::vector<std::string> keys;
  //  Statement                query0(db, "SELECT key FROM cache WHERE expire_time < ?");
  //  query0.bind(1, cnow);
  //  while (query0.executeStep())
  //  {
  //    auto key = query0.getColumn(0).getString();
  //    keys.push_back(key);
  //  };
  //  if (! keys.empty()) fmt::print("cache will remove expired {}\n", fmt::join(keys, "\n"));
  //  Statement query(db, "DELETE FROM cache WHERE expire_time < ?");
  //  query.bind(1, cnow);
  //  query.exec();
  //  //std::vector<std::string> keys;
  //  //while (query.executeStep())
  //  //{
  //  //	keys.push_back(query.getColumn(0).getString());
  //  //}
  //  //query.reset();
}
//
void PostgresCache::clean_older(int64_t need_to_free)
{
  //  Statement query(db, "SELECT key,size FROM cache ORDER BY store_time");
  //  int64_t   fred = 0;
  //  fmt::print("clean_older\n");
  //  while (query.executeStep() && fred < need_to_free)
  //  {
  //    auto key = query.getColumn(0).getString();
  //    auto sz  = query.getColumn(1).getInt64();
  //    fmt::print("cache reclaim size {} will remove key {} sz {}\n", need_to_free - fred, key, sz);
  //    fred += sz;
  //    Statement query2(db, "DELETE FROM cache where key=?");
  //    query2.bind(1, key);
  //    query2.exec();
  //  }
}
//
std::shared_ptr<PostgresCache> PostgresCache::get_default()
{
  static auto res = std::make_shared<PostgresCache>();
  return res;
}
//
//#define SQLITE_OK 0 /* Successful result */
////extern "C" int sqlite3_changes(sqlite3*);
//
//int PostgresCache::exec_retry(SQLite::Database& db, const char* apQueries)
//{
//  return try_exec_retry(db, apQueries, NB_RETRIES);
//}
//
//
//int PostgresCache::exec_retry(SQLite::Database& db, SQLite::Statement& st)
//{
//  try_exec_retry(db, st, NB_RETRIES);
//
//  // Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
//  return sqlite3_changes(db.getHandle());
//}
//
//bool PostgresCache::executeStep_retry(SQLite::Database& db, SQLite::Statement& st)
//{
//  return try_executeStep_retry(db, st, NB_RETRIES);
//}
//
//SQLite::Statement PostgresCache::query_retry(const SQLite::Database& aDatabase, const char* apQuery)
//{
//  int nb_retries = NB_RETRIES;
//  while (nb_retries > 0)
//  {
//    try
//    {
//      return Statement{aDatabase, apQuery};
//    }
//    catch (const std::exception& e)
//    {
//      std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME));
//      nb_retries--;
//      if (! nb_retries) throw std::runtime_error(fmt::format("query giving up : {}", e.what()));
//    }
//  }
//  throw;
//}


namespace
{
  void fun()
  {
    static_assert(is_a_cache<PostgresCache>);
  }
}  // namespace
