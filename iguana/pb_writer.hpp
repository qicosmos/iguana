#pragma once
#include "pb_util.hpp"

namespace iguana {
namespace detail {
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
inline void to_pb_impl(T& val, size_t field_no, std::string& out);

template <typename value_type, typename T>
inline std::string encode_pair_value(T& val, uint32_t field_no) {
  std::string temp;
  to_pb_impl<value_type>(val, field_no, temp);
  if (temp.empty()) {
    encode_key(field_no, get_wire_type<value_type>(), temp);
    serialize_varint(0, temp);
  }
  return temp;
}

template <typename value_type, typename T>
inline void to_pb_impl(T& val, size_t field_no, std::string& out) {
  if constexpr (is_reflection_v<value_type>) {
    std::string temp;
    to_pb(val, temp);
    detail::encode_string_field(field_no, WireType::LengthDelimeted, temp, out);
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    if constexpr (is_reflection_v<item_type>) {
      for (auto& item : val) {
        to_pb_impl<item_type>(item, field_no, out);
      }
    }
    else {
      std::string temp;
      for (auto& item : val) {
        to_pb_impl<item_type>(item, 0, temp);
      }
      detail::encode_string_field(field_no, WireType::LengthDelimeted, temp,
                                  out);
    }
  }
  else if constexpr (is_map_container<value_type>::value) {
    using first_type = typename value_type::key_type;
    using second_type = typename value_type::mapped_type;
    for (auto& [k, v] : val) {
      std::string temp = encode_pair_value<first_type>(k, 1);
      std::string second_temp = encode_pair_value<second_type>(v, 2);
      encode_key(field_no, WireType::LengthDelimeted, out);
      serialize_varint(temp.size() + second_temp.size(), out);
      out.append(temp).append(second_temp);
    }
  }
  else if constexpr (std::is_integral_v<value_type>) {
    detail::encode_varint_field(field_no, WireType::Varint, val, out);
  }
  else if constexpr (detail::is_signed_varint_v<value_type>) {
    detail::encode_signed_varint_field(field_no, WireType::Varint, val, out);
  }
  else if constexpr (detail::is_fixed_v<value_type> ||
                     std::is_same_v<value_type, double> ||
                     std::is_same_v<value_type, float>) {
    if constexpr (sizeof(value_type) == 8) {
      detail::encode_fixed_field(field_no, WireType::Fixed64, val, out);
    }
    else {
      detail::encode_fixed_field(field_no, WireType::Fixed32, val, out);
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
inline void to_pb(T& t, std::string& out) {
  const static auto& map = get_members<T>();
  for (auto& [field_no, member] : map) {
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