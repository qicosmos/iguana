#pragma once
#include "reflection.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <rapidxml.hpp>
#include <string>

namespace iguana {
template <typename T> void do_read(rapidxml::xml_node<char> *node, T &&t);

template <typename T>
inline void parse_item(rapidxml::xml_node<char> *node, T &t, const char *value,
                       std::string_view key = "") {
  using U = std::remove_reference_t<T>;
  if constexpr (std::is_arithmetic_v<U>) {
    t = atoi(value);
  } else if constexpr (std::is_same_v<std::string, U>) {
    t = value;
  } else if constexpr (is_reflection_v<U>) {
    do_read(node->first_node(key.data()), t);
  } else {
    static_assert(!sizeof(T), "don't support this type!!");
  }
}

template <typename T>
inline void do_read(rapidxml::xml_node<char> *node, T &&t) {
  static_assert(is_reflection_v<std::remove_reference_t<T>>,
                "must be refletable object");
  static constexpr auto frozen_map = get_iguana_struct_map<T>();
  for (auto &[key, value] : frozen_map) {
    std::string_view str = key.data();
    auto n = node->first_node(key.data());
    if (!n) {
      continue;
    }

    std::visit(
        [&, str](auto &&member_ptr) {
          using V = std::decay_t<decltype(member_ptr)>;
          if constexpr (std::is_member_pointer_v<V>) {
            parse_item(node, t.*member_ptr, n->value(), str);
          } else {
            static_assert(!sizeof(V), "type not supported");
          }
        },
        value);
  }
}

template <typename T, typename = std::enable_if_t<is_reflection<T>::value>>
inline void from_xml(T &&t, char *buf) {
  rapidxml::xml_document<> doc;
  doc.parse<0>(buf);

  auto fisrt_node = doc.first_node();
  if (fisrt_node)
    do_read(fisrt_node, t);
}
} // namespace iguana