// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include "define.h"
#include "reflection.hpp"
#include "value.hpp"
#include <bit>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <math.h>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if __has_cpp_attribute(likely) && __has_cpp_attribute(unlikely)
#define AS_LIKELY [[likely]]
#define AS_UNLIKELY [[unlikely]]
#else
#define AS_LIKELY
#define AS_UNLIKELY
#endif

namespace iguana {

template <typename T>
inline constexpr bool char_v = std::is_same_v<std::decay_t<T>, char> ||
                               std::is_same_v<std::decay_t<T>, char16_t> ||
                               std::is_same_v<std::decay_t<T>, char32_t> ||
                               std::is_same_v<std::decay_t<T>, wchar_t>;

template <typename T>
inline constexpr bool bool_v =
    std::is_same_v<std::decay_t<T>, bool> ||
    std::is_same_v<std::decay_t<T>, std::vector<bool>::reference>;

template <typename T>
inline constexpr bool int_v = std::is_integral_v<std::decay_t<T>> &&
                              !char_v<std::decay_t<T>> && !bool_v<T>;

template <typename T>
inline constexpr bool float_v = std::is_floating_point_v<std::decay_t<T>>;

template <typename T> inline constexpr bool num_v = float_v<T> || int_v<T>;

template <typename T>
inline constexpr bool enum_v = std::is_enum_v<std::decay_t<T>>;

template <typename T>
constexpr inline bool optional_v =
    is_template_instant_of<std::optional, std::remove_cvref_t<T>>::value;

template <class, class = void> struct is_container : std::false_type {};

template <class T>
struct is_container<
    T, std::void_t<decltype(std::declval<T>().size(), std::declval<T>().begin(),
                            std::declval<T>().end())>> : std::true_type {};

template <class, class = void> struct is_map_container : std::false_type {};

template <class T>
struct is_map_container<
    T, std::void_t<decltype(std::declval<typename T::mapped_type>())>>
    : is_container<T> {};

template <typename T>
constexpr inline bool container_v = is_container<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool map_container_v =
    is_map_container<std::remove_cvref_t<T>>::value;

template <class T>
constexpr inline bool c_array_v = std::is_array_v<std::remove_cvref_t<T>> &&
                                      std::extent_v<std::remove_cvref_t<T>> > 0;

template <typename Type, typename = void> struct is_array : std::false_type {};

template <typename T>
struct is_array<
    T, std::void_t<decltype(std::declval<T>().size()),
                   typename std::enable_if_t<(std::tuple_size<T>::value != 0)>>>
    : std::true_type {};

// template <class, class = void> struct is_array : std::false_type {};

// template <typename T>
// struct is_array<T, std::void_t<decltype(std::declval<T>().size())>>
//     : std::tuple_size<std::remove_cv_t<T>> {};

template <typename T>
constexpr inline bool array_v = is_array<std::remove_cvref_t<T>>::value;

template <typename Type>
constexpr inline bool fixed_array_v = c_array_v<Type> || array_v<Type>;

template <typename T>
constexpr inline bool string_view_v =
    is_template_instant_of<std::basic_string_view,
                           std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool string_v =
    is_template_instant_of<std::basic_string, std::remove_cvref_t<T>>::value;

// template <typename Type>
// concept json_view = requires(Type container) {
//   container.size();
//   container.begin();
//   container.end();
// };
// TODO: type must be char
template <typename Type>
constexpr inline bool json_view_v = is_container<Type>::value;

template <typename T>
constexpr inline bool json_byte_v =
    std::is_same_v<char, T> || std::is_same_v<unsigned char, T> ||
    std::is_same_v<std::byte, T>;

template <typename T>
constexpr inline bool sequence_container_v =
    is_sequence_container<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool tuple_v = is_tuple<std::remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool string_container_v = string_v<T> || string_view_v<T>;

// template <typename Type>
// concept unique_ptr_t = requires(Type ptr) {
//   ptr.operator*();
//   typename std::remove_cvref_t<Type>::element_type;
// }
// &&!requires(Type ptr, Type ptr2) { ptr = ptr2; };
template <typename T>
constexpr inline bool unique_ptr_v =
    is_template_instant_of<std::unique_ptr, std::remove_cvref_t<T>>::value;

template <class T>
constexpr inline bool non_refletable_v =
    container_v<T> || c_array_v<T> || tuple_v<T> || optional_v<T> ||
    unique_ptr_v<T> || std::is_fundamental_v<T>;

template <typename T>
constexpr inline bool refletable_v = is_reflection_v<std::remove_cvref_t<T>>;

class numeric_str {
public:
  std::string_view &value() { return val_; }
  std::string_view value() const { return val_; }
  template <typename T> T convert() {
    static_assert(num_t<T>, "T must be numeric type");
    if (val_.empty()) [[unlikely]] {
      throw std::runtime_error("Failed to parse number");
    }
    T res;
    auto [_, ec] =
        detail::from_chars(val_.data(), val_.data() + val_.size(), res);
    if (ec != std::errc{}) [[unlikely]] {
      throw std::runtime_error("Failed to parse number");
    }
    return res;
  }

private:
  std::string_view val_;
};

template <typename T>
concept numeric_str_v = std::is_same_v<numeric_str, std::remove_cvref_t<T>>;

template <size_t N> struct string_literal {
  static constexpr size_t size = (N > 0) ? (N - 1) : 0;

  constexpr string_literal() = default;

  constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
  constexpr const char *end() const noexcept { return value + size; }

  constexpr const std::string_view sv() const noexcept { return {value, size}; }
};

template <char c> IGUANA_INLINE void match(auto &&it, auto &&end) {
  if (it == end || *it != c)
    AS_UNLIKELY {
      static constexpr char b[] = {c, '\0'};
      //         static constexpr auto error = concat_arrays("Expected:", b);
      std::string error = std::string("Expected:").append(b);
      throw std::runtime_error(error);
    }
  else
    AS_LIKELY { ++it; }
}

template <string_literal str> IGUANA_INLINE void match(auto &&it, auto &&end) {
  const auto n = static_cast<size_t>(std::distance(it, end));
  if (n < str.size)
    AS_UNLIKELY {
      // TODO: compile time generate this message, currently borken with
      // MSVC
      static constexpr auto error = "Unexpected end of buffer. Expected:";
      throw std::runtime_error(error);
    }
  size_t i{};
  // clang and gcc will vectorize this loop
  for (auto *c = str.value; c < str.end(); ++it, ++c) {
    i += *it != *c;
  }
  if (i != 0)
    AS_UNLIKELY {
      // TODO: compile time generate this message, currently borken with
      // MSVC
      static constexpr auto error = "Expected: ";
      throw std::runtime_error(error);
    }
}

IGUANA_INLINE void skip_comment(auto &&it, auto &&end) {
  ++it;
  if (it == end)
    AS_UNLIKELY
  throw std::runtime_error("Unexpected end, expected comment");
  else if (*it == '/') {
    while (++it != end && *it != '\n')
      ;
  }
  else if (*it == '*') {
    while (++it != end) {
      if (*it == '*')
        AS_UNLIKELY {
          if (++it == end)
            AS_UNLIKELY
          break;
          else if (*it == '/') AS_LIKELY {
            ++it;
            break;
          }
        }
    }
  }
  else AS_UNLIKELY throw std::runtime_error("Expected / or * after /");
}

IGUANA_INLINE void skip_ws(auto &&it, auto &&end) {
  while (it != end) {
    // assuming ascii
    if (static_cast<uint8_t>(*it) < 33) {
      ++it;
    } else if (*it == '/') {
      skip_comment(it, end);
    } else {
      break;
    }
  }
}

IGUANA_INLINE void skip_ws_no_comments(auto &&it, auto &&end) {
  while (it != end) {
    // assuming ascii
    if (static_cast<uint8_t>(*it) < 33)
      AS_LIKELY { ++it; }
    else {
      break;
    }
  }
}

inline constexpr auto has_zero = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return (((chunk - 0x0101010101010101) & ~chunk) & 0x8080808080808080);
};

inline constexpr auto has_qoute = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0010001000100010001000100010001000100010001000100010001000100010);
};

inline constexpr auto has_escape = [](uint64_t chunk) IGUANA__INLINE_LAMBDA {
  return has_zero(
      chunk ^
      0b0101110001011100010111000101110001011100010111000101110001011100);
};

IGUANA_INLINE void skip_till_escape_or_qoute(auto &&it, auto &&end) {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);
  if (std::distance(it, end) >= 7)
    AS_LIKELY {
      const auto end_m7 = end - 7;
      for (; it < end_m7; it += 8) {
        const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
        uint64_t test = has_qoute(chunk) | has_escape(chunk);
        if (test != 0) {
          it += (std::countr_zero(test) >> 3);
          return;
        }
      }
    }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    switch (*it) {
    case '\\':
    case '"':
      return;
    }
    ++it;
  }
  throw std::runtime_error("Expected \"");
}

IGUANA_INLINE void skip_till_qoute(auto &&it, auto &&end) {
  static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);
  if (std::distance(it, end) >= 7)
    AS_LIKELY {
      const auto end_m7 = end - 7;
      for (; it < end_m7; it += 8) {
        const auto chunk = *reinterpret_cast<const uint64_t *>(&*it);
        uint64_t test = has_qoute(chunk);
        if (test != 0) {
          it += (std::countr_zero(test) >> 3);
          return;
        }
      }
    }

  // Tail end of buffer. Should be rare we even get here
  while (it < end) {
    if (*it == '"')
      return;
    ++it;
  }
  throw std::runtime_error("Expected \"");
}

IGUANA_INLINE void skip_string(auto &&it, auto &&end) noexcept {
  ++it;
  while (it < end) {
    if (*it == '"') {
      ++it;
      break;
    } else if (*it == '\\' && ++it == end)
      AS_UNLIKELY
    break;
    ++it;
  }
}

template <char open, char close>
IGUANA_INLINE void skip_until_closed(auto &&it, auto &&end) {
  ++it;
  size_t open_count = 1;
  size_t close_count = 0;
  while (it < end && open_count > close_count) {
    switch (*it) {
    case '/':
      skip_comment(it, end);
      break;
    case '"':
      skip_string(it, end);
      break;
    case open:
      ++open_count;
      ++it;
      break;
    case close:
      ++close_count;
      ++it;
      break;
    default:
      ++it;
    }
  }
}

IGUANA_INLINE bool is_numeric(auto c) noexcept {
  static constexpr int is_num[256] = {
      // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, // 2
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 3
      0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
      0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F
  };
  return static_cast<bool>(is_num[static_cast<unsigned int>(c)]);
}

} // namespace iguana
