#pragma once
#include "pb2_util.hpp"
#include "pb_reader.hpp"

namespace iguana {
namespace detail {
template <typename T>
IGUANA_INLINE void from_pb2_impl(T& val, std::string_view& pb_str,
                                 uint32_t field_no = 0,
                                 pb_unknown_fields* unknowns = nullptr);

template <typename T>
IGUANA_INLINE void decode2_pair_value(T& val, std::string_view& pb_str,
                                      pb_unknown_fields* unknowns = nullptr) {
  size_t pos;
  uint32_t key = detail::decode_varint(pb_str, pos);
  pb_str = pb_str.substr(pos);
  WireType wire_type = static_cast<WireType>(key & 0b0111);
  if (wire_type != detail::get_wire2_type<std::remove_reference_t<T>>()) {
    return;
  }
  from_pb2_impl(val, pb_str, 0, unknowns);
}

template <typename T>
IGUANA_INLINE void from_pb2_impl(T& val, std::string_view& pb_str,
                                 uint32_t field_no,
                                 pb_unknown_fields* unknowns) {
  size_t pos = 0;
  if constexpr (ylt_refletable_v<T>) {
    size_t pos;
    uint32_t size = detail::decode_varint(pb_str, pos);
    pb_str = pb_str.substr(pos);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    if (size == 0) {
      return;
    }

    if (unknowns) {
      if (field_no != 0) {
        pb_unknown_fields tmp;
        from_pb2(val, pb_str.substr(0, size), &tmp);
        if (!tmp.empty()) {
          unknowns->new_unknown(field_no, std::move(tmp));
        }
      }
      else {
        from_pb2(val, pb_str.substr(0, size), unknowns);
      }
    }
    else {
      from_pb2(val, pb_str.substr(0, size));
    }

    pb_str = pb_str.substr(size);
  }
  else if constexpr (is_sequence_container<T>::value) {
    // item_type non-packed
    using item_type = typename T::value_type;
    while (!pb_str.empty()) {
      item_type item{};
      from_pb2_impl(item, pb_str, 0, unknowns);
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
  else if constexpr (is_map_container<T>::value) {
    using item_type = std::pair<typename T::key_type, typename T::mapped_type>;
    while (!pb_str.empty()) {
      size_t pos;
      uint32_t size = detail::decode_varint(pb_str, pos);
      pb_str = pb_str.substr(pos);
      if (pb_str.size() < size)
        IGUANA_UNLIKELY {
          throw std::invalid_argument(
              "Invalid fixed int value: too few bytes.");
        }
      item_type item = {};
      decode2_pair_value(item.first, pb_str, unknowns);
      decode2_pair_value(item.second, pb_str, unknowns);
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
  else if constexpr (std::is_integral_v<T>) {
    val = static_cast<T>(detail::decode_varint(pb_str, pos));
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_signed_varint_v<T>) {
    constexpr size_t len = sizeof(typename T::value_type);
    uint64_t temp = detail::decode_varint(pb_str, pos);
    if constexpr (len == 8) {
      val.val = detail::decode_zigzag(temp);
    }
    else {
      val.val = detail::decode_zigzag(static_cast<uint32_t>(temp));
    }
    pb_str = pb_str.substr(pos);
  }
  else if constexpr (detail::is_fixed_v<T>) {
    constexpr size_t size = sizeof(typename T::value_type);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    memcpy(&(val.val), pb_str.data(), size);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
    constexpr size_t size = sizeof(T);
    if (pb_str.size() < size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid fixed int value: too few bytes.");
      }
    memcpy(&(val), pb_str.data(), size);
    pb_str = pb_str.substr(size);
  }
  else if constexpr (std::is_same_v<T, std::string> ||
                     std::is_same_v<T, std::string_view>) {
    size_t size = detail::decode_varint(pb_str, pos);
    if (pb_str.size() < pos + size)
      IGUANA_UNLIKELY {
        throw std::invalid_argument("Invalid string value: too few bytes.");
      }
    if constexpr (std::is_same_v<T, std::string_view>) {
      val = std::string_view(pb_str.data() + pos, size);
    }
    else {
      detail::resize(val, size);
      memcpy(val.data(), pb_str.data() + pos, size);
    }
    pb_str = pb_str.substr(size + pos);
  }
  else if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    U value{};
    from_pb2_impl(value, pb_str, 0, unknowns);
    val = static_cast<T>(value);
  }
  else if constexpr (optional_v<T>) {
    from_pb2_impl(val.emplace(), pb_str, 0, unknowns);
  }
  else {
    static_assert(!sizeof(T), "err");
  }
}
}  // namespace detail

template <typename T>
IGUANA_INLINE void from_pb2(T& t, std::string_view pb_str,
                            pb_unknown_fields* unknowns = nullptr) {
  if (pb_str.empty())
    IGUANA_UNLIKELY { return; }
  size_t pos = 0;
  uint32_t key = detail::decode_varint(pb_str, pos);
  WireType wire_type = static_cast<WireType>(key & 0b0111);
  uint32_t field_number = key >> 3;
  while (true) {
    pb_str = pb_str.substr(pos);
    static auto map = detail::get_members(std::forward<T>(t));
    auto it = map.find(field_number);
    if (it != map.end()) {
      auto& member = map.at(field_number);
      std::visit(
          [&t, &pb_str, &unknowns, wire_type, key, field_number](auto& val) {
            using sub_type = typename std::decay_t<decltype(val)>::sub_type;
            using value_type = typename std::decay_t<decltype(val)>::value_type;
            if (wire_type != detail::get_wire2_type<sub_type>()) {
              throw std::runtime_error("unmatched wire_type");
            }

            if (unknowns) {
              pb_unknown_fields tmp;
              detail::from_pb2_impl(val.value(t), pb_str, val.field_no, &tmp);
              if (!tmp.empty()) {
                unknowns->new_unknown(field_number, std::move(tmp));
              }
            }
            else {
              detail::from_pb2_impl(val.value(t), pb_str, val.field_no, nullptr);
            }
          },
          member);
      if (!pb_str.empty())
        IGUANA_LIKELY {
          key = detail::decode_varint(pb_str, pos);
          wire_type = static_cast<WireType>(key & 0b0111);
          field_number = key >> 3;
        }
      else {
        return;
      }
    }
    else {
      // TODO: skip field no and fresh pos
      switch (wire_type) {
        case WireType::Varint: {
          auto v = detail::decode_varint(pb_str, pos);
          if (unknowns) {
            unknowns->push(key, pb_str.substr(0, pos));
          }
        } break;
        case WireType::Fixed64:
          pos = 8;
          if (unknowns) {
            unknowns->push(key, pb_str.substr(0, pos));
          }
          break;
        case WireType::LengthDelimeted: {
          auto sz = detail::decode_varint(pb_str, pos);
          if (pb_str.size() < (pos + sz)) {
            throw std::runtime_error("string len err");
          }
          if (unknowns) {
            auto s = pb_str.substr(0, sz + pos);
            unknowns->push(key, s);
          }
          pos += sz;
        } break;
        case WireType::Fixed32:
          pos = 4;
          if (unknowns) {
            unknowns->push(key, pb_str.substr(0, pos));
          }
          break;
        default:
          throw std::runtime_error("err wire_type");
      }
      pb_str = pb_str.substr(pos);
      if (!pb_str.empty()) {
        IGUANA_LIKELY {
          key = detail::decode_varint(pb_str, pos);
          wire_type = static_cast<WireType>(key & 0b0111);
          field_number = key >> 3;
        }
      }
      else {
        return;
      }
    }
  }
}

}  // namespace iguana