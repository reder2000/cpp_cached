#include "sqlite_single.h"
#include "sqlite3.h"
#include <thread>
#include <cpp_rutils/assign_m.h>

using namespace SQLite;

constexpr int NB_RETRIES = 50;
constexpr int BUSYTIME = 2000;

std::string machine_name()
{
#if _MSC_VER
#if __clang__
#define MACHIN_PREFIX "CLANGCL"
#else
#define MACHIN_PREFIX "MSC"
#endif
#else
#define MACHIN_PREFIX "XXX"
#endif

#if _DEBUG
	return MACHIN_PREFIX "_DEBUG";
#else
	return MACHIN_PREFIX "_RELEASE";
#endif
}

SqliteSingleCache::SqliteSingleCache(std::filesystem::path root_path, uint64_t max_size)
	: AM_(root_path), A_(max_size)
{
	auto rerroot = _root_path / machine_name();
	_db_name = rerroot / "cache.db";
	if (!std::filesystem::exists(rerroot)) MREQUIRE(std::filesystem::create_directories(rerroot));
	create_db();
	//get_db();
}

void SqliteSingleCache::create_db()
{
	try
	{
		//Database db(_db_name.string(), OPEN_READWRITE | OPEN_CREATE);
		_db = std::make_unique<Database>(_db_name.string(), OPEN_READWRITE | OPEN_CREATE);
		Database& db = *_db;
		//db.setBusyTimeout(1000);
		try_exec_retry(db,
			"CREATE TABLE IF NOT EXISTS Cache ("
			" key TEXT PRIMARY KEY,"
			" store_time INTEGER,"
			" expire_time INTEGER,"
			" access_time INTEGER,"
			" access_count INTEGER DEFAULT 0,"
			" size INTEGER DEFAULT 0,"
			" value BLOB)",
			1);

		try_exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_expire_time ON"
			" Cache (expire_time)",
			1);
		try_exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_access_time ON"
			" Cache (access_time)",
			1);
		try_exec_retry(db,
			"CREATE INDEX IF NOT EXISTS Cache_access_count ON"
			" Cache (access_count)",
			1);
		//try_exec_retry(db, "PRAGMA mmap_size=2147483648;", 1);
		try_exec_retry(db, "PRAGMA SYNCHRONOUS=0;", 1);
		// try_exec_retry(db,   "PRAGMA PAGE_SIZE=8192",1);;
	}
	catch (std::exception& e)
	{
		throw std::runtime_error(fmt::format("create_db failed {}\n", e.what()));
	}
}

//void SqliteSingleCache::get_db()
//{
//	try
//	{
//			_db= std::make_unique<Database>(_db_name.string(), OPEN_READWRITE);
//	}
//	catch (std::exception& e)
//	{
//		throw std::runtime_error(fmt::format("get_db failed {}\n", e.what()));
//	}
//}

bool SqliteSingleCache::has(const std::string& key)
{
	std::string where = "has:get";
	try
	{
		{
			where = "has:query";
			Statement query = query_retry("SELECT key FROM cache WHERE key = ?");
			where = "has:bind";
			query.bind(1, key);
			where = "has::executeStep_retry";
			if (!executeStep_retry(query)) return false;
		}
		if (is_expired(key))
		{
			really_erase(key);
			return false;
		}
		return true;
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(fmt::format("has failed : {} : {}", where, e.what()));
	}
}

bool SqliteSingleCache::is_expired(const std::string& skey)
{
	// asuming has
	using duration = std__chrono::utc_clock::duration;
	time_point  expire_time;
	std::string where = "get_db";
	where = "query0";
	Statement query = query_retry("SELECT expire_time FROM CACHE where key = ?");
	where = "bind0";
	auto key = skey.c_str();
	query.bind(1, key);
	where = "executeStep_retry";
	auto success = executeStep_retry(query);
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

void SqliteSingleCache::erase(const std::string& key)
{
	if (!has(key)) return;
	really_erase(key);
}

void SqliteSingleCache::really_erase(const std::string& key)
{
	Statement query(*_db, "DELETE FROM cache where key=?");
	query.bind(1, key);
	query.exec();
}

size_t SqliteSingleCache::total_size() const
{
	Statement query(*_db, "SELECT SUM(size) FROM cache");
	query.executeStep();
	size_t res = query.getColumn(0).getInt();
	return res;
}

void SqliteSingleCache::clean_expired()
{
	int64_t cnow = std__chrono::utc_clock::now().time_since_epoch().count();

	Statement query(*_db, "DELETE FROM cache WHERE expire_time < ?");
	query.bind(1, cnow);
	query.exec();
	//std::vector<std::string> keys;
	//while (query.executeStep())
	//{
	//	keys.push_back(query.getColumn(0).getString());
	//}
	//query.reset();
}

void SqliteSingleCache::clean_older(int64_t need_to_free)
{
	Statement query(*_db, "SELECT key,size FROM cache ORDER BY store_time");
	int64_t   fred = 0;
	while (query.executeStep() && fred < need_to_free)
	{
		auto key = query.getColumn(0).getString();
		auto sz = query.getColumn(1).getInt64();
		fred += sz;
		Statement query2(*_db, "DELETE FROM cache where key=?");
		query2.bind(1, key);
		query2.exec();
	}
}

std::shared_ptr<SqliteSingleCache> SqliteSingleCache::get_default()
{
	static auto res = std::make_shared<SqliteSingleCache>();
	return res;
}

#define SQLITE_OK 0 /* Successful result */
//extern "C" int sqlite3_changes(sqlite3*);

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

int SqliteSingleCache::exec_retry(const char* apQueries)
{
	return try_exec_retry(*_db, apQueries, NB_RETRIES);
}

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

int SqliteSingleCache::exec_retry(SQLite::Statement& st)
{
	try_exec_retry(*_db, st, NB_RETRIES);

	// Return the number of rows modified by those SQL statements (INSERT, UPDATE or DELETE)
	return sqlite3_changes(_db->getHandle());
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

bool SqliteSingleCache::executeStep_retry(SQLite::Statement& st)
{
	return try_executeStep_retry(*_db, st, NB_RETRIES);
}

SQLite::Statement SqliteSingleCache::query_retry(const char* apQuery)
{
	int nb_retries = NB_RETRIES;
	while (nb_retries > 0)
	{
		try
		{
			return Statement{ *_db, apQuery };
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

//SqliteSingleCache::time_point SqliteSingleCache::last_point_of_today()
//{
//  auto now               = std__chrono::utc_clock::now();
//  auto nowsys            = std__chrono::utc_clock::to_sys(now);
//  auto nowsystmt         = std::chrono::system_clock::to_time_t(nowsys);
//  auto nowsysstructtm    = to_<struct tm>::_(nowsystmt);
//  nowsysstructtm.tm_hour = 23;
//  nowsysstructtm.tm_min  = 59;
//  nowsysstructtm.tm_sec  = 59;
//  nowsystmt              = to_<time_t>::_(nowsysstructtm);
//  nowsys                 = std::chrono::system_clock::from_time_t(nowsystmt);
//  auto now_midnight      = std__chrono::utc_clock::from_sys(nowsys);
//  return now_midnight;
//}
