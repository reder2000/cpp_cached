#pragma once

// try to abstract the serialization in one place

#if defined(PREFERED_SERIALIZATION_cereal)
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cpp_rutils/memstream.h>
//using serialization_out_type = std::string;
#elif defined(PREFERED_SERIALIZATION_zpp_bits)
#include <zpp_bits.h>
using serialization_out_type = std::vector<std::byte>;
//namespace zpp::bits
//{
//  template <class T>
//  constexpr auto serialize(auto& archive, std::shared_ptr<const T>& csp)
//  {
//    std::shared_ptr<T> sp;
//    auto               res = archive(sp);
//    csp                    = sp;
//    return res;
//  }
//}  // namespace zpp::bits
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
    std::string res = out.str();
    return res;
#elif defined(PREFERED_SERIALIZATION_zpp_bits)
    std::string data;
    auto        out = zpp::bits::out(data);
    out(t).or_throw();
    return data;
#endif
  }

#if defined(PREFERED_SERIALIZATION_cereal)
  template <class T>
  T get_value(const std::string& value)
  {
    T          res;
    imemstream sin(
        value.data(),
        value.size());  //reinterpret_cast<const char*>(col_blob.data()), col_blob.size());
    cereal::BinaryInputArchive archive(sin);
    archive(res);
    return res;
  }
#elif defined(PREFERED_SERIALIZATION_zpp_bits)
  //{
  //  template <class T>
  //  constexpr auto serialize(auto& archive, std::shared_ptr<const T>& csp)
  //  {
  //    std::shared_ptr<T> sp;
  //    auto               res = archive(sp);
  //    csp                    = sp;
  //    return res;
  //  }
  //}  // namespace zpp::bits

  template <class T>
  T get_value(const std::string& value)
  {
    T    t;
    auto s = std::span{value.begin(), value.size()};
    //std::vector<std::byte> data;
    auto in = zpp::bits::in(s);
    in(t).or_throw();
    return t;
  }  // namespace zpp::bits

  template <typename T>
  concept is_a_shared_ptr =
      requires(T) { std::is_same_v<std::shared_ptr<typename T::element_type>, T>; };

  template <typename T>
  concept is_a_shared_ptr_to_const =
      std::is_const_v<typename T::element_type> && is_a_shared_ptr<T>;

  //void fun()
  //{
  //  std::shared_ptr<double> sp;
  //  static_assert(! is_a_shared_ptr_to_const<decltype(sp)>);
  //  std::shared_ptr<const double> spd;
  //  static_assert(is_a_shared_ptr_to_const<decltype(spd)>);
  //}


  template <is_a_shared_ptr_to_const T>
  T get_value(const std::string& value)
  {
    //std::shared_ptr<int>       ptr = std::make_shared<int>(42);
    //std::shared_ptr<const int> constPtr(ptr);

    using U                 = std::remove_const_t<typename T::element_type>;
    std::shared_ptr<U> pres = get_value<std::shared_ptr<U>>(value);
    //T                  res  = std::const_pointer_cast<typename T::element_type>(pres);
    std::shared_ptr<const U> res(pres);
    return res;
  }

#endif

}  // namespace cpp_cached_serialization