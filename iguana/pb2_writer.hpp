#pragma once
#include "pb2_util.hpp"
#include "pb2_writer.hpp"

namespace iguana {
namespace detail {
// omit_default_val = true indicates to omit the default value in searlization
template <uint32_t key, bool omit_default_val, typename Type, typename It>
IGUANA_INLINE void to_pb2_impl(Type&& t, It&& it, uint32_t*& sz_ptr,
                               pb_unknown_fields* unknowns) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
    // can't be omitted even if values are empty
    if constexpr (key != 0) {
        auto len = pb2_value_size(t, sz_ptr, unknowns);
        serialize_varint_u32_constexpr<key>(it);
        serialize_varint(len, it);
        if (len == 0)
        IGUANA_UNLIKELY { return; }
    }
    static auto tuple = get_pb_members_tuple(std::forward<Type>(t));
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    for_each_n(
        [&t, &it, &sz_ptr, &unknowns](auto i) IGUANA__INLINE_LAMBDA {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          auto value = std::get<decltype(i)::value>(tuple);
          auto& val = value.value(t);

          using U = typename field_type::value_type;
          //using sub_type = typename field_type::sub_type;
          constexpr uint32_t sub_key =
              (value.field_no << 3) |
              static_cast<uint32_t>(get_wire2_type<U>());
          if (unknowns) {
            to_pb2_impl<sub_key, omit_default_val>(
                val, it, sz_ptr, unknowns->child(value.field_no));
          }
          else {
            to_pb2_impl<sub_key, omit_default_val>(val, it, sz_ptr, nullptr);
          }
        },
        std::make_index_sequence<SIZE>{});
    if (unknowns) {
      size_t max_field_no = std::get<SIZE - 1>(tuple).field_no;
      unknowns->write_unknown_field(it, max_field_no);
    }
  }
  else if constexpr (is_sequence_container<T>::value) {
    // pb2 default non-packed
    for (auto& item : t) {
      to_pb2_impl<key, false>(item, it, sz_ptr, unknowns);
    }
  }
  else if constexpr (is_map_container<T>::value) {
    using first_type = typename T::key_type;
    using second_type = typename T::mapped_type;
    constexpr uint32_t key1 =
        (1 << 3) | static_cast<uint32_t>(get_wire2_type<first_type>());
    constexpr auto key1_size = variant_uint32_size_constexpr(key1);
    constexpr uint32_t key2 =
        (2 << 3) | static_cast<uint32_t>(get_wire2_type<second_type>());
    constexpr auto key2_size = variant_uint32_size_constexpr(key2);

    for (auto& [k, v] : t) {
      serialize_varint_u32_constexpr<key>(it);
      // k must be string or numeric
      auto k_val_len = str_numeric_size<0, false>(k);
      auto v_val_len = pb2_value_size<false>(v, sz_ptr, unknowns);
      auto pair_len = key1_size + key2_size + k_val_len + v_val_len;
      if constexpr (is_lenprefix2_v<first_type>) {
        pair_len += variant_uint32_size(k_val_len);
      }
      if constexpr (is_lenprefix2_v<second_type>) {
        pair_len += variant_uint32_size(v_val_len);
      }
      serialize_varint(pair_len, it);
      // map k and v can't be omitted even if values are empty
      encode_pair_value<key1>(k, it, k_val_len, sz_ptr);
      encode_pair_value<key2>(v, it, v_val_len, sz_ptr);
    }
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return;
    }
    to_pb2_impl<key, omit_default_val>(*t, it, sz_ptr, unknowns);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    if constexpr (omit_default_val) {
      if (t.size() == 0)
        IGUANA_UNLIKELY { return; }
    }
    serialize_varint_u32_constexpr<key>(it);
    serialize_varint(t.size(), it);
    memcpy(it, t.data(), t.size());
    it += t.size();
  }
  else {
    encode_numeric_field<key, omit_default_val>(t, it);
  }
}
}  // namespace detail

template <
    typename T, typename Stream,
    std::enable_if_t<ylt_refletable_v<T> || detail::is_custom_reflection_v<T>,
                     int> = 0>
IGUANA_INLINE void to_pb2(T const& t, Stream& out, pb_unknown_fields* unknowns = nullptr) {
  std::vector<uint32_t> size_arr;
  auto byte_len = detail::pb2_key_value_size<0, false>(t, size_arr);
  if (unknowns) {
    byte_len += unknowns->get_unknown_size();
  }
  detail::resize(out, byte_len);
  auto sz_ptr = size_arr.empty() ? nullptr : &size_arr[0];
  detail::to_pb2_impl<0, false>(t, &out[0], sz_ptr, unknowns);
}

}  // namespace iguana