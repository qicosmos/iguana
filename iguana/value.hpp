#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace iguana {

#define GCC_COMPILER (defined(__GNUC__) && !defined(__clang__))

#ifdef GCC_COMPILER
#include <map>
template <class Key, class T, typename... Args>
using json_map = std::map<Key, T, Args...>;
#else
template <class Key, class T, typename... Args>
using json_map = std::unordered_map<Key, T, Args...>;
#endif

enum dom_parse_error { ok, wrong_type };

template <typename CharT>
struct basic_json_value
    : std::variant<
          std::monostate, std::nullptr_t, bool, double, int,
          std::basic_string<CharT>, std::vector<basic_json_value<CharT>>,
          json_map<std::basic_string<CharT>, basic_json_value<CharT>>> {
  using string_type = std::basic_string<CharT>;
  using array_type = std::vector<basic_json_value<CharT>>;
  using object_type = json_map<string_type, basic_json_value<CharT>>;

  using base_type = std::variant<std::monostate, std::nullptr_t, bool, double,
                                 int, string_type, array_type, object_type>;

  using base_type::base_type;

  inline const static std::unordered_map<size_t, std::string> type_map_ = {
      {0, "undefined type"}, {1, "null type"},  {2, "bool type"},
      {3, "double type"},    {4, "int type"},   {5, "string type"},
      {6, "array type"},     {7, "object type"}};

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

  template <typename T>
  std::pair<std::error_code, T> get() const {
    std::error_code ec{};
    try {
      return std::make_pair(ec, std::get<T>(*this));
    } catch (std::exception &e) {
      auto it = type_map_.find(this->index());
      if (it == type_map_.end()) {
        ec = iguana::make_error_code(iguana::dom_errc::wrong_type,
                                     "undefined type");
      } else {
        ec = iguana::make_error_code(iguana::dom_errc::wrong_type, it->second);
      }
      return std::make_pair(ec, T{});
    }
  }

  template <typename T>
  std::error_code get_to(T &v) const {
    auto [ec, val] = get<T>();
    if (ec.value() == 0)
      v = std::move(val);
    return ec;
  }

  template <typename T>
  T _get_to_may_throw() const {
    auto [ec, v] = get<T>();
    if (ec.value())
      throw(ec);
    return v;
  }

  template <typename T>
  T _get_to_no_throw(std::error_code &ec) const {
    auto [code, v] = get<T>();
    if (code.value())
      ec = code;
    return v;
  }

  object_type to_object() const { return _get_to_may_throw<object_type>(); }
  object_type to_object(std::error_code &ec) const {
    return _get_to_no_throw<object_type>(ec);
  }

  array_type to_array() const { return _get_to_may_throw<array_type>(); }
  array_type to_array(std::error_code &ec) const {
    return _get_to_no_throw<array_type>(ec);
  }

  double to_double() const { return _get_to_may_throw<double>(); }
  double to_double(std::error_code &ec) const {
    return _get_to_no_throw<double>(ec);
  }

  int to_int() const { return _get_to_may_throw<int>(); }
  int to_int(std::error_code &ec) const { return _get_to_no_throw<int>(ec); }

  bool to_bool() const { return _get_to_may_throw<bool>(); }
  bool to_bool(std::error_code &ec) const { return _get_to_no_throw<bool>(ec); }

  std::basic_string<CharT> to_string() const {
    return _get_to_may_throw<std::basic_string<CharT>>();
  }
  std::basic_string<CharT> to_string(std::error_code &ec) const {
    return _get_to_no_throw<std::basic_string<CharT>>(ec);
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
