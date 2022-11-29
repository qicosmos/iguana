#pragma once
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace iguana {
template <typename CharT>
struct json_value
    : std::variant<std::monostate, std::nullptr_t, bool, double,
                   std::basic_string<CharT>, std::vector<json_value<CharT>>,
                   std::map<std::basic_string<CharT>, json_value<CharT>>> {
  using string_type = std::basic_string<CharT>;
  using array_type = std::vector<json_value<CharT>>;
  using object_type = std::map<string_type, json_value<CharT>>;

  using base_type = std::variant<std::monostate, std::nullptr_t, bool, double,
                                 string_type, array_type, object_type>;

  using base_type::base_type;

  json_value() : base_type(std::in_place_type<std::monostate>) {}

  json_value(CharT const *value)
      : base_type(std::in_place_type<string_type>, value) {}

  base_type &base() { return *this; }
  base_type const &base() const { return *this; }
};

template <typename CharT> using jarray = typename json_value<CharT>::array_type;

template <typename CharT>
using json_object = typename json_value<CharT>::object_type;

template <typename CharT> using pair = typename json_object<CharT>::value_type;

template <typename CharT>
void swap(json_value<CharT> &lhs, json_value<CharT> &rhs) noexcept {
  lhs.swap(rhs);
}

} // namespace iguana
