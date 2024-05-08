#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "detail/pb_type.hpp"
#include "reflection.hpp"
#include "util.hpp"

namespace iguana {

enum class WireType : uint32_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimeted = 2,
  StartGroup = 3,
  EndGroup = 4,
  Fixed32 = 5,
  Unknown
};

template <size_t Idx, typename V>
struct one_of_t {
  static constexpr bool is_one_of_v = true;
  using value_type = std::optional<
      std::remove_reference_t<decltype(std::get<Idx>(std::declval<V>()))>>;
  value_type value;
};

namespace detail {
template <typename T>
constexpr bool is_fixed_v =
    std::is_same_v<T, fixed32_t> || std::is_same_v<T, fixed64_t> ||
    std::is_same_v<T, sfixed32_t> || std::is_same_v<T, sfixed64_t>;

template <typename T>
constexpr bool is_signed_varint_v =
    std::is_same_v<T, sint32_t> || std::is_same_v<T, sint64_t>;

template <typename Type, typename = void>
struct is_one_of_t : std::false_type {};

template <typename T>
struct is_one_of_t<T, std::void_t<decltype(T::is_one_of_v)>> : std::true_type {
};

template <typename T>
constexpr bool is_one_of_v = is_one_of_t<T>::value;

template <typename T>
constexpr inline WireType get_wire_type() {
  if constexpr (std::is_integral_v<T> || is_signed_varint_v<T> ||
                std::is_enum_v<T> || std::is_same_v<T, bool>) {
    return WireType::Varint;
  }
  else if constexpr (std::is_same_v<T, fixed32_t> ||
                     std::is_same_v<T, sfixed32_t> ||
                     std::is_same_v<T, float>) {
    return WireType::Fixed32;
  }
  else if constexpr (std::is_same_v<T, fixed64_t> ||
                     std::is_same_v<T, sfixed64_t> ||
                     std::is_same_v<T, double>) {
    return WireType::Fixed64;
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view> ||
                     is_reflection_v<T> || is_sequence_container<T>::value ||
                     is_map_container<T>::value) {
    return WireType::LengthDelimeted;
  }
  else if constexpr (optional_v<T> || is_one_of_v<T>) {
    return get_wire_type<typename T::value_type>();
  }
  else {
    throw std::runtime_error("unknown type");
  }
}

template <typename T>
constexpr bool is_lenprefix_v = (get_wire_type<T>() ==
                                 WireType::LengthDelimeted);

[[nodiscard]] inline uint32_t encode_zigzag(int32_t v) {
  return (static_cast<uint32_t>(v) << 1U) ^
         static_cast<uint32_t>(
             -static_cast<int32_t>(static_cast<uint32_t>(v) >> 31U));
}
[[nodiscard]] inline uint64_t encode_zigzag(int64_t v) {
  return (static_cast<uint64_t>(v) << 1U) ^
         static_cast<uint64_t>(
             -static_cast<int64_t>(static_cast<uint64_t>(v) >> 63U));
}

[[nodiscard]] inline int64_t decode_zigzag(uint64_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}
[[nodiscard]] inline int64_t decode_zigzag(uint32_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}

template <class T>
inline uint64_t decode_varint(T& data, size_t& pos) {
  const int8_t* begin = reinterpret_cast<const int8_t*>(data.data());
  const int8_t* end = begin + data.size();
  const int8_t* p = begin;
  uint64_t val = 0;

  // end is always greater than or equal to begin, so this subtraction is safe
  if (size_t(end - begin) >= 10) {  // fast path
    int64_t b;
    do {
      b = *p++;
      val = (b & 0x7f);
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 7;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 14;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 21;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 28;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 35;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 42;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 49;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 56;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x01) << 63;
      if (b >= 0) {
        break;
      }
      throw std::invalid_argument("Invalid varint value: too many bytes.");
    } while (false);
  }
  else {
    int shift = 0;
    while (p != end && *p < 0) {
      val |= static_cast<uint64_t>(*p++ & 0x7f) << shift;
      shift += 7;
    }
    if (p == end) {
      throw std::invalid_argument("Invalid varint value: too few bytes.");
    }
    val |= static_cast<uint64_t>(*p++) << shift;
  }

  pos = (p - begin);
  return val;
}

template <typename Stream>
inline void serialize_varint(uint64_t v, Stream& out) {
  while (v >= 0x80) {
    out.push_back(static_cast<uint8_t>(v | 0x80));
    v >>= 7;
  }
  out.push_back(static_cast<uint8_t>(v));
}

inline uint32_t log2_floor_uint32(uint32_t n) {
#if defined(__GNUC__)
  return 31 ^ static_cast<uint32_t>(__builtin_clz(n));
#else
  unsigned long where;
  _BitScanReverse(&where, n);
  return where;
#endif
}

inline size_t variant_uint32_size(uint32_t value) {
  // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
  // Use an explicit multiplication to implement the divide of
  // a number in the 1..31 range.
  // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
  // undefined.
  uint32_t log2value = log2_floor_uint32(value | 0x1);
  return static_cast<size_t>((log2value * 9 + 73) / 64);
}

// value == 0 ? 1 : floor(log2(value)) / 7 + 1
constexpr size_t variant_uint32_size_constexpr(uint32_t value) {
  if (value == 0) {
    return 1;
  }
  int log = 0;
  while (value >>= 1) ++log;
  return log / 7 + 1;
}

inline uint32_t log2_floor_uint64(uint64_t n) {
#if defined(__GNUC__)
  return 63 ^ static_cast<uint32_t>(__builtin_clzll(n));
#else
  unsigned long where;
  _BitScanReverse64(&where, n);
  return where;
#endif
}

inline size_t variant_uint64_size(uint64_t value) {
  // This computes value == 0 ? 1 : floor(log2(value)) / 7 + 1
  // Use an explicit multiplication to implement the divide of
  // a number in the 1..63 range.
  // Explicit OR 0x1 to avoid calling Bits::Log2FloorNonZero(0), which is
  // undefined.
  uint32_t log2value = log2_floor_uint64(value | 0x1);
  return static_cast<size_t>((log2value * 9 + 73) / 64);
}

template <typename U>
constexpr inline size_t variant_intergal_size(U value) {
  using T = std::remove_reference_t<U>;
  if constexpr (sizeof(T) == 8) {
    return variant_uint64_size(static_cast<uint64_t>(value));
  }
  else if constexpr (sizeof(T) == 4) {
    if constexpr (std::is_same_v<int32_t, T>) {
      if (value < 0) {
        return 10;
      }
    }
    return variant_uint32_size(static_cast<uint32_t>(value));
  }
  else {
    static_assert(!sizeof(T), "intergal in not supported");
  }
}

template <typename F, size_t... I>
constexpr void for_each_n(F&& f, std::index_sequence<I...>) {
  (std::forward<F>(f)(std::integral_constant<size_t, I>{}), ...);
}

// cache the size of reflection type
template <typename T>
auto& get_set_size_cache(T& t) {
  static std::map<size_t, size_t> cache;
  return cache[reinterpret_cast<size_t>(&t)];
}

template <bool omit_default_val, size_t key_size, typename T>
inline size_t numeric_size(T&& t) {
  using value_type = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (omit_default_val) {
    if constexpr (is_fixed_v<value_type> || is_signed_varint_v<value_type>) {
      if (t.val == 0) {
        return 0;
      }
    }
    else {
      if (t == static_cast<value_type>(0)) {
        return 0;
      }
    }
  }
  if constexpr (std::is_integral_v<value_type>) {
    if constexpr (std::is_same_v<bool, value_type>) {
      return 1 + key_size;
    }
    else {
      return key_size + variant_intergal_size(t);
    }
  }
  else if constexpr (detail::is_signed_varint_v<value_type>) {
    return key_size + variant_intergal_size(encode_zigzag(t.val));
  }
  else if constexpr (detail::is_fixed_v<value_type>) {
    return key_size + sizeof(typename value_type::value_type);
  }
  else if constexpr (std::is_same_v<value_type, double> ||
                     std::is_same_v<value_type, float>) {
    return key_size + sizeof(value_type);
  }
  else if constexpr (std::is_enum_v<value_type>) {
    using U = std::underlying_type_t<value_type>;
    return key_size + variant_intergal_size(static_cast<U>(t));
  }
  else {
    static_assert(!sizeof(value_type), "err");
  }
}

template <size_t key_size, typename T>
inline size_t pb_key_value_size(T&& t);

template <size_t field_no, typename T>
inline size_t pb_oneof_size(T&& t) {
  int len = 0;
  std::visit(
      [&len](auto&& value) {
        using value_type =
            std::remove_const_t<std::remove_reference_t<decltype(value)>>;
        constexpr uint32_t key =
            (field_no << 3) |
            static_cast<uint32_t>(get_wire_type<value_type>());
        len = pb_key_value_size<variant_uint32_size_constexpr(key)>(
            std::forward<value_type>(value));
      },
      std::forward<T>(t));
  return len;
}

// returns size = key_size + optional(len_size) + len
// when key_size == 0, return len
template <size_t key_size, typename T>
inline size_t pb_key_value_size(T&& t) {
  using value_type = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (is_reflection_v<value_type> ||
                is_custom_reflection_v<value_type>) {
    size_t len = 0;
    constexpr auto tuple = get_field_tuple<value_type>();
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&len, &t](auto i) {
          constexpr static auto tp = get_field_tuple<value_type>();
          constexpr auto value = std::get<decltype(i)::value>(tp);
          using U = typename std::decay_t<decltype(value.value(t))>;
          if constexpr (variant_v<U>) {
            len += pb_oneof_size<value.field_no>(value.value(t));
          }
          else {
            constexpr uint32_t sub_key =
                (value.field_no << 3) |
                static_cast<uint32_t>(get_wire_type<U>());
            constexpr auto sub_keysize = variant_uint32_size_constexpr(sub_key);
            len += pb_key_value_size<sub_keysize>(value.value(t));
          }
        },
        std::make_index_sequence<SIZE>{});
    get_set_size_cache(t) = len;
    if constexpr (key_size == 0) {
      // for top level
      return len;
    }
    else {
      if (len == 0) {
        return 0;
      }
      else {
        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
      }
    }
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    size_t len = 0;
    if constexpr (is_lenprefix_v<item_type>) {
      for (auto& item : t) {
        len += pb_key_value_size<key_size>(item);
      }
      return len;
    }
    else {
      for (auto& item : t) {
        // here 0 to get pakced size
        len += numeric_size<false, 0>(item);
      }
      if (len == 0) {
        return 0;
      }
      else {
        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
      }
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    size_t len = 0;
    for (auto& [k, v] : t) {
      // the key_size of  k and v  is constant 1
      size_t kv_len = pb_key_value_size<1>(k) + pb_key_value_size<1>(v);
      len += key_size + variant_uint32_size(static_cast<uint32_t>(kv_len)) +
             kv_len;
    }
    return len;
  }
  else if constexpr (optional_v<value_type>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb_key_value_size<key_size>(*t);
  }
  else if constexpr (is_one_of_v<value_type>) {
    return pb_key_value_size<key_size>(t.value);
  }
  else if constexpr (std::is_same_v<value_type, std::string> ||
                     std::is_same_v<value_type, std::string_view>) {
    if (t.size() == 0) {
      return 0;
    }
    if constexpr (key_size == 0) {
      return t.size();
    }
    else {
      return key_size + variant_uint32_size(static_cast<uint32_t>(t.size())) +
             t.size();
    }
  }
  else {
    return numeric_size<true, key_size>(t);
  }
}

// return the payload size
template <typename T>
inline size_t pb_value_size(T&& t) {
  using value_type = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (is_reflection_v<value_type>) {
    return get_set_size_cache(t);
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    size_t len = 0;
    if constexpr (!is_lenprefix_v<item_type>) {
      for (auto& item : t) {
        len += numeric_size<false, 0>(item);
      }
      return len;
    }
    else {
      static_assert(!sizeof(item_type), "the size of this type is meaningless");
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    static_assert(!sizeof(value_type), "the size of this type is meaningless");
  }
  else if constexpr (optional_v<value_type>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb_value_size(*t);
  }
  else if constexpr (is_one_of_v<value_type>) {
    return pb_value_size(t.value);
  }
  else {
    return pb_key_value_size<0>(t);
  }
}

}  // namespace detail
}  // namespace iguana