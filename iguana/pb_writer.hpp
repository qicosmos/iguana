#pragma once
#include "pb_util.hpp"

namespace iguana {
namespace detail {

template <uint32_t key, typename V, typename Stream>
inline void encode_varint_field(V val, Stream& out) {
  static_assert(std::is_integral_v<V>, "must be integral");
  if (val == 0) {
    return;
  }
  if constexpr (key != 0) {
    serialize_varint(key, out);
  }
  serialize_varint(val, out);
}

template <uint32_t key, typename V, typename Stream>
inline void encode_fixed_field(V val, Stream& out) {
  if (val == 0) {
    return;
  }
  if constexpr (key != 0) {
    serialize_varint(key, out);
  }
  constexpr size_t size = sizeof(V);
  // TODO: check Stream continuous
  auto end = out.size();
  out.resize(out.size() + size);
  memcpy(&out[end], &val, size);
}

template <uint32_t key, typename Type, typename Stream>
inline void to_pb_impl(Type& t, Stream& out);

// TOOD: pb_item_size improve
template <uint32_t key, typename V, typename Stream>
inline void encode_pair_value(V&& val, Stream& out, size_t size) {
  if (size == 0) {
    serialize_varint(key, out);
    serialize_varint(0, out);
  }
  else {
    to_pb_impl<key>(val, out);
  }
}

// key==0 occurs exclusively in packed value and top level reflection
template <uint32_t key, typename Type, typename Stream>
inline void to_pb_impl(Type& t, Stream& out) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (is_reflection_v<T> || is_custom_reflection_v<T>) {
    // TODO: improve the key serialize
    auto len = pb_load_size(t);
    if (len == 0) {
      return;
    }
    if constexpr (key != 0) {
      serialize_varint(key, out);
      serialize_varint(len, out);
    }
    // TODO: reduce get_members_impl call times
    constexpr auto tuple = get_members_impl<T>();
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&t, &out](auto i) {
          constexpr static auto tp = get_members_impl<T>();
          constexpr auto value = std::get<decltype(i)::value>(tp);
          using U = typename std::decay_t<decltype(value)>::value_type;
          constexpr uint32_t sub_key =
              (value.field_no << 3) | static_cast<uint32_t>(get_wire_type<U>());
          to_pb_impl<sub_key>(value.value(t), out);
        },
        std::make_index_sequence<SIZE>{});
  }
  else if constexpr (is_sequence_container<T>::value) {
    // TODO support std::array
    using item_type = typename T::value_type;
    if constexpr (is_lenprefix_v<item_type>) {
      // non-packed
      for (auto& item : t) {
        to_pb_impl<key>(item, out);
      }
    }
    else {
      serialize_varint(key, out);
      serialize_varint(pb_load_size(t), out);
      for (auto& item : t) {
        to_pb_impl<0>(item, out);
      }
    }
  }
  else if constexpr (is_map_container<T>::value) {
    using first_type = typename T::key_type;
    using second_type = typename T::mapped_type;
    constexpr uint32_t key1 =
        (1 << 3) | static_cast<uint32_t>(get_wire_type<first_type>());
    constexpr auto key1_size = variant_uint32_size_constexpr(key1);
    constexpr uint32_t key2 =
        (2 << 3) | static_cast<uint32_t>(get_wire_type<second_type>());
    constexpr auto key2_size = variant_uint32_size_constexpr(key2);

    for (auto& [k, v] : t) {
      serialize_varint(key, out);
      auto k_len = pb_load_size(k);
      auto v_len = pb_load_size(v);
      auto pair_len = key1_size + key2_size + k_len + v_len;
      if constexpr (is_lenprefix_v<first_type>) {
        pair_len += variant_uint32_size(k_len);
      }
      if constexpr (is_lenprefix_v<second_type>) {
        pair_len += variant_uint32_size(v_len);
      }
      serialize_varint(pair_len, out);
      encode_pair_value<key1>(k, out, k_len);
      encode_pair_value<key2>(v, out, v_len);
    }
  }
  else if constexpr (std::is_integral_v<T>) {
    detail::encode_varint_field<key>(t, out);
  }
  else if constexpr (detail::is_signed_varint_v<T>) {
    detail::encode_varint_field<key>(encode_zigzag(t.val), out);
  }
  else if constexpr (detail::is_fixed_v<T>) {
    detail::encode_fixed_field<key>(t.val, out);
  }
  else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    detail::encode_fixed_field<key>(t, out);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    serialize_varint(key, out);
    serialize_varint(t.size(), out);
    out.append(t);
  }
  else if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    detail::encode_varint_field<key>(static_cast<U>(t), out);
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return;
    }
    to_pb_impl<key>(*t, out);
  }
  else if constexpr (is_one_of_v<T>) {
    to_pb_impl<key>(t.value, out);
  }
  else {
    static_assert(!sizeof(T), "err");
  }
}
}  // namespace detail

template <typename T, typename Stream>
inline void to_pb(T& t, Stream& out) {
  auto byte_len = detail::pb_item_size(t);
  out.reserve(out.size() + byte_len);
  detail::to_pb_impl<0>(t, out);
}
}  // namespace iguana