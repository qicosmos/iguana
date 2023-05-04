/*
Copyright (c) 2018 Erik Rigtorp <erik@rigtorp.se>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace rigtorp {

struct to_chars_result {
  char *ptr;
  std::errc ec;
};

struct from_chars_result {
  const char *ptr;
  std::errc ec;
};

namespace detail {

static constexpr uint32_t powers_of_10_32[] = {
    UINT32_C(0),          UINT32_C(10),       UINT32_C(100),
    UINT32_C(1000),       UINT32_C(10000),    UINT32_C(100000),
    UINT32_C(1000000),    UINT32_C(10000000), UINT32_C(100000000),
    UINT32_C(1000000000),
};

static constexpr uint64_t powers_of_10_64[] = {
    UINT64_C(0),
    UINT64_C(10),
    UINT64_C(100),
    UINT64_C(1000),
    UINT64_C(10000),
    UINT64_C(100000),
    UINT64_C(1000000),
    UINT64_C(10000000),
    UINT64_C(100000000),
    UINT64_C(1000000000),
    UINT64_C(10000000000),
    UINT64_C(100000000000),
    UINT64_C(1000000000000),
    UINT64_C(10000000000000),
    UINT64_C(100000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(10000000000000000),
    UINT64_C(100000000000000000),
    UINT64_C(1000000000000000000),
    UINT64_C(10000000000000000000),
};

template <typename T> constexpr unsigned to_chars_len(T value) noexcept {
  static_assert(std::is_unsigned<T>::value);
  static_assert(std::numeric_limits<unsigned>::max() >=
                std::numeric_limits<T>::digits);
  if constexpr (sizeof(T) <= sizeof(int)) {
    static_assert(sizeof(int) == 4);
    const unsigned t = (32 - __builtin_clz(value | 1)) * 1233 >> 12;
    return t - (value < powers_of_10_32[t]) + 1;
  } else {
    static_assert(sizeof(T) <= sizeof(long long));
    static_assert(sizeof(long long) == 8);
    const unsigned t = (64 - __builtin_clzll(value | 1)) * 1233 >> 12;
    return t - (value < powers_of_10_64[t]) + 1;
  }
}

template <typename T>
constexpr to_chars_result to_chars(char *first, char *last, T value) noexcept {
  static_assert(std::is_integral<T>::value);
  using UT = std::make_unsigned_t<T>;
  static_assert(sizeof(UT) == sizeof(T));
  UT uvalue = value;
  if constexpr (std::is_signed<T>::value) {
    if (value < 0) {
      if (__builtin_expect(first != last, 1)) {
        *first++ = '-';
        uvalue = UT(~value) + UT(1);
      }
    }
  }
  const auto len = to_chars_len(uvalue);
  if (__builtin_expect(last - first < len, 0)) {
    return {last, std::errc::value_too_large};
  }
  uint32_t pos = len - 1;
  while (uvalue >= 10) {
    const auto q = uvalue / 10;
    const auto r = uvalue % 10;
    first[pos--] = r + '0';
    uvalue = q;
  }
  first[0] = uvalue + '0';
  return {first + len, {}};
}

template <typename T>
constexpr from_chars_result from_chars(const char *first, const char *last,
                                       T &value) noexcept {
  static_assert(std::is_integral<T>::value);
  [[maybe_unused]] int sign = 1;
  if constexpr (std::is_signed<T>::value) {
    if (first != last && *first == '-') {
      sign = -1;
      ++first;
    }
  }
  if (__builtin_expect(first == last, 0)) {
    return {first, std::errc::invalid_argument};
  }
  std::make_unsigned_t<T> res = 0;
  static_assert(sizeof(res) == sizeof(T));
  while (first != last) {
    const uint8_t c = *first - '0';
    if (__builtin_expect(c > 9, 0)) {
      return {first, std::errc::invalid_argument};
    }
    if (__builtin_expect(__builtin_mul_overflow(res, 10, &res), 0) ||
        __builtin_expect(__builtin_add_overflow(res, c, &res), 0)) {
      return {first, std::errc::result_out_of_range};
    }
    ++first;
  }
  if constexpr (std::is_signed<T>::value) {
    T tmp;
    if (__builtin_expect(__builtin_mul_overflow(res, sign, &tmp), 0)) {
      return {first, std::errc::result_out_of_range};
    }
    value = tmp;
  } else {
    value = res;
  }
  return {first, {}};
}
} // namespace detail

constexpr inline to_chars_result to_chars(char *first, char *last,
                                          uint32_t value) noexcept {
  return detail::to_chars(first, last, value);
}

constexpr inline to_chars_result to_chars(char *first, char *last,
                                          int32_t value) noexcept {
  return detail::to_chars(first, last, value);
}

constexpr inline to_chars_result to_chars(char *first, char *last,
                                          uint64_t value) noexcept {
  return detail::to_chars(first, last, value);
}

constexpr inline to_chars_result to_chars(char *first, char *last,
                                          int64_t value) noexcept {
  return detail::to_chars(first, last, value);
}

constexpr inline from_chars_result
from_chars(const char *first, const char *last, uint32_t &value) noexcept {
  return detail::from_chars(first, last, value);
}

constexpr inline from_chars_result
from_chars(const char *first, const char *last, int32_t &value) noexcept {
  return detail::from_chars(first, last, value);
}

constexpr inline from_chars_result
from_chars(const char *first, const char *last, uint64_t &value) noexcept {
  return detail::from_chars(first, last, value);
}

constexpr inline from_chars_result
from_chars(const char *first, const char *last, int64_t &value) noexcept {
  return detail::from_chars(first, last, value);
}
} // namespace rigtorp