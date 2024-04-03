#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "reflection.hpp"
namespace iguana {
namespace detail {
enum class WireType : uint32_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimeted = 2,
  StartGroup = 3,
  EndGroup = 4,
  Fixed32 = 5,
};

template <class To, class From>
inline To bit_cast(From from) {
  static_assert(sizeof(To) == sizeof(From), "");
  static_assert(std::is_trivially_copyable_v<To>, "");
  static_assert(std::is_trivially_copyable_v<From>, "");

  To to;
  std::memcpy(&to, &from, sizeof(from));
  return to;
}

inline void write_varint(uint32_t value, auto& out) {
  uint8_t b[5]{};
  for (size_t i = 0; i < 5; ++i) {
    b[i] = value & 0b0111'1111;
    value >>= 7;
    if (value) {
      b[i] |= 0b1000'0000;
    }
    else {
      out.append((const char*)b, i + 1);
      // out.write(b, i + 1);
      break;
    }
  }
}

inline void write_varint(int32_t value, auto& out) {
  write_varint(bit_cast<uint32_t>(value), out);
}

inline uint32_t make_tag_wire_type(uint32_t tag, WireType wire_type) {
  return (tag << 3) | static_cast<uint32_t>(wire_type);
}

inline void write_tag_wire_type(uint32_t tag, WireType wire_type, auto& out) {
  write_varint(make_tag_wire_type(tag, wire_type), out);
}

struct varint_serializer {
  void serialize(uint32_t tag, int32_t value, auto& out) {
    if (value == 0) {
      return;
    }

    write_tag_wire_type(tag, WireType::Varint, out);
    write_varint(value, out);
  }
};
}  // namespace detail

template <typename T>
inline void from_pb(T& value, std::string_view pb_str) {}

template <typename T>
inline void to_pb(T& t, std::string& out) {
  const auto& arr = get_members(t);
  for (auto& member : arr) {
    std::visit(
        [&t, &out](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if constexpr (std::is_integral_v<value_type>) {
            detail::varint_serializer ser;
            ser.serialize(val.tag, val.value(t), out);
          }
          else {
            static_assert(sizeof(value_type), "err");
          }
        },
        member);
  }
}
}  // namespace iguana