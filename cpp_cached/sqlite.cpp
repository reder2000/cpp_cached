#include "sqlite.h"
#include "sqlite3.h"
#include <thread>
#include <fmt/ranges.h>

using namespace SQLite;

constexpr int NB_RETRIES = 50;
constexpr int BUSYTIME = 2000;


SqliteCache::SqliteCache(std::filesystem::path root_path, uint64_t max_size)
	: _root_path(std::move(root_path)), _max_size(max_size)
{
	auto rerroot = _root_path / machine_name();
	_db_name = rerroot / "cache.db";
	if (!std::filesystem::exists(rerroot)) MREQUIRE(std::filesystem::create_directories(rerroot));
	create_db();
}

void SqliteCache::create_db()
{
	try
	{
		Database db(_db_name.string(), OPEN_READWRITE | OPEN_CREATE);
		//db.setBusyTimeout(1000);
		exec_retry(db,
			"CREATE TABLE IF NOT EXISTS Cache ("
			" key TEXT PRIMARY KEY,"
			" store_time INTEGER,"
			" expire_time INTEGER,"
			" access_time INTEGER,"
			" access_count INTEGER DEFAULT 0,"
			" size INTEGER DEFAULT 0,"
			" value BLOB)");

		exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_expire_time ON"
			" Cache (expire_time)");
		exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_access_time ON"
			" Cache (access_time)");
		exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_access_count ON"
			" Cache (access_count)");
	}
	catch (std::exception& e)
	{
		throw std::runtime_error(fmt::format("create_db failed {}\n", e.what()));
	}
}

Database SqliteCache::get_db(bool can_write)
{
	try
	{
		if (can_write)
		{
			Database db(_db_name.string(), OPEN_READWRITE);
			return db;
		}
		Database db(_db_name.string(), OPEN_READONLY);
		return db;
	}
	catch (std::exception& e)
	{
		throw std::runtime_error(fmt::format("get_db failed {}\n", e.what()));
	}
}

bool SqliteCache::has(const std::string& key)
{
	std::string where = "has:get";
	try
	{
		{
			auto db = get_db(false);
			where = "has:query";
			Statement query = query_retry(db, "SELECT key FROM cache WHERE key = ?");
			where = "has:bind";
			query.bind(1, key);
			where = "has::executeStep_retry";
			if (!executeStep_retry(db, query)) return false;
		}
		if (is_expired(key))
		{
			auto db = get_db(true);
			really_erase(db, key);
			return false;
		}
		return true;
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(fmt::format("has failed : {} : {}", where, e.what()));
	}
}

bool SqliteCache::is_expired(const std::string& skey)
{
	// asuming has
	using duration = std__chrono::utc_clock::duration;
	time_point  expire_time;
	std::string where = "get_db";
	auto        db = get_db(false);
	where = "query0";
	Statement query = query_retry(db, "SELECT expire_time FROM CACHE where key = ?");
	where = "bind0";
	auto key = skey.c_str();
	query.bind(1, key);
	where = "executeStep_retry";
	auto success = executeStep_retry(db, query);
	//if (! success) return res;
	MREQUIRE(success, "{} not in db", skey);
	where = "getColumns";
	int64_t i_expire_time = query.getColumn(0).getInt64();
	//auto    values = query.getColumns<cache_row_value, 6>();
	//  expire_time = time_point(duration(values.expire_time));
	expire_time = time_point(duration(i_expire_time));

	std__chrono::utc_clock::time_point now = std__chrono::utc_clock::now();
	// remove row if expired
	return now > expire_time;
}

void SqliteCache::erase(const std::string& key)
{
	if (!has(key)) return;
	auto db = get_db(true);
	really_erase(db, key);
}

void SqliteCache::really_erase(SQLite::Database& db, const std::string& key)
{
	Statement query(db, "DELETE FROM cache where key=?");
	query.bind(1, key);
	query.exec();
}

size_t SqliteCache::total_size(Database& db) const
{
	//Statement query(db, "SELECT SUM(size) FROM cache");
	//query.executeStep();
	//size_t res = query.getColumn(0).getInt();
	Statement query(db, "PRAGMA page_count");
	query.executeStep();
	size_t    pc = query.getColumn(0).getInt();
	Statement query0(db, "PRAGMA page_size");
	query0.executeStep();
	size_t ps = query0.getColumn(0).getInt();
	size_t res = pc * ps;
	return res;
}

void SqliteCache::clean_expired(Database& db)
{
	int64_t cnow = std__chrono::utc_clock::now().time_since_epoch().count();

	std::vector<std::string> keys;
	Statement                query0(db, "SELECT key FROM cache WHERE expire_time < ?");
	query0.bind(1, cnow);
	while (query0.executeStep())
	{
		auto key = query0.getColumn(0).getString();
		keys.push_back(key);
	};
	if (!keys.empty()) fmt::print("cache will remove expired {}\n", fmt::join(keys, "\n"));
	Statement query(db, "DELETE FROM cache WHERE expire_time < ?");
	query.bind(1, cnow);
	query.exec();
	//std::vector<std::string> keys;
	//while (query.executeStep())
	//{
	//	keys.push_back(query.getColumn(0).getString());
	//}
	//query.reset();
}

void SqliteCache::clean_older(Database& db, int64_t need_to_free)
{
	Statement query(db, "SELECT key,size FROM cache ORDER BY store_time");
	int64_t   fred = 0;
	fmt::print("clean_older\n");
	while (query.executeStep() && fred < need_to_free)
	{
		auto key = query.getColumn(0).getString();
		auto sz = query.getColumn(1).getInt64();
		fmt::print("cache reclaim size {} will remove key {} sz {}\n", need_to_free - fred, key, sz);
		fred += sz;
		Statement query2(db, "DELETE FROM cache where key=?");
		query2.bind(1, key);
		query2.exec();
	}
}

std::shared_ptr<SqliteCache> SqliteCache::get_default()
{
	static auto res = std::make_shared<SqliteCache>();
	return res;
}

#define SQLITE_OK 0 /* Successful result */
//extern "C" int sqlite3_changes(sqlite3*);

int SqliteCache::exec_retry(SQLite::Database& db, const char* apQueries)
{
	return try_exec_retry(db, apQueries, NB_RETRIES);
}


int SqliteCache::exec_retry(SQLite::Database& db, SQLite::Statement& st)
{
	try_exec_retry(db, st, NB_RETRIES);

	// Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
	return sqlite3_changes(db.getHandle());
}

bool SqliteCache::executeStep_retry(SQLite::Database& db, SQLite::Statement& st)
{
	return try_executeStep_retry(db, st, NB_RETRIES);
}

SQLite::Statement SqliteCache::query_retry(const SQLite::Database& aDatabase, const char* apQuery)
{
	int nb_retries = NB_RETRIES;
	while (nb_retries > 0)
	{
		try
		{
			return Statement{ aDatabase, apQuery };
		}
		catch (const std::exception& e)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME));
			nb_retries--;
			if (!nb_retries) throw std::runtime_error(fmt::format("query giving up : {}", e.what()));
		}
	}
	throw;
}

int try_exec_retry(SQLite::Database& db, const char* apQueries, int nb_retries)
{
	int res;
	while (nb_retries--)
	{
		res = db.tryExec(apQueries);
		if (res == SQLITE_OK)
		{
			return sqlite3_changes(db.getHandle());
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME));
		}
	}
	//    if (SQLITE_OK != res && SQLITE_DONE != res && SQLITE_BUSY != res && SQLITE_LOCKED != res)

	throw std::runtime_error(fmt::format("exec apQueries {}, giving up : {}", apQueries,
		SQLite::Exception(db.getHandle(), res).what()));
}


int try_exec_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries);
bool try_executeStep_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries);


int try_exec_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries)
{
	int res;
	int nb_retries1 = nb_retries;
	while (nb_retries1--)
	{
		res = st.tryExecuteStep();
		if (SQLITE_DONE == res)  // the statement has finished executing successfully
			return res;
		std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME));
	}
	nb_retries1 = 2 * nb_retries;
	while (nb_retries1--)
	{
		res = st.tryExecuteStep();
		if (SQLITE_DONE == res)  // the statement has finished executing successfully
			return res;
		std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME * 2));
	}
	if (SQLITE_ROW == res)
	{
		throw std::runtime_error(
			fmt::format("Statement exec, giving up : {}",
				SQLite::Exception("exec() does not expect results. Use executeStep.").what()));
	}
	else if (res == sqlite3_errcode(db.getHandle()))
	{
		throw std::runtime_error(fmt::format("Statement exec, giving up : {}",
			SQLite::Exception(db.getHandle(), res).what()));
	}
	else
	{
		throw std::runtime_error(
			fmt::format("Statement exec, giving up : {}",
				SQLite::Exception("Statement needs to be reseted", res).what()));
	}
}

bool try_executeStep_retry(SQLite::Database& db, SQLite::Statement& st, int nb_retries)
{
	int res;
	while (nb_retries--)
	{
		res = st.tryExecuteStep();
		if ((SQLITE_ROW != res) && (SQLITE_DONE != res))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(BUSYTIME));
		}
		else
		{
			return st.hasRow();
		}
	}
	if (res == sqlite3_errcode(db.getHandle()))
	{
		throw std::runtime_error(
			fmt::format("executeStep, giving up : {}", SQLite::Exception(db.getHandle(), res).what()));
	}
	else
	{
		throw std::runtime_error(
			fmt::format("executeStep, giving up : {}",
				SQLite::Exception("Statement needs to be reseted", res).what()));
	}
}
