#pragma once
#ifdef YLT_USE_CXX26_REFLECTION
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "member_names.hpp"

namespace ylt::reflection::reflect26 {

inline constexpr std::string_view normalized_member_name(
    std::string_view name) {
  if (name.size() > 3 && name[0] == '_' && name[1] == '_' && name[2] == '_') {
    name.remove_prefix(3);
  }
  return name;
}

template <typename T, std::size_t... Is>
constexpr auto member_index_map_impl(std::index_sequence<Is...>) {
  constexpr auto names = ylt::reflection::get_member_names<T>();
  return frozen::unordered_map<frozen::string, std::size_t, sizeof...(Is)>{
      {normalized_member_name(names[Is]), Is}...};
}

template <typename T>
constexpr auto member_index_map() {
  return member_index_map_impl<T>(
      std::make_index_sequence<ylt::reflection::members_count_v<T>>{});
}

template <typename T, typename Func>
void dispatch_by_runtime_index(T& obj, std::size_t target, Func& func) {
  static constexpr auto members = std::define_static_array(
      data_members_26<ylt::reflection::remove_cvref_t<T>>());
  bool done = false;
  [[maybe_unused]] std::size_t index = 0;
  template for (constexpr auto member : members) {
    if (!done && index == target) {
      func(obj.[:member:]);
      done = true;
    }
    ++index;
  }
}

template <typename T, typename Func>
bool dispatch_by_name(T& obj, std::string_view key, Func&& func) {
  using U = ylt::reflection::remove_cvref_t<T>;
  if constexpr (ylt::reflection::members_count_v<U> == 0) {
    return false;
  }
  else {
    static constexpr auto map = member_index_map<U>();
    auto it = map.find(key);
    if (it == map.end()) {
      return false;
    }
    dispatch_by_runtime_index(obj, it->second, func);
    return true;
  }
}

template <std::size_t Index, typename T, typename Func>
void dispatch_by_index(T& obj, Func&& func) {
  static constexpr auto members = std::define_static_array(
      data_members_26<ylt::reflection::remove_cvref_t<T>>());
  static_assert(Index < members.size(), "index out of range");
  func(obj.[:members[Index]:]);
}

}  // namespace ylt::reflection::reflect26
#endif  // YLT_USE_CXX26_REFLECTION
