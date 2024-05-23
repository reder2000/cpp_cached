#include "disk.h"
#include <algorithm>
//#include <fmt/chrono.h>
#include <cpp_rutils/literals.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

using path = std::filesystem::path;

using mapped_elem  = std::string;
using mapped_table = mapped_elem[255];

const mapped_table& get_mapped_table()
{
  static mapped_table res;
  static int          is_init = 0;
  if (! is_init)
  {
    is_init = 1;
    for (unsigned char c = 'a'; c <= 'z'; c++)
      res[c] = c;
    for (unsigned char c = 'A'; c <= 'Z'; c++)
      res[c] = c;
    for (unsigned char c = '0'; c <= '9'; c++)
      res[c] = c;
    res['_'_uchar] = "_";
    res['-'_uchar] = "-";
    res[' '_uchar] = "~~~";
    if constexpr (std::filesystem::path::preferred_separator == '\\')
    {
      res['\\'_uchar] = "\\";
      res['/'_uchar]  = "~0~";
    }
    else
    {
      res['\\'_uchar] = "~0~";
      res['/'_uchar]  = "/";
    }
    res['~'_uchar] = "~1~";
    res['?'_uchar] = "~2~";
    res['='_uchar] = "~3~";
    res['&'_uchar] = "~4~";
    res['['_uchar] = "~5~";
    res[']'_uchar] = "~6~";
    res['"'_uchar] = "~7~";
    res[','_uchar] = "~8~";
    res[':'_uchar] = "~9~";
    res['.'_uchar] = "~a~";
  }
  return res;
}


std::string to_conformant(const std::string& key)
{
  auto& mt = get_mapped_table();
  auto  bad_char =
      std::find_if(key.begin(), key.end(), [&](const unsigned char& c) { return mt[c].empty(); });
  MREQUIRE(bad_char == key.end(), "invalid input char {}", *bad_char);
  std::string res;
  for (unsigned char c : key)
    res += mt[c];
  return res;
}

std::string split_long_filename(const std::string& key)
{
  size_t      first;
  size_t      last = std::min<size_t>(key.size(), 240);
  std::string res(key.begin(), key.begin() + last);
  for (; last < key.size();)
  {
    res += std::filesystem::path::preferred_separator;
    first = last;
    last  = std::min<size_t>(key.size(), last + 240);
    res += std::string(key.begin() + first, key.begin() + last);
  }
  return res;
}

path get_full_path(const path& root_path, const std::string& key)
{
  path res       = root_path;
  auto re_key    = to_conformant(key);
  auto split_key = split_long_filename(re_key);
  res.append(split_key);
  return res;
}


DiskCache::DiskCache(path root_path) : _root_path(std::move(root_path))
{
  time_t tt;
  time(&tt);
  struct tm tm = m_localtime_s(tt);
  // auto      s  = std__format("{:%Y-%m-%d}", tm);
  auto      s  = std__format("{:04d}-{:02d}-{:02d}", tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);
  _root_path.append(s);
}

bool DiskCache::has(const std::string& key) const
{
  auto fp  = get_full_path_split(key);
  bool res = std::filesystem::exists(fp);
  return res;
}

void DiskCache::erase(const std::string& key)
{
  auto fp = get_full_path_split(key);
  if (exists(fp))
  {
    std::filesystem::remove(fp);
  }
}

void DiskCache::erase_symbol(const std::string_view /*symbol*/)
{
  MFAIL("erase_symbol is not implemented");
}

std::filesystem::path DiskCache::get_full_path_split(const std::string& key) const
{
  auto res = ::get_full_path(_root_path, key);
  return res;
}
