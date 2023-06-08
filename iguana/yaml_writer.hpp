#pragma once

#include "detail/charconv.h"
#include "yaml_util.hpp"

namespace iguana {

// TODO: support more string style
template <typename Stream, string_t T>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, size_t min_spaces) {
  ss.append(t.data(), t.size());
}

template <typename Stream, num_t T>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, size_t min_spaces) {
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
}

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t, size_t min_spaces) {
  for (const auto& v : t) {
    ss.push_back('\n');
    ss.append(min_spaces, ' ');
    ss.append("- ");
    render_yaml_value(ss, v, min_spaces + 1);
  }
}

template <typename Stream, map_container T>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t, size_t min_spaces) {
  for (const auto& [k, v] : t) {
    ss.push_back('\n');
    ss.append(min_spaces, ' ');
    render_yaml_value(ss, k, 0); // key must be plaint type
    ss.append(": ");
    render_yaml_value(ss, v, min_spaces + 1);
  }
}

constexpr auto write_yaml_key = [](auto &s, auto i,
                                   auto &t) IGUANA__INLINE_LAMBDA {
  constexpr auto name =
      get_name<decltype(t),
               decltype(i)::value>();
  s.append(name.data(), name.size());
};

template <typename Stream, refletable T>
IGUANA_INLINE void to_yaml(T &&t, Stream &s, size_t min_spaces = 0) {
  s.append(min_spaces, ' ');
  for_each(std::forward<T>(t),
           [&t, &s, min_spaces](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_members(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);

             write_yaml_key(s, i, t);
             s.append(": ");
             if constexpr (!is_reflection<decltype(v)>::value) {
               render_yaml_value(s, t.*v, min_spaces + 1);
             } else {
               to_yaml(t.*v, s, min_spaces + 1);
             }
             s.push_back('\n');
           });
}

} // namespace iguana
