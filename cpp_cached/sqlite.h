#pragma once

#if ! defined(WITH_SQLITE)
#error("WITH_SQLITE must be defined")
#endif

// disk cache data stored in sqlite db

#include "cache_imp_exp.h"

#include <filesystem>
#include <cpp_rutils/literals.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <cpp_rutils/require.h>
#include <cpp_rutils/memstream.h>
#include <cpp_rutils/date.h>
#include <cereal/archives/binary.hpp>

#include "cache_entry.h"
#include "time_point.h"

int try_exec_retry(SQLite::Database& db, const char* apQueries, int nb_retries);
int try_exec_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries);
bool try_executeStep_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries);

// sqlite cache parameters:
// - size: size of the cache in bytes
// - path: path to the cache directory
// - data max age : either in seconds or days (triggers at midnight in a given timezone)


class cpp_cached_API SqliteCache
{

public:
	//using time_point = std__chrono::utc_clock::time_point;
	using time_point = cpp_cached::time_point;
	static std::filesystem::path default_root_path()
	{
		return std::filesystem::temp_directory_path().append("cache").append("sqlite");
	}
	static constexpr size_t default_max_size = 8_GB;

	SqliteCache(std::filesystem::path root_path = default_root_path(),
		uint64_t              max_size = default_max_size);


	bool has(const std::string& key);
	void erase(const std::string& key);
	template <class T>
	T get(const std::string& key);
	template <class T>
	void set(const std::string& key, const T& value, time_point d = time_point());
	// gets a value, compute it if necessary
	template <class F>
	std::invoke_result_t<F> get(const std::string& key, F callback, time_point d = time_point());

	size_t total_size(SQLite::Database& db) const;
	void   clean_expired(SQLite::Database& db);
	void   clean_older(SQLite::Database& db, int64_t need_to_free);

	static std::shared_ptr<SqliteCache> get_default();

private:
	std::filesystem::path _root_path, _db_name;
	uint64_t              _max_size;


	static int               exec_retry(SQLite::Database& db, const char* apQueries);
	static int               exec_retry(SQLite::Database& db, SQLite::Statement& st);
	static bool              executeStep_retry(SQLite::Database& db, SQLite::Statement& st);
	static SQLite::Statement query_retry(const SQLite::Database& aDatabase, const char* apQuery);

	void             create_db();
	SQLite::Database get_db(bool can_write);
	bool             is_expired(const std::string& key);
	void             really_erase(SQLite::Database& db, const std::string& key);
};

template <class T>
T SqliteCache::get(const std::string& s_key)
{
	using namespace SQLite;
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
			auto db = get_db(true);
			where = "blob query";
			Statement query = query_retry(db, "SELECT * FROM CACHE where key = ?");
			where = "blob bind";
			query.bind(1, key);
			where = "blob executeStep";
			auto success = executeStep_retry(db, query);
			//if (! success) return res;
			MREQUIRE(success, "getting blob failed for {}", s_key);
			where = "blob getColumn";
			auto col_blob = query.getColumn(6);
			where = "blob getblob";
			imemstream sin(reinterpret_cast<const char*>(col_blob.getBlob()), col_blob.size());
			cereal::BinaryInputArchive archive(sin);
			//T                          tmp;
			where = "blob archive";
			archive(res);
			//res = tmp;
		}
		{
			// update counters
			where = "get2";
			auto db = get_db(true);
			where = "query2";
			Statement query2 =
				query_retry(db, "UPDATE cache SET access_count = access_count + 1 WHERE key=?");
			where = "bind2";
			query2.bind(1, key);
			where = "exec_retry2";
			exec_retry(db, query2);
			//query2.exec();
			where = "query2";
			Statement query3 = query_retry(db, "UPDATE cache SET access_time = ? WHERE key=?");
			std__chrono::utc_clock::time_point now = std__chrono::utc_clock::now();
			duration                           d_now = now.time_since_epoch();
			where = "bind3";
			bind(query3, d_now.count(), key);
			where = "exec_retry3";
			exec_retry(db, query3);
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
void SqliteCache::set(const std::string& key, const T& value, time_point d)
{
	using namespace SQLite;
	cache_row_value values{};
	values.store_time = std__chrono::utc_clock::now().time_since_epoch().count();
	if (d == time_point())  // standard expiration is today at midnight
	{
		d = cpp_cached::last_point_of_today();
	}
	values.expire_time = d.time_since_epoch().count();
	values.access_time = values.store_time;
	auto                        db = get_db(true);
	Statement                   query(db,
		"INSERT OR REPLACE INTO "
		"cache(key,store_time,expire_time,access_time,access_count,size,value) "
		"VALUES(?,?,?,?,?,?,?)");
	std::ostringstream          out;
	cereal::BinaryOutputArchive archive(out);
	archive(value);
	size_t sz = out.str().size();
	size_t ts = total_size(db);
	if ((ts + sz) > _max_size)  // reclaim expired
		clean_expired(db);
	ts = total_size(db);
	if ((ts + sz) > _max_size)
	{  // reclaim old
		size_t need_to_free = (ts + sz) - _max_size;
		clean_older(db, need_to_free);
	}
	int i_sz = static_cast<int>(sz);
	bind(query, key.c_str(), values.store_time, values.expire_time, values.access_time,
		values.access_count, i_sz);
	query.bind(7, reinterpret_cast<const void*>(out.str().c_str()), i_sz);
	query.exec();
}

template <class F>
std::invoke_result_t<F> SqliteCache::get(const std::string& key, F callback, time_point d)
{
	using T = std::invoke_result_t<F>;
	bool has_key = has(key);
	if (has_key) return get<T>(key);
	T res = callback();
	set(key, res, d);
	return res;
}
