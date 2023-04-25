#pragma once
#include <string>

// "metadata" for db-stored objects
struct cache_row_value
{
  std::string key;
  int64_t     store_time;
  int64_t     expire_time;
  int64_t     access_time;
  int64_t     accesss_count;
  int64_t     size;
};

std::string machine_name();