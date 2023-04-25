#pragma once

#include "cache_imp_exp.h"
#include <unordered_map>
#include <cpp_rutils/name.h>
#include <string>

extern cpp_cached_API std::unordered_map<std::string, void*> unique_type_storage;

template <class T, class... Args>
T* unique_type_new(Args... args)
{
  std::string name{type_name<T>()};
  if (unique_type_storage.find(name) != unique_type_storage.end())
    return reinterpret_cast<T*>(unique_type_storage[name]);
  else
  {
    T* value                  = new T{std::forward<Args>(args)...};
    unique_type_storage[name] = value;
    return value;
  }
}

// never deletes
void inline unique_type_deleter(void*) {}