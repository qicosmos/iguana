#pragma once
#include <string_view>

namespace iguana {
template <typename T> constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <auto T> constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <typename T> inline constexpr std::string_view type_string() {
  constexpr auto sample = get_raw_name<int>();
  constexpr size_t pos = sample.find("int");
  constexpr auto str = get_raw_name<T>();
  constexpr auto next1 = str.find(sample[pos + 3]);
  constexpr auto s1 = str.substr(pos, next1 - pos);
  return s1;
}

template <auto T> inline constexpr std::string_view enum_string() {
  auto sample = get_raw_name<int>();
  size_t pos = sample.find("int");
  auto str = get_raw_name<T>();
  auto next1 = str.find(sample[pos + 3]);
#if defined(__clang__)
  auto s1 = str.substr(pos, next1 - pos);
#else
  auto s1 = str.substr(pos + 5, next1 - pos - 5);
#endif
  return s1;
}
} // namespace iguana