// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include <bit>
#include <string_view>

#include "define.h"
#include "error_code.h"

namespace iguana {
template <size_t N> struct string_literal {
  static constexpr size_t size = (N > 0) ? (N - 1) : 0;

  constexpr string_literal() = default;

  constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
  constexpr const char *end() const noexcept { return value + size; }

  constexpr const std::string_view sv() const noexcept { return {value, size}; }
};

template <char c> IGUANA_INLINE errc match(auto &&it, auto &&end) noexcept {
  if (it == end || *it != c) [[unlikely]] {
    return errc::not_match_specific_chars;
  } else [[likely]] {
    ++it;
  }

  return errc::ok;
}

template <string_literal str>
IGUANA_INLINE errc match(auto &&it, auto &&end) noexcept {
  const auto n = static_cast<size_t>(std::distance(it, end));
  if (n < str.size) [[unlikely]] {
    return errc::unexpected_end;
  }
  size_t i{};
  // clang and gcc will vectorize this loop
  for (auto *c = str.value; c < str.end(); ++it, ++c) {
    i += *it != *c;
  }
  if (i != 0) [[unlikely]] {
    return errc::not_match_specific_chars;
  }

  return errc::ok;
}

IGUANA_INLINE errc skip_comment(auto &&it, auto &&end) noexcept {
  ++it;
  if (it == end) [[unlikely]]
    return errc::unexpected_end;
  else if (*it == '/') {
    while (++it != end && *it != '\n')
      ;
  } else if (*it == '*') {
    while (++it != end) {
      if (*it == '*') [[unlikely]] {
        if (++it == end) [[unlikely]]
          break;
        else if (*it == '/') [[likely]] {
          ++it;
          break;
        }
      }
    }
  } else [[unlikely]] {
    return errc::lack_of_backslash;
  }
  return errc::ok;
}

IGUANA_INLINE errc skip_ws(auto &&it, auto &&end) noexcept {
  while (it != end) {
    // assuming ascii
    if (static_cast<uint8_t>(*it) < 33) {
      ++it;
    } else if (*it == '/') {
      auto ec = skip_comment(it, end);
      if (ec != errc::ok) {
        return ec;
      }
    } else {
      break;
    }
  }

  return errc::ok;
}

IGUANA_INLINE errc skip_till_escape_or_qoute(auto &&it, auto &&end) noexcept {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

  auto has_zero = [](uint64_t chunk) {
    return (((chunk - 0x0101010101010101) & ~chunk) & 0x8080808080808080);
  };

  auto has_qoute = [&](uint64_t chunk) IGUANA__INLINE_LAMBDA {
    return has_zero(
        chunk ^
        0b0010001000100010001000100010001000100010001000100010001000100010);
  };

  auto has_escape = [&](uint64_t chunk) IGUANA__INLINE_LAMBDA {
    return has_zero(
        chunk ^
        0b0101110001011100010111000101110001011100010111000101110001011100);
  };

  if (std::distance(it, end) >= 7) [[likely]] {
    const auto end_m7 = end - 7;
    for (; it < end_m7; it += 8) {
      const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
      uint64_t test = has_qoute(chunk) | has_escape(chunk);
      if (test != 0) {
        it += (std::countr_zero(test) >> 3);
        return errc::ok;
      }
    }
  }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    switch (*it) {
    case '\\':
    case '"':
      return errc::ok;
    }
    ++it;
  }

  return errc::lack_of_quote;
}

IGUANA_INLINE constexpr bool is_numeric(const auto c) noexcept {
  switch (c) {
  case '0':
  case '1':
  case '2':
  case '3': //
  case '4':
  case '5':
  case '6':
  case '7': //
  case '8':
  case '9': //
  case '.':
  case '+':
  case '-': //
  case 'e':
  case 'E': //
    return true;
  }
  return false;
}

} // namespace iguana
