#pragma once
#include "detail/fast_float.h"
#include "reflection.hpp"
#include "type_traits.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <functional>
#include <optional>
#include <rapidxml.hpp>
#include <string>
#include <type_traits>

namespace iguana::xml {
template <typename T> constexpr inline bool is_std_optinal_v = false;

template <typename T>
constexpr inline bool is_std_optinal_v<std::optional<T>> = true;

template <typename T> void do_read(rapidxml::xml_node<char> *node, T &&t);

template <typename T>
inline void parse_item(rapidxml::xml_node<char> *node, T &t,
                       std::string_view value) {
  using U = std::remove_reference_t<T>;
  if constexpr (std::is_same_v<char, U>) {
    if (!value.empty())
      t = value.back();
  } else if constexpr (std::is_arithmetic_v<U>) {
    if constexpr (std::is_same_v<bool, U>) {
      if (value == "true") {
        t = true;
      } else if (value == "false") {
        t = false;
      } else {
        throw std::invalid_argument("Failed to parse bool");
      }
    } else {
      double num;
      auto [p, ec] = fast_float::from_chars(value.data(),
                                            value.data() + value.size(), num);
      if (ec != std::errc{})
        throw std::invalid_argument("Failed to parse number");
      t = static_cast<T>(num);
    }
  } else if constexpr (std::is_same_v<std::string, U>) {
    t = value;
  } else if constexpr (is_reflection_v<U>) {
    do_read(node, t);
  } else if constexpr (is_std_optinal_v<U>) {
    if (!value.empty()) {
      using value_type = typename U::value_type;
      value_type opt;
      parse_item(node, opt, value);
      t = std::move(opt);
    }
  } else if constexpr (std::is_same_v<
                           std::unordered_map<std::string, std::string>, U>) {
    rapidxml::xml_attribute<> *attr = node->first_attribute();
    while (attr != nullptr) {
      t.emplace(attr->name(), attr->value());
      attr = attr->next_attribute();
    }
  } else {
    static_assert(!sizeof(T), "don't support this type!!");
  }
}

template <typename T>
inline void parse_attribute(rapidxml::xml_node<char> *node, T &t) {
  using U = std::remove_reference_t<T>;
  if constexpr (is_std_optinal_v<U>) {
    using value_type = typename U::value_type;
    value_type opt;
    parse_attribute(node, opt);
    t = std::move(opt);
  }
  if constexpr (std::is_same_v<std::unordered_map<std::string, std::string>,
                               U>) {
    rapidxml::xml_attribute<> *attr = node->first_attribute();
    while (attr != nullptr) {
      t.emplace(attr->name(), attr->value());
      attr = attr->next_attribute();
    }
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

    if (key == std::string_view("_attribute")) {
      std::visit(
          [&, str](auto &&member_ptr) {
            using V = std::decay_t<decltype(member_ptr)>;
            if constexpr (std::is_member_pointer_v<V>) {
              parse_attribute(node, t.*member_ptr);
            } else {
              static_assert(!sizeof(V), "type not supported");
            }
          },
          value);
      continue;
    }

    if (!n) {
      continue;
    }

    std::visit(
        [&, str](auto &&member_ptr) {
          using V = std::decay_t<decltype(member_ptr)>;
          using type_v =
              decltype(std::declval<T>().*std::declval<decltype(member_ptr)>());
          using item_type = std::remove_cvref_t<type_v>;

          if constexpr (std::is_member_pointer_v<V>) {
            if constexpr (!std::is_same_v<std::string, item_type> &&
                          is_container<item_type>::value) {
              using value_type = typename item_type::value_type;
              value_type item;
              while (n) {
                if (n->name() != str) {
                  break;
                }
                parse_item(n, item,
                           std::string_view(n->value(), n->value_size()));
                (t.*member_ptr).push_back(item);
                n = n->next_sibling();
              }

            } else {
              parse_item(node->first_node(str.data()), t.*member_ptr,
                         std::string_view(n->value(), n->value_size()));
            }
          } else {
            static_assert(!sizeof(V), "type not supported");
          }
        },
        value);
  }
}

template <int Flags = 0, typename T,
          typename = std::enable_if_t<is_reflection<T>::value>>
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
} // namespace iguana::xml