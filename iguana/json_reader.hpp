#pragma once
#include "detail/fast_float.h"
#include "detail/utf.hpp"
#include "json_util.hpp"
#include "reflection.hpp"
#include <charconv>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <string_view>
#include <type_traits>

namespace iguana {

template <class T>
concept char_t = std::same_as < std::decay_t<T>,
char > || std::same_as<std::decay_t<T>, char16_t> ||
    std::same_as<std::decay_t<T>, char32_t> ||
    std::same_as<std::decay_t<T>, wchar_t>;

template <class T>
concept bool_t = std::same_as < std::decay_t<T>,
bool > || std::same_as<std::decay_t<T>, std::vector<bool>::reference>;

template <class T>
concept int_t =
    std::integral<std::decay_t<T>> && !char_t<std::decay_t<T>> && !bool_t<T>;

template <class T>
concept num_t = std::floating_point<std::decay_t<T>> || int_t<T>;

template <class T>
concept str_t = std::convertible_to<std::decay_t<T>, std::string_view>;

template <typename Type> constexpr inline bool is_std_vector_v = false;

template <typename... args>
constexpr inline bool is_std_vector_v<std::vector<args...>> = true;

template <typename Type>
concept vector_container = is_std_vector_v<std::remove_reference_t<Type>>;

template <typename Type>
concept optional = requires(Type optional) {
  optional.value();
  optional.has_value();
  optional.operator*();
  typename std::remove_cvref_t<Type>::value_type;
};

template <typename Type>
concept container = requires(Type container) {
  typename std::remove_cvref_t<Type>::value_type;
  container.size();
  container.begin();
  container.end();
};

template <typename Type>
concept map_container = container<Type> && requires(Type container) {
  typename std::remove_cvref_t<Type>::mapped_type;
};

template <class T>
concept c_array = std::is_array_v<std::remove_cvref_t<T>> &&
                  std::extent_v<std::remove_cvref_t<T>> >
0;

template <typename Type>
concept array = requires(Type arr) {
  arr.size();
  std::tuple_size<std::remove_cvref_t<Type>>{};
};

template <typename Type>
concept fixed_array = c_array<Type> || array<Type>;

template <typename Type>
concept tuple = !array<Type> && requires(Type tuple) {
  std::get<0>(tuple);
  sizeof(std::tuple_size<std::remove_cvref_t<Type>>);
};

template <typename Type>
concept json_view = requires(Type container) {
  container.size();
  container.begin();
  container.end();
};

template <typename T>
concept json_byte = std::is_same_v<char, T> ||
    std::is_same_v<unsigned char, T> || std::is_same_v<std::byte, T>;

template <typename Type> constexpr inline bool is_std_list_v = false;
template <typename... args>
constexpr inline bool is_std_list_v<std::list<args...>> = true;

template <typename Type> constexpr inline bool is_std_deque_v = false;
template <typename... args>
constexpr inline bool is_std_deque_v<std::deque<args...>> = true;

template <typename Type>
concept sequence_container = is_std_list_v<std::remove_reference_t<Type>> ||
    is_std_vector_v<std::remove_reference_t<Type>> ||
    is_std_deque_v<std::remove_reference_t<Type>>;

template <class T>
concept non_refletable = container<T> || c_array<T> || tuple<T> ||
    optional<T> || std::is_fundamental_v<T>;

template <refletable T, typename It>
errc from_json(T &value, It &&it, It &&end) noexcept;

template <num_t U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  skip_ws(it, end);

  using T = std::remove_reference_t<U>;

  if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
    if constexpr (std::is_floating_point_v<T>) {
      const auto size = std::distance(it, end);
      if (size == 0) [[unlikely]]
        return errc::failed_parse_number;

      const auto start = &*it;
      auto [p, ec] = fast_float::from_chars(start, start + size, value);
      if (ec != std::errc{}) [[unlikely]]
        return errc::failed_parse_number;
      it += (p - &*it);
    } else {
      const auto size = std::distance(it, end);
      const auto start = &*it;
      auto [p, ec] = std::from_chars(start, start + size, value);
      if (ec != std::errc{}) [[unlikely]]
        return errc::failed_parse_number;
      it += (p - &*it);
    }
  } else {
    double num;
    char buffer[256];
    size_t i{};
    while (it != end && is_numeric(*it)) {
      if (i > 254) [[unlikely]]
        return errc::number_is_too_long;
      buffer[i] = *it++;
      ++i;
    }
    auto [p, ec] = fast_float::from_chars(buffer, buffer + i, num);
    if (ec != std::errc{}) [[unlikely]]
      return errc::failed_parse_number;
    value = static_cast<T>(num);
  }

  return errc::ok;
}

template <str_t U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it, It &&end,
                                            bool skip = false) noexcept {
  if (!skip) {
    skip_ws(it, end);
    auto ec = match<'"'>(it, end);
    if (ec != errc::ok) {
      return ec;
    }
  }

  if constexpr (!std::contiguous_iterator<std::decay_t<It>>) {
    const auto cend = value.cend();
    for (auto c = value.begin(); c < cend; ++c, ++it) {
      if (it == end) [[unlikely]]
        return errc::lack_of_parenthesis;
      switch (*it) {
        [[unlikely]] case '\\' : {
          if (++it == end) [[unlikely]]
            return errc::lack_of_parenthesis;
          else [[likely]] {
            *c = *it;
          }
          break;
        }
        [[unlikely]] case '"' : {
          ++it;
          value.resize(std::distance(value.begin(), c));
          return errc::ok;
        }
        [[unlikely]] case 'u' : {
          ++it;
          auto start = it;
          auto code_point = parse_unicode_hex4(it);
          std::string str;
          encode_utf8(str, code_point);
          std::memcpy(value.data(), str.data(), str.size());
          --it;
          c += std::distance(start, it) - 1;

          break;
        }
        [[likely]] default : *c = *it;
      }
    }
  }

  // growth portion
  if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
    value.clear(); // Single append on unescaped strings so overwrite opt isnt
                   // as important
    auto start = it;
    while (it < end) {
      auto ec = skip_till_escape_or_qoute(it, end);
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
      if (*it == '"') {
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it;
        return errc::ok;
      } else {
        // Must be an escape
        // TODO propperly handle this
        value.append(&*start, static_cast<size_t>(std::distance(start, it)));
        ++it; // skip first escape
        if (*it == 'u') {
          ++it;
          auto code_point = parse_unicode_hex4(it);
          encode_utf8(value, code_point);
          start = it;
        } else {
          value.push_back(*it); // add the escaped character
          ++it;
          start = it;
        }
      }
    }
  } else {
    while (it != end) {
      switch (*it) {
        [[unlikely]] case '\\' : {
          if (++it == end) [[unlikely]]
            return errc::lack_of_parenthesis;
          else [[likely]] {
            value.push_back(*it);
          }
          break;
        }
        [[unlikely]] case ']' : { return errc::ok; }
        [[unlikely]] case '"' : {
          ++it;
          return errc::ok;
        }
        [[unlikely]] case 'u' : {
          ++it;
          auto code_point = parse_unicode_hex4(it);
          encode_utf8(value, code_point);
          break;
        }
        [[likely]] default : value.push_back(*it);
      }
      ++it;
    }
  }

  return errc::ok;
}

template <fixed_array U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  using T = std::remove_reference_t<U>;
  skip_ws(it, end);

  auto ec = match<'['>(it, end);
  if (ec != errc::ok) {
    return ec;
  }
  skip_ws(it, end);
  if (it == end) {
    return errc::unexpected_end;
  }

  if (*it == ']') [[unlikely]] {
    ++it;
    return errc::ok;
  }

  constexpr auto n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));

  auto value_it = std::begin(value);

  for (size_t i = 0; i < n; ++i) {
    ec = parse_item(*value_it++, it, end);
    if (ec != errc::ok) [[unlikely]] {
      return ec;
    }
    skip_ws(it, end);
    if (it == end) {
      return errc::unexpected_end;
    }
    if (*it == ',') [[likely]] {
      ++it;
      skip_ws(it, end);
    } else if (*it == ']') {
      ++it;
      return errc::ok;
    } else [[unlikely]] {
      return errc::lack_of_bracket;
    }
  }

  return errc::ok;
}

template <sequence_container U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  value.clear();
  errc ec{};
  skip_ws(it, end);

  ec = match<'['>(it, end);
  if (ec != errc::ok) [[unlikely]] {
    return ec;
  }

  skip_ws(it, end);
  for (size_t i = 0; it != end; ++i) {
    if (*it == ']') [[unlikely]] {
      ++it;
      return errc::ok;
    }
    if (i > 0) [[likely]] {
      ec = match<','>(it, end);
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
    }

    using value_type = typename std::remove_cv_t<U>::value_type;
    if constexpr (refletable<value_type>) {
      ec = from_json(value.emplace_back(), it, end);
    } else {
      ec = parse_item(value.emplace_back(), it, end);
    }

    if (ec != errc::ok) [[unlikely]] {
      return ec;
    }

    skip_ws(it, end);
  }

  return errc::lack_of_bracket;
}

template <map_container U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  using T = std::remove_reference_t<U>;
  skip_ws(it, end);

  iguana::errc ec{};
  ec = match<'{'>(it, end);
  if (ec != errc::ok) {
    return ec;
  }
  skip_ws(it, end);
  bool first = true;
  while (it != end) {
    if (*it == '}') [[unlikely]] {
      ++it;
      return errc::ok;
    } else if (first) [[unlikely]]
      first = false;
    else [[likely]] {
      ec = match<','>(it, end);
      if (ec != errc::ok) {
        return ec;
      }
    }

    static thread_local std::string key{};
    ec = parse_item(key, it, end);
    if (ec != errc::ok) [[unlikely]] {
      return ec;
    }

    skip_ws(it, end);
    ec = match<':'>(it, end);
    if (ec != errc::ok) [[unlikely]] {
      return ec;
    }

    if constexpr (std::is_same_v<typename T::key_type, std::string>) {
      ec = parse_item(value[key], it, end);
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
    } else {
      static thread_local typename T::key_type key_value{};
      ec = parse_item(key_value, key.begin(), key.end());
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
      ec = parse_item(value[key_value], it, end);
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
    }
    skip_ws(it, end);
  }

  return errc::ok;
}

template <tuple U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  skip_ws(it, end);
  iguana::errc ec{};
  ec = match<'['>(it, end);
  if (ec != errc::ok) {
    return ec;
  }
  skip_ws(it, end);

  for_each(value, [&](auto &v, auto i) IGUANA__INLINE_LAMBDA {
    if (ec != errc::ok) [[unlikely]] {
      return;
    }
    constexpr auto I = decltype(i)::value;
    if (it == end || *it == ']') {
      return;
    }
    if constexpr (I != 0) {
      ec = match<','>(it, end);
      if (ec != errc::ok) [[unlikely]] {
        return;
      }
      skip_ws(it, end);
    }
    ec = parse_item(v, it, end);
    if (ec != errc::ok) [[unlikely]] {
      return;
    }
    skip_ws(it, end);
  });

  if (ec != errc::ok) [[unlikely]] {
    return ec;
  }

  ec = match<']'>(it, end);
  if (ec != errc::ok) {
    return ec;
  }
  return errc::ok;
}

template <bool_t U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  skip_ws(it, end);
  iguana::errc ec{};
  if (it < end) [[likely]] {
    switch (*it) {
    case 't': {
      ++it;
      ec = match<"rue">(it, end);
      if (ec != errc::ok) {
        return ec;
      }
      value = true;
      break;
    }
    case 'f': {
      ++it;
      ec = match<"alse">(it, end);
      if (ec != errc::ok) {
        return ec;
      }
      value = false;
      break;
    }
      [[unlikely]] default : return errc::not_a_bool;
    }
  } else [[unlikely]] {
    return errc::not_a_bool;
  }

  return errc::ok;
}

template <optional U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  skip_ws(it, end);
  using T = std::remove_reference_t<U>;
  if (it == end) {
    return errc::unexpected_end;
  }
  if (*it == 'n') {
    ++it;
    auto ec = match<"ull">(it, end);
    if (ec != errc::ok) [[unlikely]] {
      return ec;
    }
    if constexpr (!std::is_pointer_v<T>) {
      value.reset();
    }
  } else {
    if (!value) {
      typename T::value_type t;
      auto ec = parse_item(t, it, end);
      if (ec != errc::ok) [[unlikely]] {
        return ec;
      }
      value = std::move(t);
    }
  }

  return errc::ok;
}

template <char_t U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  // TODO: this does not handle escaped chars
  skip_ws(it, end);
  errc ec{};
  ec = match<'"'>(it, end);
  if (ec != errc::ok) [[unlikely]] {
    return ec;
  }
  if (it == end) [[unlikely]]
    return errc::unexpected_end;
  if (*it == '\\') [[unlikely]]
    if (++it == end) [[unlikely]]
      return errc::unexpected_end;
  value = *it++;
  match<'"'>(it, end);
  if (ec != errc::ok) {
    return ec;
  }
  return errc::ok;
}

template <refletable U, class It>
[[nodiscard]] IGUANA_INLINE errc parse_item(U &value, It &&it,
                                            It &&end) noexcept {
  return from_json(value, it, end);
}

template <refletable T, typename It>
[[nodiscard]] IGUANA_INLINE errc from_json(T &value, It &&it,
                                           It &&end) noexcept {
  skip_ws(it, end);

  errc ec{};
  ec = match<'{'>(it, end);
  if (ec != errc::ok) [[unlikely]] {
    return errc::not_match_specific_chars;
  }
  skip_ws(it, end);
  bool first = true;
  while (it != end) {
    if (*it == '}') [[unlikely]] {
      ++it;
      return errc::ok;
    } else if (first) [[unlikely]]
      first = false;
    else [[likely]] {
      ec = match<','>(it, end);
      if (ec != errc::ok) [[unlikely]] {
        return errc::not_match_specific_chars;
      }
    }

    if constexpr (refletable<T>) {
      std::string_view key;
      if constexpr (std::contiguous_iterator<std::decay_t<It>>) {
        // skip white space and escape characters and find the string
        skip_ws(it, end);
        ec = match<'"'>(it, end);
        if (ec != errc::ok) [[unlikely]] {
          return errc::not_match_specific_chars;
        }
        auto start = it;
        ec = skip_till_escape_or_qoute(it, end);
        if (ec != errc::ok) [[unlikely]] {
          return ec;
        }
        if (*it == '\\') [[unlikely]] {
          // we dont' optimize this currently because it would increase binary
          // size significantly with the complexity of generating escaped
          // compile time versions of keys
          it = start;
          static thread_local std::string static_key{};
          ec = parse_item(static_key, it, end, true);
          if (ec != errc::ok) [[unlikely]] {
            return ec;
          }
          key = static_key;
        } else [[likely]] {
          key = std::string_view{&*start,
                                 static_cast<size_t>(std::distance(start, it))};
          ++it;
        }
      } else {
        static thread_local std::string static_key{};
        ec = parse_item(static_key, it, end, true);
        if (ec != errc::ok) [[unlikely]] {
          return ec;
        }
        key = static_key;
      }

      skip_ws(it, end);
      ec = match<':'>(it, end);
      if (ec != errc::ok) [[unlikely]] {
        return errc::not_match_specific_chars;
      }

      static constexpr auto frozen_map = get_iguana_struct_map<T>();
      const auto &member_it = frozen_map.find(key);
      if (member_it != frozen_map.end()) {
        std::visit(
            [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
              using V = std::decay_t<decltype(member_ptr)>;
              if constexpr (std::is_member_pointer_v<V>) {
                ec = parse_item(value.*member_ptr, it, end);
              } else {
                static_assert(!sizeof(V), "type not supported");
              }
            },
            member_it->second);
        if (ec != errc::ok) [[unlikely]] {
          return ec;
        }
      } else [[unlikely]] {
        return errc::unknown_key;
      }
    }
    skip_ws(it, end);
  }
  return errc::ok;
}

template <non_refletable T, typename It>
[[nodiscard]] IGUANA_INLINE errc from_json(T &value, It &&it,
                                           It &&end) noexcept {
  return parse_item(value, it, end);
}

template <typename T>
[[nodiscard]] IGUANA_INLINE errc
from_json_file(T &value, const std::string &filename) noexcept {
  std::error_code ec;
  uint64_t size = std::filesystem::file_size(filename, ec);
  if (ec) {
    return errc::file_size_error;
  }

  if (size == 0) {
    return errc::empty_file;
  }

  std::ifstream file(filename, std::ios::binary);

  std::string content;
  content.resize(size);

  file.read(content.data(), size);
  return from_json(value, content.begin(), content.end());
}

template <typename T, json_view View>
[[nodiscard]] IGUANA_INLINE errc from_json(T &value,
                                           const View &view) noexcept {
  return from_json(value, std::begin(view), std::end(view));
}

template <typename T, json_byte Byte>
IGUANA_INLINE errc from_json(T &value, const Byte *data, size_t size) noexcept {
  std::string_view buffer(data, size);
  return from_json(value, buffer);
}

template <typename T, typename It>
IGUANA_INLINE errc from_json(T &value, It &&it, It &&end) {
  static_assert(!sizeof(T), "The type is not support, please check if you have "
                            "defined REFLECTION for the type, otherwise the "
                            "type is not supported now!");
  return {};
}

} // namespace iguana
