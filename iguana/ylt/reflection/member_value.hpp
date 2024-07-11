#pragma once
#include "template_string.hpp"

#if __has_include(<concepts>) || defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)
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
  ((void)((i == Is) && ((set_member_ptr(member, &std::get<Is>(t))), true)),
   ...);
}
}  // namespace internal

template <typename Member, typename T>
inline Member& get_member_by_name(T& t, std::string_view name) {
  static constexpr auto map = get_member_names_map<T>();
  size_t index = map.at(name);  // may throw out_of_range: unknown key.
  auto ref_tp = object_to_tuple(t);

  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
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
  auto ref_tp = object_to_tuple(t);

  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
                         std::make_index_sequence<map.size()>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}

template <typename Member, typename T>
inline Member& get_member_by_index(T& t, size_t index) {
  auto ref_tp = object_to_tuple(t);
  constexpr size_t tuple_size = std::tuple_size_v<decltype(ref_tp)>;

  if (index >= tuple_size) {
    std::string str = "index out of range, ";
    str.append("index: ")
        .append(std::to_string(index))
        .append(" is greater equal than member count ")
        .append(std::to_string(tuple_size));
    throw std::out_of_range(str);
  }
  Member* member_ptr = nullptr;
  internal::tuple_switch(member_ptr, index, ref_tp,
                         std::make_index_sequence<tuple_size>{});
  if (member_ptr == nullptr) {
    throw std::invalid_argument(
        "given member type is not match the real member type");
  }
  return *member_ptr;
}

template <typename Member, size_t index, typename T>
inline Member& get_member_by_index(T& t) {
  auto ref_tp = object_to_tuple(t);

  static_assert(
      std::is_same_v<Member&, std::tuple_element_t<index, decltype(ref_tp)>>,
      "member type is not match");

  return std::get<index>(ref_tp);
}

template <typename T>
struct ylt_field {
  T& value;
  size_t index;
  std::string_view name;
};

template <typename T>
ylt_field(T& a, size_t, std::string_view) -> ylt_field<T>;

template <typename T, typename Visit>
inline void for_each(T& t, Visit func) {
  auto ref_tp = object_to_tuple(t);
  constexpr size_t tuple_size = std::tuple_size_v<decltype(ref_tp)>;
  constexpr auto arr = get_member_names<T>();
  [&]<size_t... Is>(std::index_sequence<Is...>) mutable {
    (func(ylt_field{std::get<Is>(ref_tp), Is, arr[Is]}), ...);
  }
  (std::make_index_sequence<tuple_size>{});
}

}  // namespace ylt::reflection

template <ylt::reflection::FixedString s>
inline constexpr auto operator""_ylts() {
  return s;
}
#endif
