#pragma once

#include "detail/charconv.h"
#include "yaml_util.hpp"

namespace iguana {

namespace detail {

template <str_view_t U, class It>
IGUANA_INLINE void parse_value(U &value, It &&value_begin, It &&value_end) {
  using T = std::decay_t<U>;
  if (*value_begin == '"') {
    ; // TODO:  parse escape
  } else {
    auto start = value_begin;
    auto end = value_end;
    if (*value_begin == '\'') {
      ++start;
      --end;
      if (*end != '\'') [[unlikely]] {
        throw std::runtime_error(R"(Expected ')");
      }
    }
    value = T(&*start,
              static_cast<size_t>(std::distance(start, end)));
  }
}

template <str_view_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  skip_space(it, end);
  if (*it == '|') {
    ; // TODO: support |
  } else if (*it == '>') {
    ; // TODO: support > >-
  } else {
    skip_space_and_lines(it, end, min_spaces);
    auto start = it;
    auto value_end = skip_till<'\n', false>(it, end);
    parse_value(value, start, value_end);
  }
}

// minspaces : The minimum indentation
// parse_item  is always not at the start of line
template <sequence_container_t U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  value.clear();
  auto spaces = skip_space_and_lines(it, end, min_spaces);
  using value_type = typename std::remove_cv_t<U>::value_type;
  if (*it == '[') {
    ++it;
    while (it != end) {
      if (*it == ']') [[unlikely]] {
        ++it;
        return;
      }
      skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<',', ']'>(it, end);
        parse_value(value.emplace_back(), start, value_end);
      } else {
        parse_item(value.emplace_back(), it, end, min_spaces);
        skip_space_and_lines(it, end, min_spaces);
        if (*it == ',') [[likely]] {
          ++it;
        }
      }
      if (*(it - 1) == ']') [[unlikely]] {
        ++it;
        return;
      }
    }
  } else if (*it == '-') {
    // TODO: - must after \n
    ++it;
    bool first = true;
    auto subspaces = skip_space_and_lines<false>(it, end, min_spaces); 
    while (it != end) {
      if (subspaces < min_spaces) [[unlikely]] {
        it -= subspaces + 1; // back to the las line end
        return;
      }
      if (!first) [[likely]]
        match<'-'>(it, end);
      skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        auto start = it;
        auto value_end = skip_till<'\n', false>(it, end); 
        parse_value(value.emplace_back(), start, value_end);
        if (it != end) [[likely]] {
          --it;
        }
      } else {
        parse_item(value.emplace_back(), it, end, spaces + 1);
      }
      first = false;
      subspaces = skip_space_and_lines<false>(it, end, min_spaces);
    }
  } else [[unlikely]] {
    throw std::runtime_error("Expected ] or \\n");
  }
}

template <map_container U, class It>
IGUANA_INLINE void parse_item(U &value, It &&it, It &&end, size_t min_spaces) {
  using T = std::remove_reference_t<U>;
  using key_type = typename T::key_type;
  using value_type = typename T::mapped_type;
  [[maybe_unused]]auto spaces = skip_space_and_lines(it, end, min_spaces);
  if (*it == '{') {
    ++it;
    while (it != end) {
      if (*it == '}') [[unlikely]] {
        ++it;
        return;
      }
      auto subspaces = skip_space_and_lines(it, end, min_spaces);
      auto start = it;
      auto value_end = skip_till<':'>(it, end);
      key_type key;
      parse_value(key, start, value_end);
      subspaces = skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        start = it;
        value_end = skip_till<',', '}'>(it, end);
        parse_value(value[key], start, value_end);
      } else {
        parse_item(value[key], it, end, min_spaces);
        subspaces = skip_space_and_lines(it, end, min_spaces);
        if (*it == ',') [[likely]] {
          ++it;
        }
      }
      if (*(it - 1) == '}') [[unlikely]] {
        ++it;
        return;
      }
    }
  } else {
    while (it != end) {
      auto subspaces = skip_space_and_lines<false>(it, end, min_spaces); 
      if (subspaces < min_spaces) [[unlikely]] {
        it -= subspaces + 1; // back to the las line end
        return;
      }
      auto start = it;
      auto value_end = skip_till<':'>(it, end);
      key_type key;
      parse_value(key, start, value_end);
      subspaces = skip_space_and_lines(it, end, min_spaces);
      if constexpr (plain_t<value_type>) {
        start = it;
        value_end = skip_till<'\n', false>(it, end);
        parse_value(value[key], start, value_end);
        if (it != end) [[likely]] {
          --it;
        }
      } else {
        parse_item(value[key], it, end, spaces + 1);
        subspaces = skip_space_and_lines(it, end, min_spaces);
      }
    }
  }
}

} // namespace detail

template <refletable T, typename It>
IGUANA_INLINE void from_yaml(T &value, It &&it, It &&end,
                             size_t min_spaces = 0) {
  auto spaces = skip_space_and_lines_for_key(it, end);
  if (spaces < min_spaces) [[unlikely]] {
    throw std::runtime_error("Indentation problem");
  }
  auto start = it;
  auto keyend = skip_till<':'>(it, end);

  std::string_view key = std::string_view{
      &*start, static_cast<size_t>(std::distance(start, keyend))};
  static constexpr auto frozen_map = get_iguana_struct_map<T>();
  if constexpr (frozen_map.size() > 0) {
    const auto &member_it = frozen_map.find(key);
    if (member_it != frozen_map.end()) [[likely]] {
      std::visit(
          [&](auto &&member_ptr) IGUANA__INLINE_LAMBDA {
            using V = std::decay_t<decltype(member_ptr)>;
            if constexpr (std::is_member_pointer_v<V>) {
              detail::parse_item(value.*member_ptr, it, end, spaces + 1);
            } else {
              static_assert(!sizeof(V), "type not supported");
            }
          },
          member_it->second);
    } else [[unlikely]] {
      // TODO: don't throw
      throw std::runtime_error("Unknown key: " + std::string(key));
    }
  }
}

template <typename T, string_t View>
IGUANA_INLINE void from_yaml(T &value, const View &view) {
  from_yaml(value, std::begin(view), std::end(view));
}

} // namespace iguana
