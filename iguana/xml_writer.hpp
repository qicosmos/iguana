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
#include <msstl/charconv.hpp>
#include <rapidxml_print.hpp>
#include <string.h>

namespace iguana::xml {
// to xml
template <typename Stream, typename T>
inline void to_xml_impl(Stream &s, T &&t, std::string_view name = "");

class any_t;

template <typename Stream, typename T>
inline std::enable_if_t<std::is_arithmetic_v<T>> render_xml_value(Stream &ss,
                                                                  T &value) {
  char temp[40];
  auto [p, ec] = msstl::to_chars(temp, temp + sizeof(temp), value);
  const auto n = std::distance(temp, p);
  ss.append(temp, n);
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

template <typename Stream>
inline void render_xml_value(Stream &ss, const any_t &t) {
  ss.append(t.get_value().data(), t.get_value().size());
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
inline void render_xml_node(Stream &ss, std::string_view name, T &&item) {
  render_head(ss, name.data());
  render_xml_value(ss, std::forward<T>(item));
  render_tail(ss, name.data());
}

template <typename Stream, typename T>
inline void render_xml_value0(Stream &ss, const T &v, std::string_view name) {
  for (auto &item : v) {
    using item_type = std::decay_t<decltype(item)>;
    if constexpr (is_reflection_v<item_type>) {
      to_xml_impl(ss, item, name);
    } else {
      render_xml_node(ss, name, item);
    }
  }
}

template <typename Stream, typename T>
inline void to_xml_impl(Stream &s, T &&t, std::string_view name) {
  if (name.empty()) {
    name = iguana::get_name<T>();
  }
  s.append("<").append(name);
  constexpr auto Idx = get_type_index<is_map_container, std::decay_t<T>>();
  if constexpr (Idx != iguana::get_value<std::decay_t<T>>()) {
    auto attr_value = get<Idx>(t);
    for (auto &[k, v] : attr_value) {
      s.append(" ").append(k).append("=\"");
      render_xml_value(s, v);
      s.append("\"");
    }
  }
  s.append(">");
  for_each(std::forward<T>(t), [&t, &s](const auto v, auto i) {
    using M = decltype(iguana_reflect_members(std::forward<T>(t)));
    constexpr auto Idx = decltype(i)::value;
    constexpr auto Count = M::value();
    static_assert(Idx < Count);

    using type_v = decltype(std::declval<T>().*std::declval<decltype(v)>());
    if constexpr (!is_reflection<type_v>::value) {
      if constexpr (is_map_container<std::decay_t<type_v>>::value) {
        return;
      } else if constexpr (!std::is_same_v<std::string, std::decay_t<type_v>> &&
                           is_container<type_v>::value) {
        std::string_view sv = get_name<T, Idx>().data();
        render_xml_value0(s, t.*v, sv);
      } else {
        render_xml_node(s, get_name<T, Idx>().data(), t.*v);
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
inline bool to_xml_pretty(Stream &s, T &&t) {
  to_xml_impl(s, std::forward<T>(t));

  bool r = true;
  try {
    rapidxml::xml_document<> doc;
    doc.parse<Flags>(s.data());
    std::string ss;
    rapidxml::print(std::back_inserter(ss), doc);
    s = std::move(ss);
  } catch (std::exception &e) {
    r = false;
    std::cerr << e.what() << "\n";
  }

  return r;
}
} // namespace iguana::xml
#endif // IGUANA_XML17_HPP