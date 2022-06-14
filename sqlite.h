#pragma once

// disk cache data stored in sqlite db

#include "cache_imp_exp.h"

#include <filesystem>
#include <cpp_rutils/literals.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <cpp_rutils/require.h>
#include <cpp_rutils/memstream.h>
#include <cereal/archives/binary.hpp>

// sqlite cache parameters:
// - size: size of the cache in bytes
// - path: path to the cache directory
// - data max age : either in seconds or days (triggers at midnight in a given timezone)


class cpp_cached_API SqliteCache {

public:

	using duration = std::chrono::utc_clock::duration;
	static constexpr duration max_duration = duration(std::numeric_limits<int64_t>::max());
	static std::filesystem::path default_root_path() {
		return
			std::filesystem::temp_directory_path().append("cache").append("sqlite");
	}

	SqliteCache(const std::filesystem::path& root_path =
		default_root_path(),
		uint64_t max_size = 2_GB);

	SQLite::Database get_db(bool create_tables=false);

	bool contains(const char* key);

	template <class T>
	void set(const char* key, const T& value, const duration& d = max_duration);

	template <class T>
	std::optional<T> get(const char* key);

private:

	std::filesystem::path _root_path, _db_name;
	uint64_t _max_size;

	size_t total_size(SQLite::Database &db) const;
	void clean_expired(SQLite::Database& db);
	void clean_older(SQLite::Database& db, int64_t need_to_free);
};

// struct for extracting all fields
struct cache_row_value
{
	std::string key;
	int64_t store_time;
	int64_t expire_time;
	int64_t access_time;
	int64_t accesss_count;
	int64_t size;
};

template <class T> inline
std::optional<T> SqliteCache::get( const char* key)
{
	using namespace  SQLite;
	std::optional<T> res;
	auto db = get_db();
	Statement query(db, "SELECT * FROM CACHE where key = ?");
	query.bind(1, key );
	auto success = query.executeStep();
	if (!success)
		return res;
	auto values = query.getColumns<cache_row_value, 6>();
	duration expire_time = duration(values.expire_time);
	std::chrono::utc_clock::time_point now = std::chrono::utc_clock::now();
	duration dnow = now.time_since_epoch();
	// remove row if expired
	if (dnow>expire_time)
	{
		Statement query(db, "DELETE FROM cache WHERE key = ?");
		query.bind(1, key);
		query.exec();
		return res;
	}
	// get blob
	auto col_blob = query.getColumn(6);
	imemstream sin( reinterpret_cast<const char*>(col_blob.getBlob()),col_blob.size() );
	cereal::BinaryInputArchive archive(sin);
	T tmp;
	archive( tmp );
	res = tmp;
	// update counters
	Statement query2(db, "UPDATE cache SET access_count = access_count + 1 WHERE key=?");
	query2.bind(1, key);
	query2.exec();
	Statement query3(db, "UPDATE cache SET access_time = ? WHERE key=?");
	bind(query3, dnow.count(), key);
	query3.exec();
	return res;
}

template <class T> inline
void SqliteCache::set(const char* key, const T& value, const duration& d)
{
	using namespace  SQLite;
	cache_row_value values{};
	values.store_time = std::chrono::utc_clock::now().time_since_epoch().count();
	values.expire_time = d.count();
	values.access_time = values.store_time;
	auto db = get_db();
	Statement query(db, "INSERT OR REPLACE INTO cache(key,store_time,expire_time,access_time,access_count,size,value) VALUES(?,?,?,?,?,?,?)");
	std::ostringstream out;
	cereal::BinaryOutputArchive archive(out);
	archive(value);
	int sz = out.str().size();
	int ts = total_size(db);
	if ((ts+sz)>_max_size)		// reclaim expired
		clean_expired(db);
	ts = total_size(db);
	if ((ts + sz) > _max_size) {		// reclaim old
		int64_t need_to_free = (ts + sz) - _max_size;
		clean_older(db, need_to_free);
	}
	bind(query, key, values.store_time, values.expire_time, values.access_time, values.accesss_count, sz);
	query.bind( 7, reinterpret_cast<const void*>(out.str().c_str()), sz);
	query.exec();
}
