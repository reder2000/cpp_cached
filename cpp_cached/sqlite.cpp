#include "sqlite.h"

using namespace  SQLite;

std::string machine_name()
{
#if _MSC_VER
  #if _DEBUG
  return "MSC_DEBUG_";
  #else
  return "MSC_RELEASE_";
  #endif
#else
#if _DEBUG
  return "XXX_DEBUG_";
#else
  return "XXX_RELEASE_";
#endif
#endif

}

SqliteCache::SqliteCache(const std::filesystem::path& root_path, uint64_t max_size) :
	_root_path(root_path), _max_size(max_size)
{
  auto rerroot = _root_path / machine_name();
	_db_name =  rerroot / "cache.db";
  if (! std::filesystem::exists(rerroot))
          MREQUIRE(std::filesystem::create_directories(rerroot));
	auto db = get_db(true);
}

Database SqliteCache::get_db(bool create_tables)
{

	Database db(_db_name.string(), OPEN_READWRITE | OPEN_CREATE);

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

bool SqliteCache::has(const std::string& key)
{
	auto db = get_db();
	Statement query(db, "SELECT key FROM cache WHERE key = ?");
	query.bind(1, key);
	return query.executeStep() != 0 ;

}

void SqliteCache::erase(const std::string& key)
{
	if (!has(key)) return;
	auto db = get_db();
	Statement query(db, "DELETE FROM cache where key=?");
	query.bind(1, key);
	query.exec();
}

size_t SqliteCache::total_size(Database& db) const
{
	Statement   query(db, "SELECT SUM(size) FROM cache");
	query.executeStep();
	size_t res = query.getColumn(0).getInt();
	return res;
}

void SqliteCache::clean_expired(Database& db)
{
	int64_t cnow = std__chrono::utc_clock::now().time_since_epoch().count();

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

	Statement query(db, "SELECT key,sIzE FROM cache ORDER BY store_time");
	int64_t fred = 0;
	while (query.executeStep() && fred < need_to_free)
	{
		auto key = query.getColumn(0).getString();
		auto sz = query.getColumn(1).getInt64();
		fred += sz;
		Statement query2(db, "DELETE FROM cache where key=?");
		query2.bind(1, key);
		query2.exec();
	}
}

SqliteCache::time_point last_point_of_today()
{
  auto now               = std__chrono::utc_clock::now();
  auto nowsys            = std__chrono::utc_clock::to_sys(now);
  auto nowsystmt         = std::chrono::system_clock::to_time_t(nowsys);
  auto nowsysstructtm    = to_<struct tm>::_(nowsystmt);
  nowsysstructtm.tm_hour = 23;
  nowsysstructtm.tm_min  = 59;
  nowsysstructtm.tm_sec  = 59;
  nowsystmt              = to_<time_t>::_(nowsysstructtm);
  nowsys                 = std::chrono::system_clock::from_time_t(nowsystmt);
  auto now_midnight          = std__chrono::utc_clock::from_sys(nowsys);
  return now_midnight;
}
