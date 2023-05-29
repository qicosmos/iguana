#pragma once
#include "dragonbox_to_chars.h"
#include "fast_float.h"
#include "itoa.hpp"

namespace iguana {
template <typename T>
struct is_char_type
    : std::disjunction<std::is_same<T, char>, std::is_same<T, unsigned char>,
                       std::is_same<T, signed char>, std::is_same<T, wchar_t>,
                       std::is_same<T, char16_t>, std::is_same<T, char32_t>> {};

namespace detail {

template <typename T>
std::pair<const char *, std::errc>
from_chars(const char *first, const char *last, T &value) noexcept {

  double num;
  auto [p, ec] = fast_float::from_chars(first, last, num);
  value = static_cast<std::decay_t<T>>(num);
  return {p, ec};
}

// not support uint8 for now
template <typename T> char *to_chars(char *buffer, T value) noexcept {
  using U = std::decay_t<T>;
  if constexpr (std::is_floating_point_v<U>) {
    return jkj::dragonbox::to_chars(value, buffer);
  } else if constexpr (std::is_signed_v<U> && (sizeof(U) >= 8)) {
    return xtoa(value, buffer, 10, 1); // int64_t
  } else if constexpr (std::is_unsigned_v<U> && (sizeof(U) >= 8)) {
    return xtoa(value, buffer, 10, 0); // uint64_t
  } else if constexpr (std::is_integral_v<U> && !is_char_type<U>::value) {
    return itoa_fwd(value, buffer); // only support more than 2 bytes intergal
  } else {
    static_assert(!sizeof(U), "only support arithmetic type except char type");
  }
}

} // namespace detail
} // namespace iguana
