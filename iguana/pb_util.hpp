#pragma once
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace iguana {
namespace detail {
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

[[nodiscard]] inline bool decode_varint(const char* data, std::size_t& pos_,
                                        std::size_t size_, uint64_t& v) {
  // fix test failed on arm due to different char definition
  const signed char* data_ = reinterpret_cast<const signed char*>(data);
  // from https://github.com/facebook/folly/blob/main/folly/Varint.h
  if (pos_ < size_ && (static_cast<uint64_t>(data_[pos_]) & 0x80U) == 0) {
    v = static_cast<uint64_t>(data_[pos_]);
    pos_++;
    return true;
  }
  constexpr const int8_t max_varint_length = sizeof(uint64_t) * 8 / 7 + 1;
  uint64_t val = 0;
  if (size_ - pos_ >= max_varint_length) [[likely]] {
    do {
      // clang-format off
                int64_t b = data_[pos_++];
                val  = ((uint64_t(b) & 0x7fU)       ); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) <<  7U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 14U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 21U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 28U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 35U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 42U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 49U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x7fU) << 56U); if (b >= 0) { break; }
                b = data_[pos_++]; val |= ((uint64_t(b) & 0x01U) << 63U); if (b >= 0) { break; }
      // clang-format on
      return false;
    } while (false);
  }
  else {
    unsigned int shift = 0;
    while (pos_ != size_ && int64_t(data_[pos_]) < 0) {
      val |= (uint64_t(data_[pos_++]) & 0x7fU) << shift;
      shift += 7;
    }
    if (pos_ == size_) {
      return false;
    }
    val |= uint64_t(data_[pos_++]) << shift;
  }
  v = val;
  return true;
}

template <typename Stream>
inline void serialize_varint(uint64_t v, Stream& out) {
  while (v >= 0x80) {
    out.push_back(static_cast<uint8_t>(v | 0x80));
    v >>= 7;
  }
  out.push_back(static_cast<uint8_t>(v));
}
[[nodiscard]] inline bool deserialize_varint(const char* data, std::size_t& pos,
                                             std::size_t size, uint64_t& v) {
  return decode_varint(data, pos, size, v);
}
[[nodiscard]] inline bool read_tag(const char* data, std::size_t& pos,
                                   std::size_t size, uint64_t& tag) {
  return deserialize_varint(data, pos, size, tag);
}
}  // namespace detail
}  // namespace iguana