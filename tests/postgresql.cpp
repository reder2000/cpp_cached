#include "../cpp_cached/postgresql.h"

#define CATCH_CONFIG_ALL_PARTS
#include <catch2/catch_test_macros.hpp>
#include <cereal/types/vector.hpp>


TEST_CASE("postgresql", "[cache][hide]")
{
  try
  {
    pqxx::connection c(std::vector<std::pair<std::string, std::string>>{{"dbname", "cpp_cached"},
                                                                        {"password", "qustrat"},
                                                                        {"user", "postgres"},
                                                                        {"port", "32769"}});
    pqxx::work       w(c);
    pqxx::row        r = w.exec1("SELECT 1");
    w.commit();
    CHECK(r[0].as<int>() == 1);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

TEST_CASE("postgresql2", "[cache][hide]")
{
  const char* prevpass = std::getenv("PGPASSWORD");
  auto        fail     = []()
  {
    _putenv_s("PGPASSWORD", "qkflqkj");
    PostgresCache pc;
  };
  CHECK_THROWS(fail());
  if (prevpass != nullptr)
    _putenv_s("PGPASSWORD", prevpass);
  else
  {
    std::string remove = std::string("PGPASSWORD") + "=";
    _putenv(remove.c_str());
  }
  PostgresCache pc;
  std::string   key = "unit_test_key";
  pc.erase(key);
  CHECK(! pc.has(key));
  pc.set(key, 3.14159);
  CHECK_THROWS(pc.set(key, 3.14159));
  CHECK(pc.has(key));
  CHECK(fabs(pc.get<double>(key) - 3.14159) < 1e-8);
  pc.erase(key);
  CHECK(! pc.has(key));
}