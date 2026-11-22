#pragma once

#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#include "../util.hpp"
#include "string_resize.hpp"

namespace iguana::detail {

inline void resize_json_string(std::string &output, size_t size) {
#if __cpp_lib_string_resize_and_overwrite >= 202110L || \
    (defined(_MSC_VER) && _MSC_VER <= 1920)
  // resize_and_overwrite cannot publish unwritten bytes as the new length.
  output.resize(size);
#else
  resize(output, size);
#endif
}

// Keep the existing prefix and append directly into reusable string storage.
class json_string_sink {
 public:
  explicit json_string_sink(std::string &output) : output_(output) {
    const auto used = output_.size();
    if (output_.capacity() < 512) {
      output_.reserve(512);
    }
    expose(used);
  }

  json_string_sink(const json_string_sink &) = delete;
  json_string_sink &operator=(const json_string_sink &) = delete;
  ~json_string_sink() { resize_json_string(output_, size()); }

  void push_back(char value) { *prepare(1) = value; }

  void append(const char *data, size_t size) {
    if (size != 0) {
      std::memcpy(prepare(size), data, size);
    }
  }

  void append(std::string_view bytes) { append(bytes.data(), bytes.size()); }

  template <bool Escape, size_t N>
  void write_quoted(const std::array<char, N> &prefix, std::string_view value) {
    if constexpr (Escape) {
      if (value.size() >= 16 || needs_escape(value)) {
        append(prefix.data(), N);
        write_escaped(value);
        return;
      }
    }
    auto *destination = prepare(N + value.size() + 2);
    if constexpr (N != 0) {
      std::memcpy(destination, prefix.data(), N);
    }
    destination[N] = '"';
    if (!value.empty()) {
      std::memcpy(destination + N + 1, value.data(), value.size());
    }
    destination[N + 1 + value.size()] = '"';
  }

  template <size_t N, typename Integer>
  void write_integer(const std::array<char, N> &prefix, Integer value) {
    if (static_cast<uint64_t>(value) < 1000) {
      write_small_integer(prefix, static_cast<unsigned>(value));
      return;
    }
    append(prefix.data(), N);
    write_wide_integer(value);
  }

 private:
  size_t size() const { return static_cast<size_t>(next_ - output_.data()); }

  char *prepare(size_t count) {
    if (count > static_cast<size_t>(end_ - next_)) [[unlikely]] {
      grow(count);
    }
    auto *destination = next_;
    next_ += count;
    return destination;
  }

  void expose(size_t used) {
    const auto capacity = output_.capacity();
    resize_json_string(output_, capacity);
    next_ = output_.data() + used;
    end_ = output_.data() + capacity;
  }

#if defined(__GNUC__)
  [[gnu::cold, gnu::noinline]]
#endif
  void grow(size_t count) {
    const auto used = size();
    if (count > output_.max_size() - used) {
      throw std::length_error("JSON output too large");
    }
    resize_json_string(output_, used);
    next_ = output_.data() + used;
    output_.reserve(used + count);
    expose(used);
  }

  static bool needs_escape(uint64_t word) {
    constexpr uint64_t high_bits = UINT64_C(0x8080808080808080);
    constexpr uint64_t control_bias = UINT64_C(0x2020202020202020);
    constexpr uint64_t slashes = UINT64_C(0x5c5c5c5c5c5c5c5c);
    return (((word | (word - control_bias)) & high_bits) != 0) ||
           has_qoute(word) || has_zero(word ^ slashes);
  }

  static bool needs_escape(std::string_view value) {
    // Two in-bounds loads cover short strings between 8 and 15 bytes.
    if (value.size() >= sizeof(uint64_t)) {
      uint64_t first, last;
      std::memcpy(&first, value.data(), sizeof(first));
      std::memcpy(&last, value.data() + value.size() - sizeof(last),
                  sizeof(last));
      return needs_escape(first) || needs_escape(last);
    }
    if (value.size() >= sizeof(uint32_t)) {
      uint32_t first, last;
      std::memcpy(&first, value.data(), sizeof(first));
      std::memcpy(&last, value.data() + value.size() - sizeof(last),
                  sizeof(last));
      return needs_escape(uint64_t{first} | (uint64_t{last} << 32));
    }
    for (unsigned char byte : value) {
      if (byte < 0x20 || byte >= 0x80 || byte == '"' || byte == '\\') {
        return true;
      }
    }
    return false;
  }

#if defined(__GNUC__)
  [[gnu::noinline]]
#endif
  void write_escaped(std::string_view value) {
    push_back('"');
    write_string_with_escape(value.data(), value.size(), *this);
    push_back('"');
  }

  template <size_t N>
  void write_small_integer(const std::array<char, N> &prefix, unsigned value) {
    if (static_cast<size_t>(end_ - next_) < N + 3) [[unlikely]] {
      grow(N + 3);
    }
    if constexpr (N != 0) {
      std::memcpy(next_, prefix.data(), N);
    }
    auto *destination = next_ + N;
    if (value >= 100) {
      destination[0] = static_cast<char>('0' + value / 100);
      std::memcpy(destination + 1, dec_::digits.data() + (value % 100) * 2, 2);
      next_ = destination + 3;
    }
    else if (value >= 10) {
      std::memcpy(destination, dec_::digits.data() + value * 2, 2);
      next_ = destination + 2;
    }
    else {
      destination[0] = static_cast<char>('0' + value);
      next_ = destination + 1;
    }
  }

  template <typename Integer>
#if defined(__GNUC__)
  [[gnu::noinline]]
#endif
  void write_wide_integer(Integer value) {
    constexpr auto width = std::numeric_limits<Integer>::digits10 + 2;
    auto *destination = prepare(width);
    next_ = std::to_chars(destination, destination + width, value).ptr;
  }

  std::string &output_;
  char *next_;
  char *end_;
};

template <typename T>
inline constexpr bool json_string_sink_v =
    std::is_same_v<std::remove_cvref_t<T>, json_string_sink>;

}  // namespace iguana::detail
