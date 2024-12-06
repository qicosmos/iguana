#pragma once
#include "pb_util.hpp"

namespace iguana {
struct pb_unknown_fields {
  using pb_unknown_pair = std::pair<uint32_t, std::string_view>;
  using var_t =
      std::variant<std::monostate, pb_unknown_pair, pb_unknown_fields>;
  using arr_t = std::map<uint32_t, std::vector<var_t>>;

  pb_unknown_fields() = default;

  void push(uint32_t key, std::string_view data) {
    uint32_t field_number = key >> 3;
    unknowns_[field_number].emplace_back(std::make_pair(key, data));
  }

  template <typename... Args>
  auto new_unknown(uint32_t field_number, Args&&... args) {
    auto it = unknowns_.emplace(
        field_number, std::vector<var_t>{var_t(std::forward<Args>(args)...)});
    if (!it.second) {
      throw std::runtime_error("repeate field_number");
    }
    return &it.first->second;
  }

  size_t get_unknown_size() const {
    size_t sz = 0;
    for (auto& it : unknowns_) {
      for (auto& t : it.second) {
        if (auto p = std::get_if<pb_unknown_pair>(&t); p) {
          sz += detail::variant_uint32_size_constexpr(p->first);
          sz += p->second.size();
        }
        else if (auto p = std::get_if<pb_unknown_fields>(&t); p) {
          sz += p->get_unknown_size();
        }
        else {
          throw std::runtime_error("bad unknown field");
        }
      }
    }
    return sz;
  }

  pb_unknown_fields* child(uint32_t field_number) {
    auto it = unknowns_.find(field_number);
    if (it == unknowns_.end()) {
      return nullptr;
    }
    if (it->second.size() == 1) {
      return std::get_if<pb_unknown_fields>(it->second.data());
    }
    return nullptr;
  }

  template <typename It>
  void write_unknown_field(It& it, size_t last_field_no) {
    for (auto& unknown : unknowns_) {
      if (unknown.first <= last_field_no) {
        continue;
      }
      for (auto& var : unknown.second) {
        if (auto* p = std::get_if<pb_unknown_pair>(&var); p) {
          detail::serialize_varint(p->first, it);  // tag
          memcpy(it, p->second.data(), p->second.size());
          it += p->second.size();
        }
        else if (auto* p = std::get_if<pb_unknown_fields>(&var); p) {
          p->write_unknown_field(it, 0);
        }
        else {
          throw std::runtime_error("unknow error var");
        }
      }
    }
  }

  std::string dump(std::size_t dep = 0) {
    std::stringstream ss;
    auto f_push_str = [&]() -> std::stringstream& {
      for (int i = 0; i < dep; ++i) {
        ss << " ";
      }
      return ss;
    };
    for (auto& it : unknowns_) {
      f_push_str() << "field" << it.first << std::endl;
      for (auto& t : it.second) {
        if (auto p = std::get_if<pb_unknown_pair>(&t); p) {
          f_push_str() << "key:" << p->first << ":(";
          for (auto c : p->second) {
            f_push_str() << static_cast<int>(c) << ",";
          }
          f_push_str() << ")" << std::endl;
        }
        else if (auto p = std::get_if<pb_unknown_fields>(&t); p) {
          f_push_str() << "{" << std::endl;
          f_push_str() << p->dump(dep + 1) << std::endl;
          f_push_str() << "}";
        }
      }
    }
    return ss.str();
  }

  bool empty() { return unknowns_.empty(); }
  arr_t unknowns_;
};

namespace detail {
template <typename T>
constexpr inline WireType get_wire2_type() {
  if constexpr (std::is_integral_v<T> || is_signed_varint_v<T> ||
                std::is_enum_v<T> || std::is_same_v<T, bool>) {
    return WireType::Varint;
  }
  else if constexpr (std::is_same_v<T, fixed32_t> ||
                     std::is_same_v<T, sfixed32_t> ||
                     std::is_same_v<T, float>) {
    return WireType::Fixed32;
  }
  else if constexpr (std::is_same_v<T, fixed64_t> ||
                     std::is_same_v<T, sfixed64_t> ||
                     std::is_same_v<T, double>) {
    return WireType::Fixed64;
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view> ||
                     ylt_refletable_v<T> || is_map_container<T>::value) {
    return WireType::LengthDelimeted;
  }
  else if constexpr (optional_v<T>) {
    return get_wire_type<typename T::value_type>();
  }
  else if constexpr (is_sequence_container<T>::value) {
    return get_wire_type<typename T::value_type>();
  }
  else {
    throw std::runtime_error("unknown type");
  }
}

template <typename T>
constexpr bool is_lenprefix2_v =
    (get_wire2_type<T>() == WireType::LengthDelimeted);

template <size_t key_size, bool omit_default_val, typename Type, typename Arr>
IGUANA_INLINE size_t pb2_key_value_size(Type&& t, Arr& size_arr) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
    size_t len = 0;
    static auto tuple = get_pb_members_tuple(std::forward<Type>(t));
    constexpr size_t SIZE = std::tuple_size_v<std::decay_t<decltype(tuple)>>;
    size_t pre_index = -1;
    if constexpr (!inherits_from_base_v<T> && key_size != 0) {
      pre_index = size_arr.size();
      size_arr.push_back(0);  // placeholder
    }
    for_each_n(
        [&len, &t, &size_arr](auto i) IGUANA__INLINE_LAMBDA {
          using field_type =
              std::tuple_element_t<decltype(i)::value,
                                   std::decay_t<decltype(tuple)>>;
          auto value = std::get<decltype(i)::value>(tuple);
          using U = typename field_type::value_type;
          using sub_type = typename field_type::sub_type;
          auto& val = value.value(t);
          constexpr uint32_t sub_key =
              (value.field_no << 3) |
              static_cast<uint32_t>(get_wire2_type<U>());
          constexpr auto sub_keysize = variant_uint32_size_constexpr(sub_key);
          len +=
              pb2_key_value_size<sub_keysize, omit_default_val>(val, size_arr);
        },
        std::make_index_sequence<SIZE>{});
    if constexpr (inherits_from_base_v<T>) {
      t.cache_size = len;
    }
    else if constexpr (key_size != 0) {
      size_arr[pre_index] = len;
    }
    if constexpr (key_size == 0) {
      // for top level
      return len;
    }
    else {
      if (len == 0) {
        // equals key_size  + variant_uint32_size(len)
        return key_size + 1;
      }
      else {
        return key_size + variant_uint32_size(static_cast<uint32_t>(len)) + len;
      }
    }
  }
  else if constexpr (is_sequence_container<T>::value) {
    size_t len = 0;
    for (auto& item : t) {
      len += pb2_key_value_size<key_size, false>(item, size_arr);
    }
    return len;
  }
  else if constexpr (is_map_container<T>::value) {
    size_t len = 0;
    for (auto& [k, v] : t) {
      // the key_size of  k and v  is constant 1
      auto kv_len = pb2_key_value_size<1, false>(k, size_arr) +
                    pb2_key_value_size<1, false>(v, size_arr);
      len += key_size + variant_uint32_size(static_cast<uint32_t>(kv_len)) +
             kv_len;
    }
    return len;
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb2_key_value_size<key_size, omit_default_val>(*t, size_arr);
  }
  else {
    return str_numeric_size<key_size, omit_default_val>(t);
  }
}

// return the payload size
template <bool skip_next = true, typename Type>
IGUANA_INLINE size_t pb2_value_size(Type&& t, uint32_t*& sz_ptr,
                                    pb_unknown_fields* unknowns) {
  using T = std::remove_const_t<std::remove_reference_t<Type>>;
  size_t unknown_sz = 0;
  if (unknowns) {
    unknown_sz = unknowns->get_unknown_size();
  }
  if constexpr (ylt_refletable_v<T> || is_custom_reflection_v<T>) {
    if constexpr (inherits_from_base_v<T>) {
      return t.cache_size;
    }
    else {
      // *sz_ptr is secure and logically guaranteed
      if constexpr (skip_next) {
        return *(sz_ptr++) + unknown_sz;
      }
      else {
        return *sz_ptr + unknown_sz;
      }
    }
  }
  else if constexpr (is_sequence_container<T>::value) {
    using item_type = typename T::value_type;
    size_t len = 0;
    if constexpr (!is_lenprefix2_v<item_type>) {
      for (auto& item : t) {
        len += str_numeric_size<0, false>(item);
      }
      return len + unknown_sz;
    }
    else {
      static_assert(!sizeof(item_type), "the size of this type is meaningless");
    }
  }
  else if constexpr (is_map_container<T>::value) {
    static_assert(!sizeof(T), "the size of this type is meaningless");
  }
  else if constexpr (optional_v<T>) {
    if (!t.has_value()) {
      return 0;
    }
    return pb2_value_size(*t, sz_ptr, unknows);
  }
  else {
    return str_numeric_size<0, false>(t) + unknown_sz;
  }
}

}  // namespace detail

}  // namespace iguana