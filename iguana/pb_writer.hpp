#pragma once
#include "pb_util.hpp"

namespace iguana {
namespace detail {

template <uint32_t key, typename V, typename Stream>
IGUANA_INLINE void encode_varint_field(V val, Stream& out) {
  static_assert(std::is_integral_v<V>, "must be integral");
  if constexpr (key != 0) {
    serialize_varint_u32_constexpr<key>(out);
  }
  serialize_varint(val, out);
}

template <uint32_t key, typename V, typename Stream>
IGUANA_INLINE void encode_fixed_field(V val, Stream& out) {
  if constexpr (key != 0) {
    serialize_varint_u32_constexpr<key>(out);
  }
  constexpr size_t size = sizeof(V);
  // TODO: check Stream continuous
  auto end = out.size();
  out.resize(out.size() + size);
  memcpy(&out[end], &val, size);
}

template <uint32_t key, bool omit_default_val = true, typename Type,
          typename Stream>
IGUANA_INLINE void to_pb_impl(Type&& t, Stream& out);

template <uint32_t key, typename V, typename Stream>
IGUANA_INLINE void encode_pair_value(V&& val, Stream& out, size_t size) {
  if (size == 0)
    IGUANA_UNLIKELY {
      // map keys can't be omitted even if values are empty
      serialize_varint_u32_constexpr<key>(out);
      serialize_varint(0, out);
    }
  else {
    to_pb_impl<key, false>(val, out);
  }
}

template <bool omit_default_val, uint32_t key, typename T, typename Stream>
IGUANA_INLINE void encode_numeric_field(T t, Stream& out) {
  if constexpr (omit_default_val) {
    if constexpr (is_fixed_v<T> || is_signed_varint_v<T>) {
      if (t.val == 0) {
        return;
      }
    }
    else {
      if (t == static_cast<T>(0))
        IGUANA_UNLIKELY { return; }
    }
  }
  if constexpr (std::is_integral_v<T>) {
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
  else if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    detail::encode_varint_field<key>(static_cast<U>(t), out);
  }
  else {
    static_assert(!sizeof(T), "unsupported type");
  }
}

template <typename Variant, typename T, size_t I>
constexpr size_t get_variant_index() {
  if constexpr (I == 0) {
    static_assert(std::is_same_v<std::variant_alternative_t<0, Variant>, T>,
                  "Type T is not found in Variant");
    return 0;
  }
  else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>,
                                    T>) {
    return I;
  }
  else {
    return get_variant_index<Variant, T, I - 1>();
  }
}

template <uint32_t field_no, typename Type, typename Stream>
IGUANA_INLINE void to_pb_oneof(Type&& t, Stream& out) {
  std::visit(
      [&out](auto&& value) IGUANA__INLINE_LAMBDA {
        using value_type =
            std::remove_const_t<std::remove_reference_t<decltype(value)>>;
        constexpr uint32_t key =
            (field_no << 3) |
            static_cast<uint32_t>(get_wire_type<value_type>());
        to_pb_impl<key, false>(std::forward<value_type>(value), out);
      },
      std::forward<Type>(t));
}

// omit_default_val = true indicates to omit the default value in searlization
template <uint32_t key, bool omit_default_val, typename Type, typename Stream>
IGUANA_INLINE void to_pb_impl(Type&& t, Stream& out) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (is_reflection_v<T> || is_custom_reflection_v<T>) {
    // TODO: improve the key serialize
    auto len = pb_value_size(t);
    // can't be omitted even if values are empty
    if constexpr (key != 0) {
      serialize_varint_u32_constexpr<key>(out);
      serialize_varint(len, out);
      if (len == 0)
        IGUANA_UNLIKELY { return; }
    }
    constexpr auto tuple = get_members_tuple<T>();
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&t, &out, &tuple](auto i) IGUANA__INLINE_LAMBDA {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          constexpr auto value = std::get<decltype(i)::value>(tuple);
          auto& val = value.value(t);

          using U = typename field_type::value_type;
          if constexpr (variant_v<U>) {
            if (!std::holds_alternative<typename field_type::sub_type>(val)) {
              return;
            }

            to_pb_oneof<value.field_no>(val, out);
          }
          else {
            constexpr uint32_t sub_key =
                (value.field_no << 3) |
                static_cast<uint32_t>(get_wire_type<U>());
            to_pb_impl<sub_key>(val, out);
          }
        },
        std::make_index_sequence<SIZE>{});
  }
  else if constexpr (is_sequence_container<T>::value) {
    // TODO support std::array
    // repeated values can't be omitted even if values are empty
    using item_type = typename T::value_type;
    if constexpr (is_lenprefix_v<item_type>) {
      // non-packed
      for (auto& item : t) {
        to_pb_impl<key, false>(item, out);
      }
    }
    else {
      if (t.empty())
        IGUANA_UNLIKELY { return; }
      serialize_varint_u32_constexpr<key>(out);
      serialize_varint(pb_value_size(t), out);
      for (auto& item : t) {
        encode_numeric_field<false, 0>(item, out);
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
      serialize_varint_u32_constexpr<key>(out);
      auto k_len = pb_value_size(k);
      auto v_len = pb_value_size(v);
      auto pair_len = key1_size + key2_size + k_len + v_len;
      if constexpr (is_lenprefix_v<first_type>) {
        pair_len += variant_uint32_size(k_len);
      }
      if constexpr (is_lenprefix_v<second_type>) {
        pair_len += variant_uint32_size(v_len);
      }
      serialize_varint(pair_len, out);
      // map k and v can't be omitted even if values are empty
      encode_pair_value<key1>(k, out, k_len);
      encode_pair_value<key2>(v, out, v_len);
    }
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return;
    }
    to_pb_impl<key, omit_default_val>(*t, out);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if constexpr (omit_default_val) {
      if (t.size() == 0)
        IGUANA_UNLIKELY { return; }
    }
    serialize_varint_u32_constexpr<key>(out);
    serialize_varint(t.size(), out);
    out.append(t);
  }
  else {
    encode_numeric_field<omit_default_val, key>(t, out);
  }
}
}  // namespace detail

template <typename T, typename Stream>
IGUANA_INLINE void to_pb(T& t, Stream& out) {
  auto byte_len = detail::pb_key_value_size<0>(t);
  out.reserve(out.size() + byte_len);
  detail::to_pb_impl<0>(t, out);
}
}  // namespace iguana
