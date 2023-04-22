#pragma once
#if defined(PREFERED_SERIALIZATION_cereal)
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
using serialization_out_type = std::string;
#elif defined(PREFERED_SERIALIZATION_zpp_bits)
#include <zpp_bits.h>
using serialization_out_type = std::vector<std::byte>;
#else
#error("no serialization scheme defined")
#endif
#include <string>
#include <cpp_rutils/require.h>

namespace cpp_cached_serialization
{
  template <class T>
  std::string set_value(const T& t)
  {
#if defined(PREFERED_SERIALIZATION_cereal)
    std::ostringstream          out;
    cereal::BinaryOutputArchive archive(out);
    //cereal::JSONOutputArchive archive(out);
    archive(t);
    return out.str();
#elif defined(PREFERED_SERIALIZATION_zpp_bits)
    std::string data;
    auto        out = zpp::bits::out(data);
    out(t).or_throw();
    return data;
#endif
  }

  template <class T>
  T get_value(const std::string& value)
  {
    T t;
#if defined(PREFERED_SERIALIZATION_cereal)
    std::ostringstream          out;
    cereal::BinaryOutputArchive archive(out);
    //cereal::JSONOutputArchive archive(out);
    archive(t);
    return out.str();
#elif defined(PREFERED_SERIALIZATION_zpp_bits)


    auto s = std::span{value.begin(), value.size()};
    //std::vector<std::byte> data;
    auto in = zpp::bits::in(s);
    in(t).or_throw();
    return t;
#endif
  }

}  // namespace cpp_cached_serialization