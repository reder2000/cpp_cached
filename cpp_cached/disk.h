#pragma once

// disk cache that writes each cache entry as a file on disk
// needs long file name support

#include "cache_imp_exp.h"
#include <unordered_map>
#include <any>
#include <list>
#include <cpp_rutils/require.h>

#include <filesystem>
#include <fstream>
#include <cpp_rutils/secure_deprecate.h>
#include "serialization.h"

class cpp_cached_API DiskCache
{

 public:
  DiskCache(
      std::filesystem::path root_path = std::filesystem::temp_directory_path().append("cache"));

  template <class T>
  void push(const std::string& key, const T& value);

  [[nodiscard]] bool has(const std::string& key) const;

  template <class T>
  T get(const std::string& key) const;

 private:
  std::filesystem::path _root_path;

  [[nodiscard]] std::filesystem::path get_full_path_splitted(const std::string& key) const;
};

template <class T>
void DiskCache::push(const std::string& key, const T& value)
{
  auto fn = get_full_path_splitted(key);
  auto fp = fn;
  std::filesystem::create_directories(fp.remove_filename());
  std::ofstream fout;
  // Set exceptions to be thrown on failure
  fout.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    fout.open(fn, std::ios::binary);
    //if ((fout.rdstate() & std::ofstream::failbit) != 0)
    //	std::cerr << "Error opening " << fn << "\n";
    //cereal::BinaryOutputArchive archive(fout);
    //    archive(value);
    fout << cpp_cached_serialization::set_value(value);
  }
  catch (std::system_error& e)
  {
    std::cerr << m_strerror_s(errno) << std::endl;
    //std::cerr << e.code().message() << std::endl;
    throw;
  }
}

template <class T>
T DiskCache::get(const std::string& key) const
{
  auto              fp = get_full_path_splitted(key);
  std::ifstream     fin(fp, std::ios::binary);
  std::stringstream buffer;
  buffer << fin.rdbuf();
  T res = cpp_cached_serialization::get_value<T>(buffer.str());
  //cereal::BinaryInputArchive archive(fout);
  //T                          res;
  //archive(res);
  return res;
}
