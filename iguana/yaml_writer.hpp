#pragma once

#include "detail/charconv.h"
#include "yaml_util.hpp"

namespace iguana {
template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, char c = '\n') {
  if constexpr (Is_writing_escape) {
    ss.push_back('"');
    write_string_with_escape(t.data(), t.size(), ss);
    ss.push_back('"');
  } else {
    ss.append(t.data(), t.size());
  }
  ss.push_back(c);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, char c = '\n') {
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
  ss.push_back(c);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<is_pb_type_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, char c = '\n') {
  render_yaml_value<Is_writing_escape>(ss, value.val, c);
}

template <bool Is_writing_escape, typename Stream>
IGUANA_INLINE void render_yaml_value(Stream &ss, char value, char c = '\n') {
  ss.push_back(value);
  ss.push_back(c);
}

template <bool Is_writing_escape, typename Stream>
IGUANA_INLINE void render_yaml_value(Stream &ss, bool value, char c = '\n') {
  ss.append(value ? "true" : "false");
  ss.push_back(c);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<enum_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T value, char c = '\n') {
  static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
  if constexpr (bool_v<decltype(enum_to_str)>) {
    render_yaml_value<Is_writing_escape>(
        ss, static_cast<std::underlying_type_t<T>>(value), c);
  } else {
    auto it = enum_to_str.find(value);
    if (it != enum_to_str.end()) IGUANA_LIKELY {
        auto str = it->second;
        render_yaml_value<Is_writing_escape>(
            ss, std::string_view(str.data(), str.size()), c);
      }
    else {
      throw std::runtime_error(
          std::to_string(static_cast<std::underlying_type_t<T>>(value)) +
          " is a missing value in enum_value");
    }
  }
}

template <bool Is_writing_escape = false, typename Stream, typename T>
IGUANA_INLINE void to_yaml(T &&t, Stream &s, size_t min_spaces = 0);

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t,
                                       size_t min_spaces) {
  ss.push_back('\n');
  for (const auto &v : t) {
    ss.append(min_spaces, ' ');
    ss.append("- ");
    to_yaml<Is_writing_escape>(v, ss, min_spaces + 1);
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<tuple_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, T &&t, size_t min_spaces) {
  ss.push_back('\n');
  for_each(std::forward<T>(t),
           [&ss, min_spaces](auto &v, auto i) IGUANA__INLINE_LAMBDA {
             ss.append(min_spaces, ' ');
             ss.append("- ");
             to_yaml<Is_writing_escape>(v, ss, min_spaces + 1);
           });
}

template <bool Is_writing_escape, typename Stream, typename T, typename U>
IGUANA_INLINE void render_key_value(T &&t, Stream &s, size_t min_spaces,
                                    U &&name) {
  s.append(min_spaces, ' ');
  render_yaml_value<false>(s, name, ':');  // key must be plaint type
  s.append(" ");
  to_yaml<Is_writing_escape>(t, s, min_spaces + 1);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &t,
                                       size_t min_spaces) {
  ss.push_back('\n');
  for (const auto &[k, v] : t) {
    render_key_value<Is_writing_escape>(v, ss, min_spaces, k);
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<optional_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                       size_t min_spaces) {
  if (!val) {
    ss.append("null");
    ss.push_back('\n');
  } else {
    to_yaml<Is_writing_escape>(*val, ss, min_spaces);
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &ss, const T &val,
                                       size_t min_spaces) {
  if (!val) {
    ss.push_back('\n');
  } else {
    to_yaml<Is_writing_escape>(*val, ss, min_spaces);
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<std::is_aggregate_v<std::remove_cvref_t<T>> &&
                               non_refletable_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &s, T &&t, size_t min_spaces) {
  s.push_back('\n');
  using U = std::remove_cvref_t<T>;
  constexpr size_t Count = ylt::reflection::members_count_v<U>;
  auto tp = ylt::reflection::object_to_tuple(t);
  static constexpr auto arr = ylt::reflection::get_member_names<U>();

  [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    (render_key_value<Is_writing_escape>(std::get<Is>(tp), s, min_spaces,
                                         arr[Is]),
     ...);
  }(std::make_index_sequence<Count>{});
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<refletable_v<T>, int> = 0>
IGUANA_INLINE void render_yaml_value(Stream &s, T &&t, size_t min_spaces) {
  s.push_back('\n');
  for_each(std::forward<T>(t),
           [&t, &s, min_spaces](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_type(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);
             [[maybe_unused]] constexpr std::string_view tag_name =
                 std::string_view(get_name<std::decay_t<T>, Idx>().data(),
                                  get_name<std::decay_t<T>, Idx>().size());
             render_key_value<Is_writing_escape>(t.*v, s, min_spaces, tag_name);
           });
}

template <bool Is_writing_escape = false, typename Stream, typename T>
IGUANA_INLINE void to_yaml(T &&t, Stream &s, size_t min_spaces) {
  if constexpr (tuple_v<T> || map_container_v<T> || sequence_container_v<T> ||
                optional_v<T> || smart_ptr_v<T> || refletable_v<T> ||
                std::is_aggregate_v<std::remove_cvref_t<T>>)
    render_yaml_value<Is_writing_escape>(s, std::forward<T>(t), min_spaces);
  else if constexpr (yaml_not_support_v<T>)
    static_assert(!sizeof(T), "don't suppport this type");
  else
    render_yaml_value<Is_writing_escape>(s, t);
}

template <typename T>
IGUANA_INLINE void to_yaml_adl(iguana_adl_t *p, const T &t,
                               std::string &pb_str) {
  to_yaml(t, pb_str);
}
}  // namespace iguana
