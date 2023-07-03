#pragma once

#include "detail/charconv.h"
#include "reflection.hpp"
#include "xml_util.hpp"

namespace iguana {

template <bool tag = true, typename Stream, refletable T>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name);

template <typename Stream>
inline void render_tail(Stream &ss, std::string_view str) {
  ss.push_back('<');
  ss.push_back('/');
  ss.append(str.data(), str.size());
  ss.push_back('>');
}

template <typename Stream>
inline void render_head(Stream &ss, std::string_view str) {
  ss.push_back('<');
  ss.append(str.data(), str.size());
  ss.push_back('>');
}

template <typename Stream, string_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_head(ss, name);
  ss.append(value.data(), value.size());
  render_tail(ss, name);
}

template <typename Stream, num_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_head(ss, name);
  char temp[65];
  auto p = detail::to_chars(temp, value);
  ss.append(temp, p - temp);
  render_tail(ss, name);
}

template <typename Stream, char_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_head(ss, name);
  ss.push_back(value);
  render_tail(ss, name);
}

template <typename Stream, bool_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_head(ss, name);
  ss.append(value ? "true" : "false");
  render_tail(ss, name);
}

template <typename Stream, enum_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  render_xml_value(ss, static_cast<std::underlying_type_t<T>>(value), name);
}

template <typename Stream, optional_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value(ss, *value, name);
  }
}

template <typename Stream, unique_ptr_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  if (value) {
    render_xml_value(ss, *value, name);
  }
}

template <typename Stream, sequence_container_t T>
IGUANA_INLINE void render_xml_value(Stream &ss, const T &value,
                                    std::string_view name) {
  for (const auto &v : value) {
    render_xml_value(ss, v, name);
  }
}

template <bool tag, typename Stream, refletable T>
IGUANA_INLINE void render_xml_value(Stream &ss, T &&t, std::string_view name) {
  if constexpr (tag) {
    render_head(ss, name);
  }
  for_each(std::forward<T>(t),
           [&](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_members(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             constexpr std::string_view tag_name =
                 std::string_view(get_name<std::decay_t<T>, Idx>().data(),
                                  get_name<std::decay_t<T>, Idx>().size());
             static_assert(Idx < Count);
             render_xml_value(ss, t.*v, tag_name);
           });
  if constexpr (tag) {
    render_tail(ss, name);
  }
}

template <typename Stream, refletable T>
IGUANA_INLINE void to_xml(T &&t, Stream &s) {
  render_xml_value<false>(s, t, "");
}

} // namespace iguana