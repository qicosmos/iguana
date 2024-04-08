#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "pb_util.hpp"
#include "reflection.hpp"

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

namespace detail {
template <typename T>
constexpr bool is_fixed_v =
    std::is_same_v<T, fixed32_t> || std::is_same_v<T, fixed64_t> ||
    std::is_same_v<T, sfixed32_t> || std::is_same_v<T, sfixed64_t>;

template <typename T>
constexpr bool is_signed_varint_v =
    std::is_same_v<T, sint32_t> || std::is_same_v<T, sint64_t>;

template <typename T>
constexpr inline WireType get_wire_type() {
  if constexpr (std::is_integral_v<T>) {
    return WireType::Varint;
  }
  else if constexpr (std::is_same_v<T, fixed32_t> ||
                     std::is_same_v<T, sfixed32_t>) {
    return WireType::Fixed32;
  }
  else if constexpr (std::is_same_v<T, fixed64_t> ||
                     std::is_same_v<T, sfixed64_t>) {
    return WireType::Fixed64;
  }
  else {
    return WireType::Unknown;
  }
}

template <typename Value, typename T>
inline void encode_varint_field(uint32_t field_number, WireType type,
                                Value value, T& out) {
  static_assert(std::is_integral_v<Value>, "must be integral");
  if (value == 0) {
    return;
  }

  uint32_t key = (field_number << 3) | static_cast<uint32_t>(type);
  serialize_varint(key, out);
  serialize_varint(value, out);
}

template <typename Value, typename T>
inline void encode_signed_varint_field(uint32_t field_number, WireType type,
                                       Value value, T& out) {
  if (value == 0) {
    return;
  }

  uint32_t key = (field_number << 3) | static_cast<uint32_t>(type);
  serialize_varint(key, out);
  serialize_varint(encode_zigzag(value.val), out);
}

template <typename Value, typename T>
inline void encode_fixed_field(uint32_t field_number, WireType type,
                               Value value, T& out) {
  if (value == 0) {
    return;
  }

  uint32_t key = (field_number << 3) | static_cast<uint32_t>(type);
  serialize_varint(key, out);

  constexpr size_t size = sizeof(Value);
  char buf[size]{};
  memcpy(buf, &value, size);
  out.append(buf, size);
}

}  // namespace detail

template <typename T>
inline void from_pb(T& t, std::string_view pb_str) {
  size_t pos = 0;
  while (!pb_str.empty()) {
    uint32_t key = detail::decode_varint(pb_str, pos);
    WireType wire_type;
    wire_type = static_cast<WireType>(key & 0b0111);
    uint32_t field_number = key >> 3;

    pb_str = pb_str.substr(pos);

    const auto& arr = get_members(t);
    auto& member = arr.at(field_number - 1);
    std::visit(
        [&t, &pb_str, &pos, wire_type](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if constexpr (std::is_integral_v<value_type>) {
            if (wire_type != detail::get_wire_type<value_type>()) {
              return;
            }
            val.value(t) = detail::decode_varint(pb_str, pos);
            pb_str = pb_str.substr(pos);
          }
          else if constexpr (detail::is_signed_varint_v<value_type>) {
            if (wire_type !=
                detail::get_wire_type<typename value_type::value_type>()) {
              return;
            }

            constexpr size_t len = sizeof(typename value_type::value_type);

            uint64_t temp = detail::decode_varint(pb_str, pos);
            if constexpr (len == 8) {
              val.value(t).val = detail::decode_zigzag(temp);
            }
            else {
              val.value(t).val = detail::decode_zigzag((uint32_t)temp);
            }
            pb_str = pb_str.substr(pos);
          }
          else if constexpr (detail::is_fixed_v<value_type>) {
            if (wire_type != detail::get_wire_type<value_type>()) {
              return;
            }

            constexpr size_t size = sizeof(typename value_type::value_type);
            if (pb_str.size() < size) {
              throw std::invalid_argument(
                  "Invalid fixed int value: too few bytes.");
            }
            memcpy(&(val.value(t).val), pb_str.data(), size);
            pb_str = pb_str.substr(size);
          }
          else {
            static_assert(!sizeof(value_type), "err");
          }
        },
        member);
  }
}

template <typename T>
inline void to_pb(T& t, std::string& out) {
  const auto& arr = get_members(t);
  for (auto& member : arr) {
    std::visit(
        [&t, &out](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if constexpr (std::is_integral_v<value_type>) {
            detail::encode_varint_field(val.field_no, WireType::Varint,
                                        val.value(t), out);
          }
          else if constexpr (detail::is_signed_varint_v<value_type>) {
            detail::encode_signed_varint_field(val.field_no, WireType::Varint,
                                               val.value(t), out);
          }
          else if constexpr (detail::is_fixed_v<value_type>) {
            if constexpr (sizeof(value_type) == 8) {
              detail::encode_fixed_field(val.field_no, WireType::Fixed64,
                                         (uint64_t)(val.value(t).val), out);
            }
            else {
              detail::encode_fixed_field(val.field_no, WireType::Fixed32,
                                         (uint32_t)(val.value(t).val), out);
            }
          }
          else {
            static_assert(!sizeof(value_type), "err");
          }
        },
        member);
  }
}
}  // namespace iguana