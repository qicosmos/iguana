#pragma once
#include "field_reflection.hpp"
#include "member_names.hpp"

namespace ylt::reflection {

namespace internal {
template <typename Member, typename T>
inline void set_member_ptr(Member& member, T t) {
  if constexpr (std::is_same_v<Member, T>) {
    member = t;
  }
  else {
    std::string str = "given type: ";
    str.append(iguana::type_string<std::remove_pointer_t<Member>>());
    str.append(" is not equal with real type: ")
        .append(iguana::type_string<std::remove_pointer_t<T>>());
    throw std::invalid_argument(str);
  }
}

template <typename Member, class Tuple, std::size_t... Is>
inline void tuple_switch(Member& member, std::size_t i, Tuple& t,
                         std::index_sequence<Is...>) {
  ((void)((i == Is) && ((set_member_ptr(member, std::get<Is>(t))), true)), ...);
}
}  // namespace internal

template <std::size_t N>
struct FixedString {
  char data[N];
  template <std::size_t... I>
  constexpr FixedString(const char (&s)[N], std::index_sequence<I...>)
      : data{s[I]...} {}
  constexpr FixedString(const char (&s)[N])
      : FixedString(s, std::make_index_sequence<N>()) {}
};

template <typename Member, typename T>
inline Member& get_member_by_name(T& t, std::string_view name) {
  static constexpr auto map = get_member_names_map<T>();
  size_t index = map.at(name);  // may throw out_of_range unknown key.
  auto ptr_tp = object_to_tuple(t);

  Member* member_ptr = nullptr;
  tuple_switch(member_ptr, index, ptr_tp,
               std::make_index_sequence<map.size()>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}

template <typename Member, FixedString name, typename T>
inline Member& get_member_by_name(T& t) {
  static constexpr auto map = get_member_names_map<T>();
  static constexpr size_t index = map.at(name.data);
  auto ptr_tp = object_to_tuple(t);

  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ptr_tp,
                         std::make_index_sequence<map.size()>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}
}  // namespace ylt::reflection

template <ylt::reflection::FixedString s>
inline constexpr auto operator""_ylts() {
  return s;
}