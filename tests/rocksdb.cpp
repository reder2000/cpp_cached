#include "../cpp_cached/rocksdb.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("rocksdb", "[cache][hide]")
{
  RocksDbCache pc;
  std::string  key = "unit_test_key";
  pc.really_erase(key);
  pc.really_erase("meta_" + key);
  CHECK(! pc.has(key));
  pc.set(key, 3.14159);
  CHECK_THROWS(pc.set(key, 3.14159));
  CHECK(pc.has(key));
  CHECK(fabs(pc.get<double>(key) - 3.14159) < 1e-8);
  pc.erase(key);
  CHECK(! pc.has(key));
  pc.really_erase("symbol_test_key");
  pc.really_erase("meta_symbol_test_key");
  pc.really_erase("symbol_test_key_2");
  pc.really_erase("meta_symbol_test_key_2");
  pc.set("symbol_test_key", 1., "test");
  pc.set("symbol_test_key_2", 2., "test");
  pc.erase_symbol("test");
  CHECK(! pc.has("symbol_test_key"));
}