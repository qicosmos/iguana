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
};

inline uint32_t make_tag_wire_type(uint32_t tag, WireType wire_type) {
  return (tag << 3) | static_cast<uint32_t>(wire_type);
}

struct varint_serializer {
  template <typename T>
  static void serialize(uint32_t field_number, int32_t value, T& out) {
    if (value == 0) {
      return;
    }

    uint32_t key =
        (field_number << 3) | static_cast<uint32_t>(WireType::Varint);
    serialize_varint(key, out);
    serialize_varint(value, out);
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
            detail::varint_serializer::serialize(val.tag, val.value(t), out);
          }
          else {
            static_assert(sizeof(value_type), "err");
          }
        },
        member);
  }
}
}  // namespace iguana