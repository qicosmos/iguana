#pragma once
#include "../rapidxml/rapidxml.hpp"
#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "xml_util.hpp"
#include <charconv>

namespace iguana {

namespace detail {

template <plain_t U, class It>
IGUANA_INLINE void parse_value(U &&value, It &&value_begin, It &&value_end) {
  using T = std::decay_t<U>;
  if constexpr (string_t<T>) {
    value = T(&*value_begin,
              static_cast<size_t>(std::distance(value_begin, value_end)));
  } else if constexpr (num_t<T>) {
    auto size = std::distance(value_begin, value_end);
    const auto start = &*value_begin;
    auto [p, ec] = detail::from_chars(start, start + size, value);
    if (ec != std::errc{}) [[unlikely]]
      throw std::runtime_error("Failed to parse number");
  } else if constexpr (char_t<T>) {
    if (static_cast<size_t>(std::distance(value_begin, value_end)) != 1)
        [[unlikely]] {
      throw std::runtime_error("Expected one character");
    }
    value = *value_begin;
  } else if constexpr (bool_t<T>) {
    auto bool_v = std::string_view(
        &*value_begin,
        static_cast<size_t>(std::distance(value_begin, value_end)));
    if (bool_v == "true") {
      value = true;
    } else if (bool_v == "false") {
      value = false;
    } else [[unlikely]] {
      throw std::runtime_error("Expected true or false");
    }
  } else if constexpr (enum_t<T>) {
    using T = std::underlying_type_t<std::decay_t<U>>;
    parse_value(reinterpret_cast<T &>(value), value_begin, value_end);
  }
}

template <plain_t U, class It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end) {
  skip_sapces_and_newline(it, end);
  auto value_begin = it;
  auto value_end = skip_just_pass<'<'>(it, end);
  if (value_begin == value_end) [[unlikely]] {
    // TODO: throw?
    // throw std::runtime_error("empty value");
    return;
  }
  parse_value(value, value_begin, value_end);
}

template <refletable T, typename It>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end) {
  skip_sapces_and_newline(it, end); // TODO: not need?
  while (it != end) {
    match<'<'>(it, end);
    if (*it == '/') [[unlikely]] {
      return; // reach the close tag
    }
    auto start = it;
    skip_till<' ', '>'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    static constexpr auto frozen_map = get_iguana_struct_map<T>();
    const auto &member_it = frozen_map.find(key);
    skip_till<'>'>(it, end); // 这里跳过了属性, 可能以空格开始
    if (*(it - 1) == '/') [[unlikely]] {
      ++it;
      skip_sapces_and_newline(it, end); 
      continue;
    }
    ++it;                    // skip >
    if (member_it != frozen_map.end()) [[likely]] {
      std::visit(
          [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
            static_assert(
                std::is_member_pointer_v<std::decay_t<decltype(member_ptr)>>,
                "type must be memberptr");
            using V = std::remove_reference_t<decltype(value.*member_ptr)>;
            if constexpr (plain_t<V> || refletable<V>) {
              detail::parse_item(value.*member_ptr, it, end);
            } else if constexpr (sequence_container_t<V>) {
              detail::parse_item((value.*member_ptr).emplace_back(), it, end);
            } else if constexpr (optional_t<V>) {
              detail::parse_item((value.*member_ptr).emplace(), it, end);
            } else if constexpr (unique_ptr_t<V>) {
              value.*member_ptr = std::make_unique<typename V::element_type>();
              parse_item(*(value.*member_ptr), it, end);
            } else {
              static_assert(!sizeof(V), "type is not supported");
            }
          },
          member_it->second);
      match_close_tag(it, end, key);
    } else [[unlikely]] {
      throw std::runtime_error("Unknown key: " + std::string(key));
    }
    skip_sapces_and_newline(it, end);
  }
}

} // namespace detail

template <bool skip = false, typename U, typename It>
IGUANA_INLINE void from_xml(U &value, It &&it, It &&end) {
  using T = std::remove_reference_t<U>;
  static_assert(plain_t<T> || refletable<T>, "type not supported");
  while (true) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    if (*it == '?') {
      skip_till<'>'>(it, end); // 暂时跳过
      ++it;
      // '<?xml ' - xml declaration
      // or
      // Parse PI
    } else {
      --it;
      break;
    }
  }
  if constexpr (skip) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    auto start = it;
    skip_till<' ', '>'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    skip_till<'>'>(it, end); // 这里跳过了属性, 可能以空格开始
    ++it;                    // skip >
    detail::parse_item(value, it, end);
    match_close_tag(it, end, key);
  } else {
    detail::parse_item(value, it, end);
  }
}

template <bool skip = false, typename U, string_t View>
IGUANA_INLINE void from_xml(U &value, const View &view) {
  from_xml<skip>(value, std::begin(view), std::end(view));
}

} // namespace iguana