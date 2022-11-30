#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace iguana {
template <typename CharT>
struct basic_json_value
    : std::variant<std::monostate, std::nullptr_t, bool, double,
                   std::basic_string<CharT>,
                   std::vector<basic_json_value<CharT>>,
                   std::unordered_map<std::basic_string<CharT>,
                                      basic_json_value<CharT>>> {
  using string_type = std::basic_string<CharT>;
  using array_type = std::vector<basic_json_value<CharT>>;
  using object_type = std::unordered_map<string_type, basic_json_value<CharT>>;

  using base_type = std::variant<std::monostate, std::nullptr_t, bool, double,
                                 string_type, array_type, object_type>;

  using base_type::base_type;

  basic_json_value() : base_type(std::in_place_type<std::monostate>) {}

  basic_json_value(CharT const *value)
      : base_type(std::in_place_type<string_type>, value) {}

  base_type &base() { return *this; }
  base_type const &base() const { return *this; }
};

template <typename CharT>
using basic_jarray = typename basic_json_value<CharT>::array_type;

template <typename CharT>
using basic_jobject = typename basic_json_value<CharT>::object_type;

template <typename CharT>
using basic_jpair = typename basic_jobject<CharT>::value_type;

using jvalue = basic_json_value<char>;
using jarray = basic_jarray<char>;
using jobject = basic_jobject<char>;
using jpair = basic_jpair<char>;

template <typename CharT>
void swap(basic_json_value<CharT> &lhs, basic_json_value<CharT> &rhs) noexcept {
  lhs.swap(rhs);
}

} // namespace iguana
