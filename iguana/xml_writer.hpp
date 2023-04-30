//
// Created by qiyu on 17-6-6.
//

#ifndef IGUANA_XML17_HPP
#define IGUANA_XML17_HPP
#include "reflection.hpp"
#include "type_traits.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <rapidxml_print.hpp>
#include <string.h>

namespace iguana::xml {
// to xml
template <typename Stream, typename T>
inline std::enable_if_t<is_reflection<T>::value, void>
to_xml_impl(Stream &s, T &&t, std::string_view name = "");

template <typename Stream, typename T>
inline std::enable_if_t<!is_reflection<T>::value, void>
to_xml_impl(Stream &s, T &&t, std::string_view name);

template <typename Stream, typename T>
inline std::enable_if_t<!std::is_floating_point<T>::value &&
                        (std::is_integral<T>::value ||
                         std::is_unsigned<T>::value ||
                         std::is_signed<T>::value)>
render_xml_value(Stream &ss, T value) {
  char temp[20];
  auto p = itoa_fwd(value, temp);
  ss.append(temp, p - temp);
}

template <typename Stream, typename T>
inline std::enable_if_t<std::is_floating_point<T>::value>
render_xml_value(Stream &ss, T value) {
  char temp[20];
  sprintf(temp, "%f", value);
  ss.append(temp);
}

template <typename Stream> inline void render_xml_value(Stream &ss, bool s) {
  if (s) {
    ss.append("true");
  } else {
    ss.append("false");
  }
}

template <typename Stream> inline void render_xml_value(Stream &ss, char s) {
  ss.push_back(s);
}

template <typename Stream>
inline void render_xml_value(Stream &ss, const std::string &s) {
  ss.append(s.c_str(), s.size());
}

template <typename Stream>
inline void render_xml_value(Stream &ss, const char *s) {
  ss.append(s, strlen(s));
}

template <typename Stream, typename T>
inline void render_xml_value(Stream &ss, const std::optional<T> &s) {
  if (s.has_value()) {
    render_xml_value(ss, *s);
  }
}

template <typename Stream, typename T>
inline void render_xml_value0(Stream &ss, const T &v, std::string_view name) {
  for (auto &item : v) {
    to_xml_impl(ss, item, name);
  }
}

template <typename Stream, typename T>
inline std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream &ss,
                                                                 T t) {
  ss.push_back('<');
  render_xml_value(ss, t);
  ss.push_back('>');
}

template <typename Stream>
inline void render_key(Stream &ss, const std::string &s) {
  render_xml_value(ss, s);
}

template <typename Stream> inline void render_key(Stream &ss, const char *s) {
  render_xml_value(ss, s);
}

template <typename Stream> inline void render_tail(Stream &ss, const char *s) {
  ss.push_back('<');
  ss.push_back('/');
  ss.append(s, strlen(s));
  ss.push_back('>');
}

template <typename Stream> inline void render_head(Stream &ss, const char *s) {
  ss.push_back('<');
  ss.append(s, strlen(s));
  ss.push_back('>');
}

template <typename Stream, typename T>
inline std::enable_if_t<!is_reflection<T>::value, void>
to_xml_impl(Stream &s, T &&t, std::string_view name) {
  if constexpr (!std::is_same_v<std::string, std::remove_cvref_t<T>> &&
                is_container<std::remove_cvref_t<T>>::value) {
    std::string_view sv = name.data();
    render_xml_value0(s, t, sv);
  } else {
    render_head(s, name.data());
    render_xml_value(s, t);
    render_tail(s, name.data());
  }
}

template <typename Stream, typename T>
inline std::enable_if_t<is_reflection<T>::value, void>
to_xml_impl(Stream &s, T &&t, std::string_view name) {
  if (name.empty()) {
    name = iguana::get_name<T>();
  }
  s.append("<").append(name).append(">");
  for_each(std::forward<T>(t), [&t, &s](const auto v, auto i) {
    using M = decltype(iguana_reflect_members(std::forward<T>(t)));
    constexpr auto Idx = decltype(i)::value;
    constexpr auto Count = M::value();
    static_assert(Idx < Count);

    using type_v = decltype(std::declval<T>().*std::declval<decltype(v)>());
    if constexpr (!is_reflection<type_v>::value) {
      if constexpr (!std::is_same_v<std::string,
                                    typename std::remove_cvref<type_v>::type> &&
                    is_container<type_v>::value) {
        std::string_view sv = get_name<T, Idx>().data();
        render_xml_value0(s, t.*v, sv);
      } else {
        render_head(s, get_name<T, Idx>().data());
        render_xml_value(s, t.*v);
        render_tail(s, get_name<T, Idx>().data());
      }
    } else {
      to_xml_impl(s, t.*v, get_name<T, Idx>().data());
    }
  });
  s.append("</").append(name).append(">");
}

template <typename Stream, typename T,
          typename = std::enable_if_t<is_reflection<T>::value>>
inline void to_xml(Stream &s, T &&t) {
  to_xml_impl(s, std::forward<T>(t));
}

template <int Flags = 0, typename Stream, typename T,
          typename = std::enable_if_t<is_reflection<T>::value>>
inline void to_xml_pretty(Stream &s, T &&t) {
  to_xml_impl(s, std::forward<T>(t));
  rapidxml::xml_document<> doc;
  doc.parse<Flags>(s.data());
  std::string ss;
  rapidxml::print(std::back_inserter(ss), doc);
  s = std::move(ss);
}
} // namespace iguana::xml
#endif // IGUANA_XML17_HPP
