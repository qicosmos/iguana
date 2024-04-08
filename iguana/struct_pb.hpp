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
namespace detail {
enum class WireType : uint32_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimeted = 2,
  StartGroup = 3,
  EndGroup = 4,
  Fixed32 = 5,
  Unknown
};

template <typename T>
constexpr inline WireType get_wire_type() {
  if constexpr (std::is_integral_v<T>) {
    return WireType::Varint;
  }
  else {
    return WireType::Unknown;
  }
}

inline uint32_t make_tag_wire_type(uint32_t tag, WireType wire_type) {
  return (tag << 3) | static_cast<uint32_t>(wire_type);
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

}  // namespace detail

template <typename T>
inline void from_pb(T& t, std::string_view pb_str) {
  size_t pos = 0;
  while (!pb_str.empty()) {
    uint32_t key = detail::decode_varint(pb_str, pos);
    detail::WireType wire_type;
    wire_type = static_cast<detail::WireType>(key & 0b0111);
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
            detail::encode_varint_field(val.tag, detail::WireType::Varint,
                                        val.value(t), out);
          }
          else {
            static_assert(!sizeof(value_type), "err");
          }
        },
        member);
  }
}
}  // namespace iguana