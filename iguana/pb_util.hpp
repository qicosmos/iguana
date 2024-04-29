#pragma once
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>

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
struct sint32_t {
  using value_type = int32_t;
  int32_t val;
};

struct sint64_t {
  using value_type = int64_t;
  int64_t val;
};

inline bool operator==(sint32_t value1, int32_t value2) {
  return value1.val == value2;
}

inline bool operator==(sint64_t value1, int64_t value2) {
  return value1.val == value2;
}

struct fixed32_t {
  using value_type = uint32_t;
  uint32_t val;
};

struct fixed64_t {
  using value_type = uint64_t;
  uint64_t val;
};

inline bool operator==(fixed32_t value1, uint32_t value2) {
  return value1.val == value2;
}

inline bool operator==(fixed64_t value1, uint64_t value2) {
  return value1.val == value2;
}

struct sfixed32_t {
  using value_type = int32_t;
  int32_t val;
};

struct sfixed64_t {
  using value_type = int64_t;
  int64_t val;
};

inline bool operator==(sfixed32_t value1, int32_t value2) {
  return value1.val == value2;
}

inline bool operator==(sfixed32_t value1, int64_t value2) {
  return value1.val == value2;
}

template <size_t Idx, typename V>
struct one_of_t {
  static constexpr bool is_one_of_v = true;
  using value_type = std::optional<
      std::remove_reference_t<decltype(std::get<Idx>(std::declval<V>()))>>;
  value_type value;
};

template <typename T>
inline void to_pb(T& t, std::string& out);

template <typename T>
inline void from_pb(T& t, std::string_view pb_str);

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
    return WireType::Unknown;
  }
}

template <typename T, typename = void>
struct get_inner_type {
  using v_type = T;
};

template <typename T>
struct get_inner_type<T, std::void_t<typename T::value_type>> {
  using v_type = typename T::value_type;
};
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

// TODO: support user-defined struct
template <typename T>
inline size_t pb_item_size(T&& t, size_t key_size = 0) {
  using value_type = std::remove_reference_t<T>;
  if constexpr (is_reflection_v<value_type>) {
    size_t len = 0;
    const static auto& map = get_members(t);
    for (auto& [field_no, member] : map) {
      std::visit(
          [&t, &len](auto& val) {
            using U = typename std::decay_t<decltype(val)>::value_type;
            // TODO: get the key during compile time
            uint32_t key =
                (val.field_no << 3) | static_cast<uint32_t>(get_wire_type<U>());
            len += pb_item_size(val.value(t), variant_uint32_size(key));
          },
          member);
    }
    // TODO: compile time job
    if (key_size == 0) {
      return len;
    }
    return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    size_t len = 0;
    if constexpr (is_reflection_v<item_type>) {
      for (auto& item : t) {
        len += pb_item_size(item, key_size);
      }
      return len;
    }
    else {
      for (auto& item : t) {
        len += pb_item_size(item, 0);
      }
      return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    size_t len = 0;
    for (auto& [k, v] : t) {
      // key_size == 1;
      size_t kv_len = pb_item_size(k, 1) + pb_item_size(v, 1);
      len += key_size + variant_uint32_size(static_cast<uint32_t>(kv_len)) +
             kv_len;
    }
    return len;
  }
  else if constexpr (std::is_same_v<value_type, std::string> ||
                     std::is_same_v<value_type, std::string_view>) {
    return key_size + variant_uint32_size(static_cast<uint32_t>(t.size())) +
           t.size();
  }
  else if constexpr (std::is_integral_v<value_type>) {
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
  else if constexpr (detail::is_fixed_v<value_type> ||
                     std::is_same_v<value_type, double> ||
                     std::is_same_v<value_type, float>) {
    return key_size + sizeof(value_type);
  }
  else if constexpr (std::is_enum_v<value_type>) {
    using U = std::underlying_type_t<value_type>;
    return key_size + variant_intergal_size(static_cast<U>(t));
  }
  else if constexpr (optional_v<value_type>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb_item_size(*t, key_size);
  }
  else if constexpr (is_one_of_v<value_type>) {
    return pb_item_size(t.value, key_size);
  }
  else {
    static_assert(!sizeof(value_type), "err");
  }
}

}  // namespace detail
}  // namespace iguana