#pragma once
#include <array>
#include <string_view>

#include "frozen/string.h"
#include "frozen/unordered_map.h"

namespace iguana {
template <typename T> constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <auto T> constexpr std::string_view get_raw_name() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

template <typename T> inline constexpr std::string_view type_string() {
  constexpr std::string_view sample = get_raw_name<int>();
  constexpr size_t pos = sample.find("int");
  constexpr std::string_view str = get_raw_name<T>();
  constexpr auto next1 = str.rfind(sample[pos + 3]);
#if defined(_MSC_VER)
  constexpr auto s1 = str.substr(pos + 6, next1 - pos - 6);
#else
  constexpr auto s1 = str.substr(pos, next1 - pos);
#endif
  return s1;
}

template <auto T> inline constexpr std::string_view enum_string() {
  constexpr std::string_view sample = get_raw_name<int>();
  constexpr size_t pos = sample.find("int");
  constexpr std::string_view str = get_raw_name<T>();
  constexpr auto next1 = str.rfind(sample[pos + 3]);
#if defined(__clang__) || defined(_MSC_VER)
  constexpr auto s1 = str.substr(pos, next1 - pos);
#else
  constexpr auto s1 = str.substr(pos + 5, next1 - pos - 5);
#endif
  return s1;
}

#if defined(__clang__) && (__clang_major__ >= 17)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

template <typename E, E V> constexpr std::string_view get_raw_name_with_v() {
#ifdef _MSC_VER
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

// True means the V is a valid enum value, and the second of the result is the
// name
template <typename E, E V>
constexpr std::pair<bool, std::string_view> try_get_enum_name() {
  constexpr std::string_view sample_raw_name = get_raw_name_with_v<int, 5>();
  constexpr size_t pos = sample_raw_name.find("5");
  constexpr std::string_view enum_raw_name = get_raw_name_with_v<E, V>();
  constexpr auto enum_end = enum_raw_name.rfind(&sample_raw_name[pos + 1]);
#ifdef _MSC_VER
  constexpr auto enum_begin = enum_raw_name.rfind(',', enum_end) + 1;
#else
  constexpr auto enum_begin = enum_raw_name.rfind(' ', enum_end) + 1;
#endif
  constexpr std::string_view res =
      enum_raw_name.substr(enum_begin, enum_end - enum_begin);

  constexpr size_t pos_brackets = res.find(')');

  size_t pos_colon = res.find("::");
  return {pos_brackets == std::string_view::npos,
          res.substr(pos_colon == std::string_view::npos ? 0 : pos_colon + 2)};
}

// get the size of integer_sequence
template <typename T, T... I>
constexpr std::size_t integer_sequence_size(std::integer_sequence<T, I...>) {
  return sizeof...(I);
}

// Enumerate the numbers in a integer sequence to see if they are legal enum
// value
template <typename E, std::int64_t... Is>
constexpr inline auto
get_enum_arr(const std::integer_sequence<std::int64_t, Is...> &) {
  constexpr std::size_t N = sizeof...(Is);
  std::array<std::string_view, N> enum_names = {};
  std::array<E, N> enum_values = {};
  std::size_t num = 0;
  (([&]() {
     constexpr auto res = try_get_enum_name<E, static_cast<E>(Is)>();
     if constexpr (res.first) {
       //  the Is is a valid enum value
       enum_names[num] = res.second;
       enum_values[num] = static_cast<E>(Is);
       ++num;
     }
   })(),
   ...);
  return std::make_tuple(num, enum_values, enum_names);
}

template <std::int64_t... I1, std::int64_t... I2>
constexpr auto
concatenate_sequences(std::integer_sequence<std::int64_t, I1...>,
                      std::integer_sequence<std::int64_t, I2...>) {
  return std::integer_sequence<std::int64_t, I1..., I2...>{};
}

// convert the array to integer sequence, number in [0, Filter) will be ignore
template <std::size_t Filter, std::int64_t N, const std::array<int, N> &arr,
          std::int64_t I = N - 1>
constexpr auto array_to_seq() {
  if constexpr (I > 0) {
    if constexpr (arr[I] < Filter && arr[I] >= 0) {
      return array_to_seq<Filter, N, arr, I - 1>();
    } else {
      return concatenate_sequences(
          std::integer_sequence<std::int64_t, arr[I]>{},
          array_to_seq<Filter, N, arr, I - 1>());
    }
  } else {
    if constexpr (arr[I] < Filter && arr[I] >= 0) {
      return std::integer_sequence<std::int64_t>();
    } else {
      return std::integer_sequence<std::int64_t, arr[I]>();
    }
  }
}

// convert array to map
template <typename E, size_t N, size_t... Is>
constexpr inline auto
get_enum_to_str_map(const std::array<std::string_view, N> &enum_names,
                    const std::array<E, N> &enum_values,
                    const std::index_sequence<Is...> &) {
  return frozen::unordered_map<E, frozen::string, sizeof...(Is)>{
      {enum_values[Is], enum_names[Is]}...};
}

template <typename E, size_t N, size_t... Is>
constexpr inline auto
get_str_to_enum_map(const std::array<std::string_view, N> &enum_names,
                    const std::array<E, N> &enum_values,
                    const std::index_sequence<Is...> &) {
  return frozen::unordered_map<frozen::string, E, sizeof...(Is)>{
      {enum_names[Is], enum_values[Is]}...};
}

// the default generic enum_value
// if the user has not defined a specialization template, this will be called
template <typename T> struct enum_value {
  constexpr static std::array<int, 0> value = {};
};

// by default, numbers in the range [0, Filter) will be enumerated.
template <bool str_to_enum, typename E, size_t Filter = 50>
constexpr inline auto get_enum_map() {
  constexpr auto default_seq =
      std::make_integer_sequence<std::int64_t, Filter>();
  constexpr auto &arr = enum_value<E>::value;
  constexpr auto arr_size = arr.size();
  if constexpr (arr_size > 0) {
    // the user has defined a specialization template
    constexpr auto arr_seq = array_to_seq<Filter, arr_size, arr>();
    constexpr auto all_seq = concatenate_sequences(arr_seq, default_seq);
    constexpr auto seq_size = integer_sequence_size(all_seq);
    constexpr auto t = get_enum_arr<E>(all_seq);
    if constexpr (str_to_enum) {
      return get_str_to_enum_map<E, seq_size>(
          std::get<2>(t), std::get<1>(t),
          std::make_index_sequence<std::get<0>(t)>{});
    } else {
      return get_enum_to_str_map<E, seq_size>(
          std::get<2>(t), std::get<1>(t),
          std::make_index_sequence<std::get<0>(t)>{});
    }
  } else {
    constexpr auto t = get_enum_arr<E>(default_seq);
    if constexpr (str_to_enum) {
      return get_str_to_enum_map<E, Filter>(
          std::get<2>(t), std::get<1>(t),
          std::make_index_sequence<std::get<0>(t)>{});
    } else {
      return get_enum_to_str_map<E, Filter>(
          std::get<2>(t), std::get<1>(t),
          std::make_index_sequence<std::get<0>(t)>{});
    }
  }
}
#if defined(__clang__) && (__clang_major__ >= 17)
#pragma clang diagnostic pop
#endif

} // namespace iguana