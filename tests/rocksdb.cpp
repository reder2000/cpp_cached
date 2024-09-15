#include "../cpp_cached/rocksdb.h"

#define CATCH_CONFIG_ALL_PARTS
#include <gtest/gtest.h>
#if defined(PREFERED_SERIALIZATION_cereal)
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#endif

TEST(cpp_cached_tests,rocksdb)
{
  RocksDbCache pc;
  std::string  key = "unit_test_key";
  pc.really_erase(key);
  pc.really_erase("meta_" + key);
  EXPECT_TRUE(! pc.has(key));
  pc.set(key, 3.14159);
  EXPECT_THROW(pc.set(key, 3.14159),std::exception);
  EXPECT_TRUE(pc.has(key));
  EXPECT_TRUE(fabs(pc.get<double>(key) - 3.14159) < 1e-8);
  pc.erase(key);
  EXPECT_TRUE(! pc.has(key));
  pc.really_erase("symbol_test_key");
  pc.really_erase("meta_symbol_test_key");
  pc.really_erase("symbol_test_key_2");
  pc.really_erase("meta_symbol_test_key_2");
  pc.set("symbol_test_key", 1., "test");
  pc.set("symbol_test_key_2", 2., "test");
  pc.erase_symbol("test");
  EXPECT_TRUE(! pc.has("symbol_test_key"));
  key = "unit_test_key_sp";
  pc.really_erase(key);
  pc.really_erase("meta_" + key);
  std::shared_ptr<const double> psp = std::make_shared<double>(3.14159);
  pc.set(key, psp);
  EXPECT_THROW(pc.set(key, psp),std::exception);
  EXPECT_TRUE(pc.has(key));
  EXPECT_TRUE(fabs(*pc.get<std::shared_ptr<const double>>(key) - 3.14159) < 1e-8);
  pc.erase(key);
  EXPECT_TRUE(! pc.has(key));
}