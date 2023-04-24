#pragma once
#include "detail/fast_float.h"
#include "reflection.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <functional>
#include <optional>
#include <rapidxml.hpp>
#include <string>

namespace iguana {
template <typename T> constexpr inline bool is_std_optinal_v = false;

template <typename T>
constexpr inline bool is_std_optinal_v<std::optional<T>> = true;

template <typename T> void do_read(rapidxml::xml_node<char> *node, T &&t);

template <typename T>
inline void parse_item(rapidxml::xml_node<char> *node, T &t,
                       std::string_view value, std::string_view key) {
  using U = std::remove_reference_t<T>;
  if constexpr (std::is_same_v<char, U>) {
    if (!value.empty())
      t = value.back();
  } else if constexpr (std::is_arithmetic_v<U>) {
    double num;
    auto [p, ec] =
        fast_float::from_chars(value.data(), value.data() + value.size(), num);
    if (ec != std::errc{})
      throw std::invalid_argument("Failed to parse number");
    t = static_cast<T>(num);
  } else if constexpr (std::is_same_v<std::string, U>) {
    t = value;
  } else if constexpr (is_reflection_v<U>) {
    do_read(node->first_node(key.data()), t);
  } else if constexpr (is_std_optinal_v<U>) {
    if (!value.empty()) {
      using value_type = typename U::value_type;
      value_type opt;
      parse_item(node, opt, value, key);
      t = std::move(opt);
    }
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
            parse_item(node, t.*member_ptr,
                       std::string_view(n->value(), n->value_size()), str);
          } else {
            static_assert(!sizeof(V), "type not supported");
          }
        },
        value);
  }
}

template <int Flags = 0, typename T, typename = std::enable_if_t<is_reflection<T>::value>>
inline bool from_xml(T &&t, char *buf) {
  try {
    rapidxml::xml_document<> doc;
    doc.parse<Flags>(buf);

    auto fisrt_node = doc.first_node();
    if (fisrt_node)
      do_read(fisrt_node, t);

    return true;
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }

  return false;
}
} // namespace iguana