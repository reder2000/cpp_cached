#pragma once

// disk cache that writes each cache entry as a file on disk
// needs long file name support

#include "cache_imp_exp.h"
#include <cpp_rutils/require.h>

#include <filesystem>
#include <fstream>
#include <cpp_rutils/secure_deprecate.h>

#include "is_a_cache.h"
#include "serialization.h"

class cpp_cached_API DiskCache
{

 public:
  template <class T>
  using return_type = std::decay_t<T>;

  DiskCache(
      std::filesystem::path root_path = std::filesystem::temp_directory_path().append("cache"));

  template <class T>
  void set(const std::string& key, const T& value, std::string_view = {});

  [[nodiscard]] bool has(const std::string& key) const;

  template <class T>
  T get(const std::string& key) const;

  //template <class F>
  //const std::decay_t<std::invoke_result_t<F>>& get(const std::string& key, F callback);

  void erase(const std::string& key);

  void erase_symbol(const std::string_view symbol);

  static std::shared_ptr<DiskCache> get_default();

 private:
  std::filesystem::path _root_path;

  [[nodiscard]] std::filesystem::path get_full_path_splitted(const std::string& key) const;
};

static_assert(is_a_cache<DiskCache>);


template <class T>
void DiskCache::set(const std::string& key, const T& value, std::string_view)
{
  auto fn = get_full_path_splitted(key);
  auto fp = fn;
  create_directories(fp.remove_filename());
  std::ofstream fout;
  // Set exceptions to be thrown on failure
  fout.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    fout.open(fn, std::ios::binary);
    fout << cpp_cached_serialization::set_value(value);
  }
  catch (std::system_error& e)
  {
    std::cerr << m_strerror_s(errno) << std::endl;
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
  return res;
}
