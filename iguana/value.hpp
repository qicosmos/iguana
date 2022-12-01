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
    : std::variant<std::monostate, std::nullptr_t, bool, double, int,
                   std::basic_string<CharT>,
                   std::vector<basic_json_value<CharT>>,
                   std::unordered_map<std::basic_string<CharT>,
                                      basic_json_value<CharT>>> {
  using string_type = std::basic_string<CharT>;
  using array_type = std::vector<basic_json_value<CharT>>;
  using object_type = std::unordered_map<string_type, basic_json_value<CharT>>;

  using base_type = std::variant<std::monostate, std::nullptr_t, bool, double,
                                 int, string_type, array_type, object_type>;

  using base_type::base_type;

  basic_json_value() : base_type(std::in_place_type<std::monostate>) {}

  basic_json_value(CharT const *value)
      : base_type(std::in_place_type<string_type>, value) {}

  base_type &base() { return *this; }
  base_type const &base() const { return *this; }

  bool is_undefined() const {
    return std::holds_alternative<std::monostate>(*this);
  }
  bool is_null() const { return std::holds_alternative<std::nullptr_t>(*this); }
  bool is_bool() const { return std::holds_alternative<bool>(*this); }
  bool is_double() const { return std::holds_alternative<double>(*this); }
  bool is_int() const { return std::holds_alternative<int>(*this); }
  bool is_number() const { return is_double() || is_int(); }
  bool is_string() const { return std::holds_alternative<string_type>(*this); }
  bool is_array() const { return std::holds_alternative<array_type>(*this); }
  bool is_object() const { return std::holds_alternative<object_type>(*this); }

  array_type to_array() const {
    if (is_array())
      return std::get<array_type>(*this);
    return {};
  }

  object_type to_object() const {
    if (is_object())
      return std::get<object_type>(*this);
    return {};
  }

  double to_double(bool *ok = nullptr) const {
    if (ok)
      *ok = true;
    if (is_double())
      return std::get<double>(*this);
    if (is_int())
      return static_cast<double>(std::get<int>(*this));
    if (ok)
      *ok = false;
    return {};
  }

  double to_int(bool *ok = nullptr) const {
    if (ok)
      *ok = true;
    if (is_double())
      return static_cast<int>(std::get<double>(*this));
    if (is_int())
      return std::get<int>(*this);
    if (ok)
      *ok = false;
    return {};
  }
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
