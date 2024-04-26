#pragma once
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace iguana {
namespace detail {
template <typename T, typename = void>
struct get_inner_type {
  using v_type = T;
};

template <typename T>
struct get_inner_type<T, std::void_t<typename T::value_type>> {
  using v_type = typename T::value_type;
};
[[nodiscard]] inline uint32_t encode_zigzag(int32_t v) {
  return (static_cast<uint32_t>(v) << 1U) ^
         static_cast<uint32_t>(
             -static_cast<int32_t>(static_cast<uint32_t>(v) >> 31U));
}
[[nodiscard]] inline uint64_t encode_zigzag(int64_t v) {
  return (static_cast<uint64_t>(v) << 1U) ^
         static_cast<uint64_t>(
             -static_cast<int64_t>(static_cast<uint64_t>(v) >> 63U));
}

[[nodiscard]] inline int64_t decode_zigzag(uint64_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}
[[nodiscard]] inline int64_t decode_zigzag(uint32_t u) {
  return static_cast<int64_t>((u >> 1U)) ^
         static_cast<uint64_t>(-static_cast<int64_t>(u & 1U));
}

template <class T>
inline uint64_t decode_varint(T& data, size_t& pos) {
  const int8_t* begin = reinterpret_cast<const int8_t*>(data.data());
  const int8_t* end = begin + data.size();
  const int8_t* p = begin;
  uint64_t val = 0;

  // end is always greater than or equal to begin, so this subtraction is safe
  if (size_t(end - begin) >= 10) {  // fast path
    int64_t b;
    do {
      b = *p++;
      val = (b & 0x7f);
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 7;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 14;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 21;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 28;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 35;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 42;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 49;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x7f) << 56;
      if (b >= 0) {
        break;
      }
      b = *p++;
      val |= (b & 0x01) << 63;
      if (b >= 0) {
        break;
      }
      throw std::invalid_argument("Invalid varint value: too many bytes.");
    } while (false);
  }
  else {
    int shift = 0;
    while (p != end && *p < 0) {
      val |= static_cast<uint64_t>(*p++ & 0x7f) << shift;
      shift += 7;
    }
    if (p == end) {
      throw std::invalid_argument("Invalid varint value: too few bytes.");
    }
    val |= static_cast<uint64_t>(*p++) << shift;
  }

  pos = (p - begin);
  return val;
}

template <typename Stream>
inline void serialize_varint(uint64_t v, Stream& out) {
  while (v >= 0x80) {
    out.push_back(static_cast<uint8_t>(v | 0x80));
    v >>= 7;
  }
  out.push_back(static_cast<uint8_t>(v));
}

}  // namespace detail
}  // namespace iguana