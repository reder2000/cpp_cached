#include "rocksdb.h"

#include "is_a_cache.h"


std::filesystem::path RocksDbCache::default_root_path()
{
  return std::filesystem::temp_directory_path().append("cache");
}

RocksDbCache::RocksDbCache(std::filesystem::path   root_path,
                           uint64_t                max_size,
                           RocksdbCacheGranularity rocksdb_cache_granularity)
{
  _max_size                  = max_size;
  _rocksdb_cache_granularity = rocksdb_cache_granularity;
  auto db_file               = root_path / "rocksdb" / ROCKSDB_NAME "_rocksdb.db";
  if (! std::filesystem::is_directory(db_file.parent_path()))
    std::filesystem::create_directories(db_file.parent_path());

  rocksdb::DB*     db;
  rocksdb::Options options;
  options.create_if_missing = true;
  options.compression       = rocksdb::CompressionType::kZSTD;
  rocksdb::Status status    = rocksdb::DB::Open(options, db_file.string(), &db);
  MREQUIRE(status.ok(), "opening db failed {}", status.ToString());
  _connection.reset(db);
}

size_t to_duration(std__chrono::utc_clock::time_point tp,
                   RocksdbCacheGranularity            rocksdb_cache_granularity)
{
  size_t res;
  switch (rocksdb_cache_granularity)
  {
    case RocksdbCacheGranularity::minutes:
      res = std::chrono::time_point_cast<std::chrono::minutes>(tp).time_since_epoch().count();
      break;
    case RocksdbCacheGranularity::hours:
      res = std::chrono::time_point_cast<std::chrono::hours>(tp).time_since_epoch().count();
      break;
    case RocksdbCacheGranularity::days:
      res = std::chrono::time_point_cast<std::chrono::days>(tp).time_since_epoch().count();
      break;
  }
  return res;
}

bool RocksDbCache::has(const std::string& key)
{
  std::string what_for;
  if (! _connection->KeyMayExist(rocksdb::ReadOptions{}, key, &what_for)) return false;
  std::string value;
  if (! _connection->KeyMayExist(rocksdb::ReadOptions{}, "meta_" + key, &what_for))
  {  // bad state: meta but no key
    really_erase(key);
    return false;
  }
  rocksdb::Status status =
      _connection->Get(rocksdb::ReadOptions{}, std::string("meta_") + key, &value);
  if (! status.ok()) return false;
  //MREQUIRE(status.ok(), "reading meta key {} failed {}", key, status.ToString());
  auto                               meta  = get_value<RocksdbValueMetaData>(value);
  std__chrono::utc_clock::time_point now   = std__chrono::utc_clock::now();
  auto                               limit = to_duration(now, _rocksdb_cache_granularity);
  if (limit > meta._duration)
  {
    really_erase("meta_" + key);
    really_erase(key);
    return false;
  }
  return true;
}

void RocksDbCache::erase(const std::string& key)
{
  if (! has(key)) return;
  //auto db = get_db(true);
  really_erase(key);
  really_erase("meta_" + key);
}

std::shared_ptr<RocksDbCache> RocksDbCache::get_default()
{
  static auto res = std::make_shared<RocksDbCache>();
  return res;
}

void RocksDbCache::really_erase(const std::string& key)
{
  _connection->Delete(rocksdb::WriteOptions{}, key);
  //_connection->Flush(rocksdb::FlushOptions{});
}

void RocksDbCache::erase_symbol(const std::string_view symbol)
{
  std::vector<std::string>           to_erase;
  std::unique_ptr<rocksdb::Iterator> it(_connection->NewIterator(rocksdb::ReadOptions{}));
  it->SeekToFirst();
  //it->Seek("meta_");
  while (it->Valid())
  {
    auto key = it->key().ToString();
    if (key.starts_with("meta_"))
    {
      auto meta = get_value<RocksdbValueMetaData>(it->value().ToString());
      if (meta._symbol == symbol)
      {
        key = key.substr(5);
        to_erase.push_back(key);
      }
    }
    it->Next();
  }
  for (auto const& k : to_erase)
    erase(k);
}
