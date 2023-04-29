#pragma once
#include <type_traits>

namespace iguana {
template <class, class = void> struct is_container : std::false_type {};

template <class T>
struct is_container<
    T, std::void_t<decltype(std::declval<T>().size(), std::declval<T>().begin(),
                            std::declval<T>().end())>> : std::true_type {};

template <class, class = void> struct is_map_container : std::false_type {};

template <class T>
struct is_map_container<
    T, std::void_t<decltype(std::declval<typename T::mapped_type>())>>
    : public is_container<T> {};

template <typename T> constexpr inline bool is_std_optinal_v = false;

template <typename T>
constexpr inline bool is_std_optinal_v<std::optional<T>> = true;

template <typename T> constexpr inline bool is_std_variant_v = false;

template <typename... Args>
constexpr inline bool is_std_variant_v<std::variant<Args...>> = true;

template <typename T, typename C>
struct is_variant_contains_type : std::false_type {};

template <typename T, typename... Args>
struct is_variant_contains_type<T, std::variant<Args...>>
    : std::bool_constant<(std::is_same_v<T, std::remove_cvref_t<Args>> ||
                          ...)> {};

template <typename... Args> struct variant_first_match;

template <typename C, typename T> struct variant_first_match<C, T> {
  using type =
      std::conditional_t<is_variant_contains_type<T, C>::value, T, void>;
};

template <typename C, typename T, typename... Args>
struct variant_first_match<C, T, Args...> {
  using type =
      std::conditional_t<is_variant_contains_type<T, C>::value, T,
                         typename variant_first_match<C, Args...>::type>;
};

} // namespace iguana
