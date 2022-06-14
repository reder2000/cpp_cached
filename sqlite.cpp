#include "sqlite.h"


using namespace  SQLite;

SqliteCache::SqliteCache(const std::filesystem::path& root_path, uint64_t max_size) :
	_root_path(root_path), _max_size(max_size)
{
	_db_name = _root_path / "cache.db";
	if (!std::filesystem::exists(_root_path))
		MREQUIRE(std::filesystem::create_directories(_root_path));
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

bool SqliteCache::contains(const char* key)
{
	auto db = get_db();
	Statement query(db, "SELECT key FROM cache WHERE key = ?");
	query.bind(1, key);
	return query.exec() != 0 ;

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
	int64_t cnow = std::chrono::utc_clock::now().time_since_epoch().count();

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
