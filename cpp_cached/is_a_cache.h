#pragma once
#include <string>

template <typename T>
concept is_a_cache =
    requires(T a) { a.has(std::string{}); } &&
    requires(T a) { a.template get<int>(std::string{}); } &&
    requires(T a) { a.erase(std::string{}); } &&
    requires(T a) { a.set(std::string{}, int{}, std::string_view{}); } &&
    requires(T a) { a.get(std::string{}, []() -> int { return 0; }); } &&
    requires(T a) { a.erase_symbol(std::string_view{}); } && requires(T) { T::get_default(); };
