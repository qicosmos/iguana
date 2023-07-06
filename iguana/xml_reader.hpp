#pragma once
#include "../rapidxml/rapidxml.hpp"
#include "detail/charconv.h"
#include "detail/utf.hpp"
#include "xml_util.hpp"
#include <charconv>

namespace iguana {
namespace detail {

template <refletable T, typename It>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end,
                              std::string_view name);

template <attr_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name);

// TODO: check begin == end ?
template <plain_t U, typename It>
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

template <map_container U, typename It>
IGUANA_INLINE void parse_attr(U &&value, It &&it, It &&end) {
  using key_type = typename std::remove_cvref_t<U>::key_type;
  using value_type = typename std::remove_cvref_t<U>::mapped_type;
  while (it != end) {
    skip_sapces_and_newline(it, end);
    if (*it == '>' || *it == '/') [[unlikely]] {
      return;
    }
    auto key_begin = it;
    auto key_end = skip_pass<'='>(it, end);
    key_type key;
    parse_value(key, key_begin, key_end);

    skip_sapces_and_newline(it, end);
    match<'"'>(it, end);
    auto value_begin = it;
    auto value_end = skip_pass<'"'>(it, end);
    value_type v;
    parse_value(v, value_begin, value_end);
    value.emplace(std::move(key), std::move(v));
  }
}

template <plain_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  skip_till<'>'>(it, end);
  ++it;
  skip_sapces_and_newline(it, end);
  auto value_begin = it;
  auto value_end = skip_pass<'<'>(it, end);
  parse_value(value, value_begin, value_end);
  match_close_tag(it, end, name);
}

template <sequence_container_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  parse_item(value.emplace_back(), it, end, name);
  skip_sapces_and_newline(it, end);
  while (it != end) {
    match<'<'>(it, end);

    if (*it == '?') [[unlikely]] {
      skip_till<'>'>(it, end);
      ++it;
      continue;
    } else if (*it == '!') [[unlikely]] {
      if (*(it + 1) == '[') {
        --it;
        return;
      } else [[unlikely]] {
        skip_till<'>'>(it, end);
        ++it;
        continue;
      }
    }

    auto start = it;
    skip_till<' ', '>'>(it, end);
    std::string_view key = std::string_view{
        &*start, static_cast<size_t>(std::distance(start, it))};
    if (key != name) [[unlikely]] {
      it = start - 1;
      return;
    }
    parse_item(value.emplace_back(), it, end, name);
    skip_sapces_and_newline(it, end);
  }
}

template <optional_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  using value_type = typename std::remove_reference_t<U>::value_type;
  skip_till<'>'>(it, end);
  if (*(it - 1) == '/') {
    ++it;
    return;
  }

  if constexpr (plain_t<value_type>) {
    // The following code is for option not to be emplaced
    // when parse  "...> <..."
    skip_till<'>'>(it, end);
    ++it;
    skip_sapces_and_newline(it, end);
    auto value_begin = it;
    auto value_end = skip_pass<'<'>(it, end);
    if (value_begin == value_end) {
      match_close_tag(it, end, name);
      return;
    }
    parse_value(value.emplace(), value_begin, value_end);
    match_close_tag(it, end, name);
  } else {
    parse_item(value.emplace(), it, end, name);
  }
}

template <unique_ptr_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  value = std::make_unique<typename std::remove_cvref_t<U>::element_type>();
  parse_item(*value, it, end, name);
}

template <attr_t U, typename It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end,
                              std::string_view name) {
  parse_attr(value.attr(), it, end);
  parse_item(value.value(), it, end, name);
}

template <refletable T, typename It>
IGUANA_INLINE void parse_item(T &value, It &&it, It &&end,
                              std::string_view name) {
  constexpr auto cdata_idx = get_type_index<is_cdata_t, std::decay_t<T>>();
  skip_till<'>'>(it, end);
  ++it;
  skip_sapces_and_newline(it, end);
  while (it != end) {
    match<'<'>(it, end);
    if (*it == '/') [[unlikely]] {
      // </tag>
      match_close_tag(it, end, name);
      return; // reach the close tag
    } else if (*it == '?') [[unlikely]] {
      // <? ... ?>
      skip_till<'>'>(it, end);
      ++it;
    } else if (*it == '!') [[unlikely]] {
      ++it;
      if (*it == '[') {
        // <![
        if constexpr (cdata_idx == iguana::get_value<std::decay_t<T>>()) {
          ++it;
          skip_till<']'>(it, end);
          ++it;
          match<"]>">(it, end);
          skip_sapces_and_newline(it, end);
          continue;
        } else {
          // if parse cdata
          ++it;
          match<"CDATA[">(it, end);
          skip_sapces_and_newline(it, end);
          auto &cdata_value = get<cdata_idx>(value);
          using VT = typename std::decay_t<decltype(cdata_value)>::value_type;
          auto vb = it;
          auto ve = skip_pass<']'>(it, end);
          if constexpr (str_view_t<VT>) {
            cdata_value.value() =
                VT(&*vb, static_cast<size_t>(std::distance(vb, ve)));
          } else {
            cdata_value.value().append(
                &*vb, static_cast<size_t>(std::distance(vb, ve)));
          }
          match<"]>">(it, end);
          skip_sapces_and_newline(it, end);
          continue;
        }
      } else if (*it == 'D') {
        // // <!D
        skip_till<'>'>(it, end);
        ++it;
        skip_sapces_and_newline(it, end);
        continue;
      } // else
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
            using V = std::remove_reference_t<decltype(value.*member_ptr)>;
            if constexpr (!cdata_t<V>) {
              detail::parse_item(value.*member_ptr, it, end, key);
            }
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
IGUANA_INLINE void from_xml(U &value, It &&it, It &&end) {
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
  std::string_view key =
      std::string_view{&*start, static_cast<size_t>(std::distance(start, it))};
  detail::parse_attr(value.attr(), it, end);
  detail::parse_item(value.value(), it, end, key);
}

template <refletable U, typename It>
IGUANA_INLINE void from_xml(U &value, It &&it, It &&end) {
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
  detail::parse_item(value, it, end, key);
}

template <typename U, string_t View>
IGUANA_INLINE void from_xml(U &value, const View &view) {
  from_xml(value, std::begin(view), std::end(view));
}

} // namespace iguana