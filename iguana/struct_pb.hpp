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
                std::is_enum_v<T>) {
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

template <typename T>
inline void encode_key(uint32_t field_number, WireType type, T& out) {
  if (field_number == 0) {
    return;
  }

  uint32_t key = (field_number << 3) | static_cast<uint32_t>(type);
  serialize_varint(key, out);
}

template <typename Value, typename T>
inline void encode_varint_field(uint32_t field_number, WireType type,
                                Value value, T& out) {
  static_assert(std::is_integral_v<Value>, "must be integral");
  if (value == 0) {
    return;
  }

  encode_key(field_number, type, out);
  serialize_varint(value, out);
}

template <typename Value, typename T>
inline void encode_signed_varint_field(uint32_t field_number, WireType type,
                                       Value value, T& out) {
  if (value == 0) {
    return;
  }

  encode_key(field_number, type, out);
  serialize_varint(encode_zigzag(value.val), out);
}

template <typename Value, typename T>
inline void encode_fixed_field(uint32_t field_number, WireType type,
                               Value value, T& out) {
  if (value == 0) {
    return;
  }

  encode_key(field_number, type, out);

  constexpr size_t size = sizeof(Value);
  char buf[size]{};
  memcpy(buf, &value, size);
  out.append(buf, size);
}

template <typename T>
inline void encode_string_field(uint32_t field_number, WireType type,
                                std::string_view str, T& out) {
  if (str.empty()) {
    return;
  }

  encode_key(field_number, type, out);

  serialize_varint(str.size(), out);
  out.append(str);
}

template <typename value_type, typename T>
inline void from_pb_impl(T& val, std::string_view& pb_str,
                         uint32_t field_no = 0);

template <typename T>
inline void decode_pair_value(T& val, std::string_view& pb_str) {
  size_t pos;
  uint32_t key = detail::decode_varint(pb_str, pos);
  pb_str = pb_str.substr(pos);
  WireType wire_type = static_cast<WireType>(key & 0b0111);
  if (wire_type != detail::get_wire_type<std::remove_reference_t<T>>()) {
    return;
  }

  from_pb_impl<T>(val, pb_str);
}

template <typename value_type, typename T>
inline void from_pb_impl(T& val, std::string_view& pb_str, uint32_t field_no) {
  size_t pos = 0;
  if constexpr (is_reflection_v<value_type>) {
    size_t pos;
    uint32_t size = detail::decode_varint(pb_str, pos);
    if (pb_str.size() < size) {
      throw std::invalid_argument("Invalid fixed int value: too few bytes.");
    }
    pb_str = pb_str.substr(pos);

    from_pb(val, pb_str);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    size_t pos;
    uint32_t size = detail::decode_varint(pb_str, pos);
    if (pb_str.size() < size) {
      throw std::invalid_argument("Invalid fixed int value: too few bytes.");
    }
    pb_str = pb_str.substr(pos);

    using item_type = typename value_type::value_type;
    size_t start = pb_str.size();

    while (!pb_str.empty()) {
      item_type item;
      from_pb_impl<item_type>(item, pb_str);
      val.push_back(std::move(item));
      if (start - pb_str.size() == size) {
        break;
      }
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    using item_type = std::pair<typename value_type::key_type,
                                typename value_type::mapped_type>;
    while (!pb_str.empty()) {
      size_t pos;
      uint32_t size = detail::decode_varint(pb_str, pos);
      if (pb_str.size() < size) {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
      pb_str = pb_str.substr(pos);

      item_type item = {};
      decode_pair_value(item.first, pb_str);
      decode_pair_value(item.second, pb_str);
      val.emplace(std::move(item));

      if (pb_str.empty()) {
        break;
      }

      uint32_t key = detail::decode_varint(pb_str, pos);
      uint32_t field_number = key >> 3;
      if (field_number != field_no) {
        break;
      }

      pb_str = pb_str.substr(pos);
    }
  }
  else if constexpr (std::is_integral_v<value_type>) {
    val = detail::decode_varint(pb_str, pos);
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_signed_varint_v<value_type>) {
    constexpr size_t len = sizeof(typename value_type::value_type);

    uint64_t temp = detail::decode_varint(pb_str, pos);
    if constexpr (len == 8) {
      val = detail::decode_zigzag(temp);
    }
    else {
      val = detail::decode_zigzag((uint32_t)temp);
    }
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_fixed_v<value_type>) {
    constexpr size_t size = sizeof(typename value_type::value_type);
    if (pb_str.size() < size) {
      throw std::invalid_argument("Invalid fixed int value: too few bytes.");
    }
    memcpy(&(val), pb_str.data(), size);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (std::is_same_v<value_type, std::string> ||
                     std::is_same_v<value_type, std::string_view>) {
    size_t size = detail::decode_varint(pb_str, pos);
    if (pb_str.size() < pos + size) {
      throw std::invalid_argument("Invalid string value: too few bytes.");
    }

    if constexpr (std::is_same_v<value_type, std::string_view>) {
      val = std::string_view(pb_str.data() + pos, size);
    }
    else {
      val.resize(size);
      memcpy(val.data(), pb_str.data() + pos, size);
    }
    pb_str = pb_str.substr(size + pos);
  }
  else if constexpr (std::is_enum_v<value_type>) {
    using U = std::underlying_type_t<value_type>;
    U value;
    from_pb_impl<U>(value, pb_str);
    val = static_cast<value_type>(value);
  }
  else if constexpr (optional_v<value_type>) {
    from_pb_impl<typename value_type::value_type>(val.emplace(), pb_str);
  }
  else if constexpr (is_one_of_v<value_type>) {
    from_pb_impl<typename value_type::value_type>(val.value, pb_str);
  }
  else {
    static_assert(!sizeof(value_type), "err");
  }
}

template <typename value_type, typename T>
inline void to_pb_impl(T& val, size_t field_no, std::string& out) {
  if constexpr (is_reflection_v<value_type>) {
    std::string temp;
    to_pb(val, temp);
    detail::encode_string_field(field_no, WireType::LengthDelimeted, temp, out);
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    std::string temp;
    using item_type = typename value_type::value_type;
    for (auto& item : val) {
      to_pb_impl<item_type>(item, 0, temp);
    }
    detail::encode_string_field(field_no, WireType::LengthDelimeted, temp, out);
  }
  else if constexpr (is_map_container<value_type>::value) {
    using first_type = typename value_type::key_type;
    using second_type = typename value_type::mapped_type;
    for (auto& [k, v] : val) {
      std::string temp;
      to_pb_impl<first_type>(k, 1, temp);
      to_pb_impl<second_type>(v, 2, temp);
      detail::encode_string_field(field_no, WireType::LengthDelimeted, temp,
                                  out);
    }
  }
  else if constexpr (std::is_integral_v<value_type>) {
    detail::encode_varint_field(field_no, WireType::Varint, val, out);
  }
  else if constexpr (detail::is_signed_varint_v<value_type>) {
    detail::encode_signed_varint_field(field_no, WireType::Varint, val, out);
  }
  else if constexpr (detail::is_fixed_v<value_type>) {
    if constexpr (sizeof(value_type) == 8) {
      detail::encode_fixed_field(field_no, WireType::Fixed64, (uint64_t)(val),
                                 out);
    }
    else {
      detail::encode_fixed_field(field_no, WireType::Fixed32, (uint32_t)(val),
                                 out);
    }
  }
  else if constexpr (std::is_same_v<value_type, std::string> ||
                     std::is_same_v<value_type, std::string_view>) {
    detail::encode_string_field(field_no, WireType::LengthDelimeted, val, out);
  }
  else if constexpr (std::is_enum_v<value_type>) {
    using U = std::underlying_type_t<T>;
    detail::encode_varint_field(field_no, WireType::Varint, static_cast<U>(val),
                                out);
  }
  else if constexpr (optional_v<value_type>) {
    if (!val.has_value()) {
      return;
    }

    to_pb_impl<typename value_type::value_type>(*val, field_no, out);
  }
  else if constexpr (is_one_of_v<value_type>) {
    to_pb_impl<typename value_type::value_type>(val.value, field_no, out);
  }
  else {
    static_assert(!sizeof(value_type), "err");
  }
}
}  // namespace detail

template <typename T>
inline constexpr size_t get_member_count_impl() {
  if constexpr (is_reflection<T>::value) {
    return iguana::get_value<std::remove_reference_t<T>>();
  }
  else {
    return iguana_member_count((T*)nullptr);
  }
}

template <typename T>
inline void from_pb(T& t, std::string_view pb_str) {
  using U = std::remove_const_t<std::remove_reference_t<T>>;
  constexpr size_t Count = get_member_count_impl<U>();

  size_t pos = 0;
  while (!pb_str.empty()) {
    uint32_t key = detail::decode_varint(pb_str, pos);
    WireType wire_type = static_cast<WireType>(key & 0b0111);
    uint32_t field_number = key >> 3;

    pb_str = pb_str.substr(pos);

    const auto& arr = get_members(t);
    auto& member = arr.at(field_number - 1);
    std::visit(
        [&t, &pb_str, wire_type](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if (wire_type != detail::get_wire_type<value_type>()) {
            return;
          }

          if constexpr (detail::is_signed_varint_v<value_type> ||
                        detail::is_fixed_v<value_type>) {
            detail::from_pb_impl<value_type>(val.value(t).val, pb_str);
          }
          else {
            detail::from_pb_impl<value_type>(val.value(t), pb_str,
                                             val.field_no);
          }
        },
        member);

    if (field_number == Count) {
      break;
    }
  }
}

template <typename T>
inline void to_pb(T& t, std::string& out) {
  const auto& arr = get_members(t);
  for (auto& member : arr) {
    std::visit(
        [&t, &out](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if constexpr (detail::is_fixed_v<value_type>) {
            detail::to_pb_impl<value_type>(val.value(t).val, val.field_no, out);
          }
          else {
            detail::to_pb_impl<value_type>(val.value(t), val.field_no, out);
          }
        },
        member);
  }
}
}  // namespace iguana