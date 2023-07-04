#pragma once
#include "../rapidxml/rapidxml.hpp"
#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "xml_util.hpp"
#include <charconv>

namespace iguana {
namespace detail {

template <bool skip, refletable T, typename It>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end,
                              std::string_view name);

template <map_container U, typename It>
IGUANA_INLINE void parse_attr(U &&value, It &&it, It &&end) {
  using key_type = typename std::remove_cvref_t<U>::key_type;
  using value_type = typename std::remove_cvref_t<U>::mapped_type;
  while (it != end) {
    skip_sapces_and_newline(it, end);
    if (*it == '>') [[unlikely]] {
      ++it;
      return;
    }
    auto key_begin = it;
    auto key_end = skip_till<'='>(it, end);
    key_type key;
    parse_value(key, key_begin, key_end);

    skip_sapces_and_newline(it, end);
    auto value_begin = it;
    auto value_end = skip_till<' ', '>'>(it, end);
    value_type v;
    parse_value(v, value_begin, value_end);
    value.emplace(std::move(key), std::move(v));
  }
}

// TODO: check begin == end ?
template <plain_t U, class It>
IGUANA_INLINE void parse_value(U &&value, It &&begin, It &&end) {
  using T = std::decay_t<U>;
  if constexpr (string_t<T>) {
    value = T(&*begin, static_cast<size_t>(std::distance(begin, end)));
  } else if constexpr (num_t<T>) {
    auto size = std::distance(begin, end);
    const auto start = &*begin;
    auto [p, ec] = detail::from_chars(start, start + size, value);
    if (ec != std::errc{}) [[unlikely]]
      throw std::runtime_error("Failed to parse number");
  } else if constexpr (char_t<T>) {
    if (static_cast<size_t>(std::distance(begin, end)) != 1) [[unlikely]] {
      throw std::runtime_error("Expected one character");
    }
    value = *begin;
  } else if constexpr (bool_t<T>) {
    auto bool_v = std::string_view(
        &*begin, static_cast<size_t>(std::distance(begin, end)));
    if (bool_v == "true") {
      value = true;
    } else if (bool_v == "false") {
      value = false;
    } else [[unlikely]] {
      throw std::runtime_error("Expected true or false");
    }
  } else if constexpr (enum_t<T>) {
    using T = std::underlying_type_t<std::decay_t<U>>;
    parse_value(reinterpret_cast<T &>(value), begin, end);
  }
}

template <bool skip, plain_t U, class It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end,
                              std::string_view name) {
  if constexpr (skip) {
    skip_till<'>'>(it, end);
    ++it;
  }    
  skip_sapces_and_newline(it, end);
  auto value_begin = it;
  auto value_end = skip_pass<'<'>(it, end);
  parse_value(value, value_begin, value_end);
  match_close_tag(it, end, name);
}

template <bool skip, sequence_container_t U, class It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end,
                              std::string_view name) {
  // using value_type = typename std::remove_cvref_t<U>::value_type;
  // if constexpr(attr_t<value_type>) { 
  //   skip_till<'>'>(it, end);
  //   ++it;
  // }
  parse_item<true>(value.emplace_back(), it, end, name);
  skip_sapces_and_newline(it, end);
  while (it != end) {
    match<'<'>(it, end);
    auto start = it;
    skip_till<' ', '>'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    if (key != name) [[unlikely]] {
      it = start - 1;
      return;
    }
    // if constexpr (attr_t<value_type>) {
    //   // 解析属性
    // } else {
    //   skip_till<'>'>(it, end);
    //   if constexpr (optional_t<value_type>) {
    //     if (*(it - 1) == '/') {
    //       ++it;
    //       skip_sapces_and_newline(it, end);
    //       continue;
    //     }
    //   }
    //   ++it;
    //   parse_item(value.emplace_back(), it, end, name);
    // }
    parse_item<true>(value.emplace_back(), it, end, name);
    skip_sapces_and_newline(it, end);
  }
}
// skip必须有，为了统一
template <bool skip, optional_t U, class It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end,
                              std::string_view name) {
  skip_till<'>'>(it, end);
  if (*(it - 1) == '/') {
    ++it;
    return;
  }
  ++it;
  parse_item<false>(value.emplace(), it, end, name);
}

template <bool skip, unique_ptr_t U, class It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end,
                              std::string_view name) {
  value = std::make_unique<typename std::remove_cvref_t<U>::element_type>();
  parse_item<skip>(*value, it, end, name);
}

template <bool skip, attr_t U, typename It>
IGUANA_INLINE void parse_item(U &&value, It &&it, It &&end,
                              std::string_view name) {
  parse_attr(value.attr(), it, end);
  parse_item<false>(value.value(), it, end, name); 
}

// skip == true 跳过属性，为false一般是解析属性的时候
template <bool skip, refletable T, typename It>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end,
                              std::string_view name) {
  if constexpr (skip) {
    skip_till<'>'>(it, end);
    ++it;
  }
  skip_sapces_and_newline(it, end);
  while (it != end) {
    match<'<'>(it, end);
    if (*it == '/') [[unlikely]] {
      match_close_tag(it, end, name);
      return; // reach the close tag
    }
    auto start = it;
    skip_till<' ', '>'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    static constexpr auto frozen_map = get_iguana_struct_map<T>();
    const auto &member_it = frozen_map.find(key);
    if (member_it != frozen_map.end()) [[likely]] {
      std::visit(
          [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
            static_assert(
                std::is_member_pointer_v<std::decay_t<decltype(member_ptr)>>,
                "type must be memberptr");
            detail::parse_item<true>(value.*member_ptr, it, end, key); 
          },
          member_it->second);
    } else [[unlikely]] {
      throw std::runtime_error("Unknown key: " + std::string(key));
    }
    skip_sapces_and_newline(it, end);
  }
}

} // namespace detail

template <attr_t U, typename It>
IGUANA_INLINE void from_xml(U &&value, It &&it, It &&end) {
  while (it != end) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    if (*it == '?') {
      skip_till<'>'>(it, end);
      ++it;
    } else {
      break;
    }
  }
  auto start = it;
  skip_till<' ', '>'>(it, end);
  std::string_view key = std::string_view{
      &*start, static_cast<size_t>(std::distance(start, it))};
  detail::parse_attr(value.attr(), it, end);
  detail::parse_item<false>(value.value(), it, end, key);
}

template <refletable U, typename It>
IGUANA_INLINE void from_xml(U &&value, It &&it, It &&end) {
  while (it != end) {
    skip_sapces_and_newline(it, end);
    match<'<'>(it, end);
    if (*it == '?') {
      skip_till<'>'>(it, end);
      ++it; // skip >
      // '<?xml ' - xml declaration
      // or
      // Parse PI
    } else {
      break;
    }
  }
  auto start = it;
  skip_till<' ', '>'>(it, end);
  std::string_view key =
      std::string_view{&*start, static_cast<size_t>(std::distance(start, it))};
  detail::parse_item<true>(value, it, end, key);
}

template <typename U, string_t View>
IGUANA_INLINE void from_xml(U &&value, const View &view) {
  from_xml(value, std::begin(view), std::end(view));
}

} // namespace iguana