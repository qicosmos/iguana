#pragma once
#include "pb_util.hpp"

namespace iguana {

template <typename T>
inline void from_pb(T& t, std::string_view pb_str);

namespace detail {

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
    pb_str = pb_str.substr(pos);
    if (pb_str.size() < size) {
      throw std::invalid_argument("Invalid fixed int value: too few bytes.");
    }
    if (size == 0) {
      return;
    }
    from_pb(val, pb_str.substr(0, size));
    pb_str = pb_str.substr(size);
  }
  else if constexpr (is_sequence_container<value_type>::value) {
    using item_type = typename value_type::value_type;
    if constexpr (is_lenprefix_v<item_type>) {
      // item_type non-packed
      while (!pb_str.empty()) {
        item_type item{};  // init the default value
        from_pb_impl<item_type>(item, pb_str);
        val.push_back(std::move(item));
        if (pb_str.empty()) {
          break;
        }
        uint32_t key = detail::decode_varint(pb_str, pos);
        uint32_t field_number = key >> 3;
        if (field_number != field_no) {
          break;
        }
        else {
          pb_str = pb_str.substr(pos);
        }
      }
    }
    else {
      // item_type packed
      size_t pos;
      uint32_t size = detail::decode_varint(pb_str, pos);
      pb_str = pb_str.substr(pos);
      if (pb_str.size() < size) {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
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
  }
  else if constexpr (is_map_container<value_type>::value) {
    using item_type = std::pair<typename value_type::key_type,
                                typename value_type::mapped_type>;
    while (!pb_str.empty()) {
      size_t pos;
      uint32_t size = detail::decode_varint(pb_str, pos);
      pb_str = pb_str.substr(pos);
      if (pb_str.size() < size) {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }

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
    val = static_cast<value_type>(detail::decode_varint(pb_str, pos));
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
  else if constexpr (detail::is_fixed_v<value_type> ||
                     std::is_same_v<value_type, double> ||
                     std::is_same_v<value_type, float>) {
    constexpr size_t size =
        sizeof(typename detail::get_inner_type<value_type>::v_type);
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
  size_t pos = 0;
  while (!pb_str.empty()) {
    uint32_t key = detail::decode_varint(pb_str, pos);
    WireType wire_type = static_cast<WireType>(key & 0b0111);
    uint32_t field_number = key >> 3;

    pb_str = pb_str.substr(pos);

    const static auto& map = get_members<T>();
    uint32_t sub_val = 1;
    if constexpr (!is_reflection_v<T>) {
      sub_val = 0;
    }
    auto& member = map.at(field_number - sub_val);
    std::visit(
        [&t, &pb_str, wire_type](auto& val) {
          using value_type = typename std::decay_t<decltype(val)>::value_type;
          if (wire_type != detail::get_wire_type<value_type>()) {
            throw std::runtime_error("unmatched wire_type");
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
  }
}
}  // namespace iguana