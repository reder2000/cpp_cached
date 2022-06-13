#include "cache_imp_exp.h"
#include <filesystem>
#include <cstdint>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <cpp_rutils/require.h>
#include<iostream>
#include <sqlite3.h>
#include <cereal/archives/binary.hpp>

#include <streambuf>
#include <istream>

struct membuf : std::streambuf {
	membuf(char const* base, size_t size) {
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};
struct imemstream : virtual membuf, std::istream {
	imemstream(char const* base, size_t size)
		: membuf(base, size)
		, std::istream(static_cast<std::streambuf*>(this)) {
	}
};

// sqlite cache parameters:
// - size: size of the cache in bytes
// - path: path to the cache directory
// - data max age : either in seconds or days (triggers at midnight in a given timezone)


constexpr uint64_t operator"" _B(uint64_t bytes)
{
	return bytes;
}
constexpr uint64_t operator"" _kB(uint64_t kilo_bytes)
{
	return 1024 * kilo_bytes;
}
constexpr uint64_t operator"" _MB(uint64_t mega_bytes)
{
	return 1024_kB * mega_bytes;
}
constexpr uint64_t operator"" _GB(uint64_t giga_bytes)
{
	return 1024_MB * giga_bytes;
}

//const std::chrono::time_point<std::chrono::system_clock> now =
//std::chrono::system_clock::now();
//auto start = std::chrono::utc_clock::now();


class cpp_cached_API SqliteCache {

public:

	using duration = std::chrono::utc_clock::duration;
	static constexpr duration max_duration = duration(std::numeric_limits<int64_t>::max());


	SqliteCache(const std::filesystem::path& root_path =
		std::filesystem::temp_directory_path().append("cache").append("sqlite"),
		uint64_t size = 2_GB);

	SQLite::Database get_db(bool create_tables=false);

	bool contains(const char* key);

	template <class T>
	void set(const char* key, const T& value, const duration& d = max_duration);

	template <class T>
	std::optional<T> get(const char* key);

private:

	std::filesystem::path _root_path, _db_name;
	uint64_t _size;

	size_t total_size(SQLite::Database &db) const;

};


using namespace  SQLite;

SqliteCache::SqliteCache(const std::filesystem::path& root_path, uint64_t size) :
	_root_path(root_path) , _size(size)
{
	_db_name = _root_path / "cache.db";
	//_db_name /= "cache.db";
	//time_t tt;
	//time(&tt);
	//struct tm tm;
	//localtime_s(&tm, &tt);
	//auto s = fmt::format("{:%Y-%m-%d}", tm);
	//_root_path.append(s);
	if (!std::filesystem::exists(_root_path))
		MREQUIRE(std::filesystem::create_directories(_root_path));
	auto db = get_db(true);
	std::cout << total_size(db) << std::endl;
}

struct cache_row_value
{
	std::string key;
	int64_t store_time;
	int64_t expire_time;
	int64_t access_time;
	int64_t accesss_count;
	int64_t size;
};

SQLite::Database SqliteCache::get_db(bool create_tables)
{

	SQLite::Database db(_db_name.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

	if (create_tables) {

		db.exec(
			"CREATE TABLE IF NOT EXISTS Cache ("
			" key TEXT PRIMARY KEY,"
			" store_time INTEGER,"
			" expire_time INTEGER,"
			" access_time INTEGER,"
			" access_count INTEGER DEFAULT 0,"
			" size INTEGER DEFAULT 0,"
			" value BLOB)");

		db.exec(
			"CREATE INDEX IF NOT EXISTS Cache_expire_time ON"
			" Cache (expire_time)"
		);
		db.exec(
			"CREATE INDEX IF NOT EXISTS Cache_access_time ON"
			" Cache (access_time)"
		);
		db.exec(
			"CREATE INDEX IF NOT EXISTS Cache_access_count ON"
			" Cache (access_count)"
		);


	}

	return db;
}

size_t SqliteCache::total_size(SQLite::Database &db) const
{
	SQLite::Statement   query(db, "SELECT SUM(size) FROM cache");
	query.executeStep();
	size_t res = query.getColumn(0).getInt();
	return res;
}


// Helper function called by getColums<typename T, int N>
//template<const int... Is>
//auto  rmgetColumns(Statement & query, const std::integer_sequence<int, Is...>)
//{
//	return { Column(query.mStmtPtr, Is)... };
//}
template<int N>
auto mgetColumns(Statement& query)
{
	//	query.checkRow();
	//	query.checkIndex(N - 1);
	//return rmgetColumns(query, std::make_integer_sequence<int, N>{});
	return query.getColumns<Column, N>();
}



template <class T> inline
std::optional<T> SqliteCache::get( const char* key)
{
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
	// get blob & update values
	auto col_blob = query.getColumn(6);
	imemstream sin( reinterpret_cast<const char*>(col_blob.getBlob()),col_blob.size() );
	cereal::BinaryInputArchive archive(sin);
	T tmp;
	archive( tmp );
	res = tmp;
	return res; 
}


template <class T>
void SqliteCache::set(const char* key, const T& value, const duration& d)
{
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
	bind(query, key, values.store_time, values.expire_time, values.access_time, values.accesss_count, sz);
	query.bind( 7, reinterpret_cast<const void*>(out.str().c_str()), sz);
	query.exec();
}
