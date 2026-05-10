#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef __has_include
#define __has_include(x) 0
#endif

#if defined(IGUANA_ENABLE_CXX26_REFLECTION) && defined(__GNUC__) && \
    !defined(__clang__) && __has_include(<meta>)
#include <meta>
#endif

#if defined(IGUANA_ENABLE_CXX26_REFLECTION) && defined(__GNUC__) && \
    !defined(__clang__) && defined(__cpp_impl_reflection) &&         \
    __cpp_impl_reflection >= 202603L && defined(__cpp_lib_reflection) && \
    __cpp_lib_reflection >= 202603L
#define YLT_HAS_CXX26_REFLECTION 1
#else
#define YLT_HAS_CXX26_REFLECTION 0
#endif

namespace ylt::reflection::std_meta {

inline constexpr bool enabled = YLT_HAS_CXX26_REFLECTION;

template <typename T>
inline constexpr bool reflectable_v =
    enabled &&
    std::is_aggregate_v<std::remove_cv_t<std::remove_reference_t<T>>> &&
    std::is_class_v<std::remove_cv_t<std::remove_reference_t<T>>>;

#if YLT_HAS_CXX26_REFLECTION
namespace detail {
template <auto... vals>
struct meta_pack_type {
  template <typename F>
  constexpr decltype(auto) operator>>(F&& body) const {
    return std::forward<F>(body).template operator()<vals...>();
  }
};

template <auto... vals>
inline constexpr meta_pack_type<vals...> meta_pack = {};

template <typename Range>
consteval auto make_meta_pack(Range range) {
  std::vector<std::meta::info> args;
  for (auto value : range) {
    args.push_back(std::meta::reflect_constant(value));
  }
  return std::meta::substitute(^^meta_pack, args);
}

template <typename T>
consteval auto fields_of() {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  constexpr auto ctx = std::meta::access_context::current();
  return std::meta::nonstatic_data_members_of(^^U, ctx);
}
}  // namespace detail

template <typename T>
consteval std::size_t member_count() {
  return detail::fields_of<T>().size();
}

template <typename T>
consteval auto member_names() {
  std::array<std::string_view, member_count<T>()> names{};
  std::size_t index = 0;
  [: detail::make_meta_pack(detail::fields_of<T>()) :] >> [&]<auto... fields> {
    ((names[index++] = std::meta::identifier_of(fields)), ...);
  };
  return names;
}

template <typename T>
constexpr auto object_to_tuple(T&& value) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return [: detail::make_meta_pack(detail::fields_of<U>()) :]
      >> [&]<auto... fields>() {
           return std::tie(value.[:fields:]...);
         };
}

template <typename T>
constexpr auto member_ptr_tuple(T& value) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return [: detail::make_meta_pack(detail::fields_of<U>()) :]
      >> [&]<auto... fields>() {
           return std::make_tuple(&value.[:fields:]...);
         };
}

template <typename T>
constexpr auto member_offsets(T& value) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return [: detail::make_meta_pack(detail::fields_of<U>()) :]
      >> [&]<auto... fields>() {
           return std::array<std::size_t, sizeof...(fields)>{
               {static_cast<std::size_t>(
                   reinterpret_cast<const char*>(&value.[:fields:]) -
                   reinterpret_cast<const char*>(&value))...}};
         };
}

template <typename T, typename Visitor>
constexpr decltype(auto) visit_members(T&& value, Visitor&& visitor) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return [: detail::make_meta_pack(detail::fields_of<U>()) :]
      >> [&]<auto... fields>() -> decltype(auto) {
           return std::forward<Visitor>(visitor)(value.[:fields:]...);
         };
}
#else
template <typename T>
constexpr std::size_t member_count() {
  return 0;
}

template <typename T>
constexpr auto member_names() {
  return std::array<std::string_view, 0>{};
}

template <typename T>
constexpr auto object_to_tuple(T&&) {
  return std::tie();
}

template <typename T>
constexpr auto member_ptr_tuple(T&) {
  return std::tuple<>{};
}

template <typename T>
constexpr auto member_offsets(T&) {
  return std::array<std::size_t, 0>{};
}

template <typename T, typename Visitor>
constexpr void visit_members(T&&, Visitor&&) {
  static_assert(sizeof(T) == 0,
                "C++26 reflection backend is not enabled for this compiler");
}
#endif

}  // namespace ylt::reflection::std_meta
