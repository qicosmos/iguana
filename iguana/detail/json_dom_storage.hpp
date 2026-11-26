#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "charconv.h"
#include "utf.hpp"

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#endif

namespace iguana {

enum class json_errc {
  unexpected_end = 1,
  unexpected_token,
  expected_key,
  expected_colon,
  expected_separator,
  invalid_number,
  invalid_string,
  invalid_escape,
  invalid_utf8,
  duplicate_key,
  trailing_input,
  nesting_limit,
  capacity
};

namespace detail {

class json_error_category : public std::error_category {
 public:
  const char* name() const noexcept override { return "iguana::json"; }
  std::string message(int n) const override {
    switch (static_cast<json_errc>(n)) {
      case json_errc::unexpected_end:
        return "unexpected end of JSON";
      case json_errc::unexpected_token:
        return "unexpected JSON token";
      case json_errc::expected_key:
        return "expected an object key";
      case json_errc::expected_colon:
        return "expected ':' after object key";
      case json_errc::expected_separator:
        return "expected a comma or closing delimiter";
      case json_errc::invalid_number:
        return "invalid JSON number";
      case json_errc::invalid_string:
        return "invalid JSON string";
      case json_errc::invalid_escape:
        return "invalid JSON escape";
      case json_errc::invalid_utf8:
        return "invalid UTF-8";
      case json_errc::duplicate_key:
        return "duplicate object key";
      case json_errc::trailing_input:
        return "trailing input after JSON value";
      case json_errc::nesting_limit:
        return "JSON nesting exceeds 512 containers";
      case json_errc::capacity:
        return "JSON exceeds the DOM capacity";
    }
    return "unknown JSON error";
  }
};

}  // namespace detail

inline std::error_code make_error_code(json_errc error) noexcept {
  static const detail::json_error_category category;
  return {static_cast<int>(error), category};
}

class json_parse_error : public std::runtime_error {
 public:
  json_parse_error(json_errc reason, size_t offset)
      : std::runtime_error(make_error_code(reason).message() + " at byte " +
                           std::to_string(offset)),
        reason_(reason),
        offset_(offset) {}

  std::error_code code() const noexcept { return make_error_code(reason_); }
  size_t offset() const noexcept { return offset_; }

 private:
  json_errc reason_;
  size_t offset_;
};

namespace detail {

enum class json_kind : uint8_t {
  null,
  boolean,
  integer,
  unsigned_integer,
  real,
  string,
  array,
  object
};

using json_index = uint32_t;
inline constexpr json_index no_json_index =
    std::numeric_limits<json_index>::max();
inline constexpr unsigned json_max_depth = 512;

inline json_index dom_index(size_t n) {
  if (n >= no_json_index) {
    throw std::length_error("JSON exceeds the DOM capacity");
  }
  return static_cast<json_index>(n);
}

struct json_node {
  uint64_t data = 0;
  json_index size = 0;
  json_kind kind = json_kind::null;

  static json_node integer(int64_t value) noexcept {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return {bits, 0, json_kind::integer};
  }

  static json_node real(double value) noexcept {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return {bits, 0, json_kind::real};
  }

  int64_t as_integer() const noexcept {
    int64_t result;
    std::memcpy(&result, &data, sizeof(result));
    return result;
  }

  double as_real() const noexcept {
    double result;
    std::memcpy(&result, &data, sizeof(result));
    return result;
  }

  json_index offset() const noexcept { return static_cast<json_index>(data); }
  json_index capacity() const noexcept {
    return static_cast<json_index>(data >> 32);
  }
  bool dense_array() const noexcept {
    return kind == json_kind::array && capacity() == no_json_index;
  }

  void set_children(json_index offset, json_index capacity) noexcept {
    data = (static_cast<uint64_t>(capacity) << 32) | offset;
  }
};

struct json_child {
  json_index key = no_json_index;
  json_index value = no_json_index;
};

struct json_storage {
  static constexpr json_index linear_object_limit = 48;

  std::vector<json_node> nodes;
  std::vector<json_child> children;
  std::string strings;

  IGUANA_INLINE json_index add(json_node node = {}) {
    auto index = dom_index(nodes.size());
    nodes.push_back(node);
    return index;
  }

  json_node string_node(std::string_view str) {
    const auto offset = dom_index(strings.size());
    const auto size = dom_index(str.size());
    dom_index(strings.size() + size);
    strings.append(str);
    return {offset, size, json_kind::string};
  }

  std::string_view string(json_node node) const noexcept {
    return {strings.data() + node.offset(), node.size};
  }

  std::string_view key(json_child child) const noexcept {
    return string(nodes[child.key]);
  }

  json_child child(json_node node, json_index i) const noexcept {
    if (node.dense_array()) {
      return {no_json_index, node.offset() + i};
    }
    return children[node.offset() + i];
  }

  auto lower_bound(json_node node, std::string_view member) const {
    auto first = children.begin() + node.offset();
    return std::lower_bound(first, first + node.size, member,
                            [&](json_child child, std::string_view name) {
                              return key(child) < name;
                            });
  }

  json_index find_position(json_node node, std::string_view member) const {
    if (node.size == 0) {
      return no_json_index;
    }
    auto first = children.begin() + node.offset();
    auto last = first + node.size;
    if (node.size <= linear_object_limit) {
      auto found = std::find_if(first, last, [&](json_child child) {
        return key(child) == member;
      });
      return found == last ? no_json_index
                           : static_cast<json_index>(found - first);
    }
    auto found = lower_bound(node, member);
    return found != last && key(*found) == member
               ? static_cast<json_index>(found - first)
               : no_json_index;
  }

  json_index find(json_index index, std::string_view member) const {
    const auto node = nodes[index];
    if (node.kind == json_kind::object) {
      auto position = find_position(node, member);
      if (position != no_json_index) {
        return child(node, position).value;
      }
    }
    return no_json_index;
  }

  template <typename Iterator>
  void sort_members(Iterator first, Iterator last) const {
    std::sort(first, last, [&](json_child a, json_child b) {
      return key(a) < key(b);
    });
  }

  void allocate_children(json_node& node, json_index capacity) {
    const auto offset = dom_index(children.size());
    children.resize(dom_index(size_t(offset) + capacity));
    for (json_index i = 0; i < node.size; ++i) {
      children[offset + i] = child(node, i);
    }
    node.set_children(offset, capacity);
  }

  json_child& insert_child(json_index index, json_index position) {
    auto& node = nodes[index];
    if (node.dense_array() || node.size == node.capacity()) {
      allocate_children(node,
                        dom_index(std::max<size_t>(4, size_t(node.size) * 2)));
    }
    auto begin = children.begin() + node.offset();
    std::move_backward(begin + position, begin + node.size,
                       begin + node.size + 1);
    ++node.size;
    return begin[position];
  }

  json_index insert(json_index index, std::string_view member) {
    const auto node = nodes[index];
    auto position = node.size;
    if (node.size <= linear_object_limit) {
      auto existing = find_position(node, member);
      if (existing != no_json_index) {
        return child(node, existing).value;
      }
    }
    else {
      auto first = children.cbegin() + node.offset();
      auto found = lower_bound(node, member);
      if (found != first + node.size && key(*found) == member) {
        return found->value;
      }
      position = static_cast<json_index>(found - first);
    }
    auto k = add(string_node(member));
    auto v = add();
    insert_child(index, position) = {k, v};
    if (node.size == linear_object_limit) {
      auto first = children.begin() + nodes[index].offset();
      sort_members(first, first + nodes[index].size);
    }
    return v;
  }

  void erase_child(json_index index, json_index position) {
    auto& node = nodes[index];
    if (position + 1 != node.size) {
      if (node.dense_array()) {
        allocate_children(node, node.size);
      }
      auto first = children.begin() + node.offset() + position;
      std::move(first + 1, children.begin() + node.offset() + node.size, first);
    }
    --node.size;
  }

  // Copy a subtree, including when source and destination share this arena.
  json_node clone(const json_storage& source, json_index index,
                  unsigned depth = 0) {
    auto node = source.nodes[index];
    if (node.kind == json_kind::string) {
      return this == &source ? node : string_node(source.string(node));
    }
    if (node.kind != json_kind::array && node.kind != json_kind::object) {
      return node;
    }
    if (depth == json_max_depth) {
      throw std::length_error("JSON nesting exceeds 512 containers");
    }
    const auto original = node;
    if (node.dense_array()) {
      node.set_children(dom_index(nodes.size()), no_json_index);
      for (json_index i = 0; i < node.size; ++i) {
        add(clone(source, original.offset() + i, depth + 1));
      }
      return node;
    }
    const auto offset = dom_index(children.size());
    children.resize(dom_index(size_t(offset) + node.size));
    node.set_children(offset, node.size);
    for (json_index i = 0; i < node.size; ++i) {
      auto child = source.child(original, i);
      if (child.key != no_json_index) {
        child.key = add(clone(source, child.key, depth + 1));
      }
      child.value = add(clone(source, child.value, depth + 1));
      children[offset + i] = child;
    }
    return node;
  }
};

inline bool json_digit(char c) noexcept {
  return static_cast<unsigned>(c - '0') < 10;
}

inline bool json_space(char c) noexcept {
  return c == ' ' || c == '\n' || (c | 4) == '\r';
}

IGUANA_INLINE const char* json_skip_space(const char* first,
                                          const char* last) noexcept {
  if (first == last || !json_space(*first)) {
    return first;
  }
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  for (; last - first >= 16; first += 16) {
    auto bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(first));
    auto space = _mm_or_si128(_mm_cmpeq_epi8(bytes, _mm_set1_epi8(' ')),
                              _mm_cmpeq_epi8(bytes, _mm_set1_epi8('\n')));
    auto tab_or_cr = _mm_cmpeq_epi8(_mm_or_si128(bytes, _mm_set1_epi8(4)),
                                    _mm_set1_epi8('\r'));
    auto mask = static_cast<unsigned>(
                    _mm_movemask_epi8(_mm_or_si128(space, tab_or_cr))) ^
                0xffffu;
    if (mask) {
      return first + countr_zero(mask);
    }
  }
#endif
  while (first != last && json_space(*first)) {
    ++first;
  }
  return first;
}

// Returns the next quote, escape, control character or non-ASCII byte.
IGUANA_INLINE const char* json_ascii_end(const char* first,
                                         const char* last) noexcept {
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  for (; last - first >= 16; first += 16) {
    const auto bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(first));
    const auto stop =
        _mm_or_si128(_mm_cmplt_epi8(bytes, _mm_set1_epi8(0x20)),
                     _mm_or_si128(_mm_cmpeq_epi8(bytes, _mm_set1_epi8('"')),
                                  _mm_cmpeq_epi8(bytes, _mm_set1_epi8('\\'))));
    if (auto mask = _mm_movemask_epi8(stop)) {
      return first + countr_zero(static_cast<unsigned>(mask));
    }
  }
#endif
  while (first != last && static_cast<unsigned char>(*first) - 0x20u < 0x60u &&
         *first != '"' && *first != '\\') {
    ++first;
  }
  return first;
}

IGUANA_INLINE const char* json_utf8_end(const char* first,
                                        const char* last) noexcept {
  const auto lead = static_cast<unsigned char>(*first);
  if (lead >= 0xE0 && lead < 0xF0 && last - first >= 3) {
    uint16_t continuation;
    std::memcpy(&continuation, first + 1, sizeof(continuation));
    if ((continuation & 0xC0C0) == 0x8080) {
      const auto second = static_cast<unsigned char>(first[1]);
      if ((lead != 0xE0 || second >= 0xA0) && (lead != 0xED || second < 0xA0)) {
        return first + 3;
      }
    }
    return nullptr;
  }
  if (lead >= 0xC2 && lead < 0xE0 && last - first >= 2) {
    return (static_cast<unsigned char>(first[1]) & 0xC0) == 0x80 ? first + 2
                                                                 : nullptr;
  }
  if (lead >= 0xF0 && lead <= 0xF4 && last - first >= 4) {
    const auto second = static_cast<unsigned char>(first[1]);
    uint16_t continuation;
    std::memcpy(&continuation, first + 2, sizeof(continuation));
    if ((continuation & 0xC0C0) == 0x8080 && (second & 0xC0) == 0x80 &&
        (lead != 0xF0 || second >= 0x90) && (lead != 0xF4 || second < 0x90)) {
      return first + 4;
    }
  }
  return nullptr;
}

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
// The block starts on a code point boundary. Return the length of the
// complete prefix, or zero to let the scalar path check special lead bytes
// and report errors at their original offset.
IGUANA_INLINE unsigned json_utf8_prefix(__m128i bytes) noexcept {
  const auto non_ascii = static_cast<unsigned>(_mm_movemask_epi8(bytes));
  if (!non_ascii) {
    return 16;
  }
  const auto continuation = static_cast<unsigned>(
      _mm_movemask_epi8(_mm_cmplt_epi8(bytes, _mm_set1_epi8(-64))));
  const auto leads = non_ascii ^ continuation;
  const auto long_leads =
      non_ascii & static_cast<unsigned>(_mm_movemask_epi8(
                      _mm_cmpgt_epi8(bytes, _mm_set1_epi8(-33))));
  const auto four_byte =
      non_ascii & static_cast<unsigned>(_mm_movemask_epi8(
                      _mm_cmpgt_epi8(bytes, _mm_set1_epi8(-17))));
  // Each lead requires one continuation, and the longer forms require more.
  const auto expected = (leads << 1) | (long_leads << 2) | (four_byte << 3);
  if (continuation != (expected & 0xffffu)) {
    return 0;
  }
  const auto invalid = _mm_or_si128(_mm_cmplt_epi8(bytes, _mm_set1_epi8(-62)),
                                    _mm_cmpgt_epi8(bytes, _mm_set1_epi8(-12)));
  if (leads & static_cast<unsigned>(_mm_movemask_epi8(invalid))) {
    return 0;
  }
  // E0, ED, F0 and F4 have extra restrictions on the next byte.
  const auto special =
      _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(bytes, _mm_set1_epi8(-32)),
                                _mm_cmpeq_epi8(bytes, _mm_set1_epi8(-19))),
                   _mm_or_si128(_mm_cmpeq_epi8(bytes, _mm_set1_epi8(-16)),
                                _mm_cmpeq_epi8(bytes, _mm_set1_epi8(-12))));
  if (_mm_movemask_epi8(special)) {
    return 0;
  }
  const auto incomplete =
      (leads & 0x8000u) | (long_leads & 0x4000u) | (four_byte & 0x2000u);
  return incomplete ? countr_zero(incomplete) : 16;
}
#endif

template <typename Word>
IGUANA_INLINE bool json_equal_key_words(std::string_view a,
                                        std::string_view b) noexcept {
  Word a_first, b_first, a_last, b_last;
  const auto tail = a.size() - sizeof(Word);
  std::memcpy(&a_first, a.data(), sizeof(Word));
  std::memcpy(&b_first, b.data(), sizeof(Word));
  std::memcpy(&a_last, a.data() + tail, sizeof(Word));
  std::memcpy(&b_last, b.data() + tail, sizeof(Word));
  return ((a_first ^ b_first) | (a_last ^ b_last)) == 0;
}

IGUANA_INLINE bool json_equal_keys(std::string_view a,
                                   std::string_view b) noexcept {
  const auto size = a.size();
  if (size != b.size()) {
    return false;
  }
  if (size == 0 || a.data() == b.data()) {
    return true;
  }
  if (size <= 16) {
    if (size >= 8) {
      return json_equal_key_words<uint64_t>(a, b);
    }
    if (size >= 4) {
      return json_equal_key_words<uint32_t>(a, b);
    }
    return a.front() == b.front() && a.back() == b.back() &&
           a[size / 2] == b[size / 2];
  }
  return std::memcmp(a.data(), b.data(), size) == 0;
}

inline constexpr auto json_hex_digits = [] {
  std::array<uint8_t, 256> digits{};
  for (auto& digit : digits) {
    digit = 0xff;
  }
  for (unsigned i = 0; i < 10; ++i) {
    digits['0' + i] = static_cast<uint8_t>(i);
  }
  for (unsigned i = 0; i < 6; ++i) {
    digits['a' + i] = digits['A' + i] = static_cast<uint8_t>(i + 10);
  }
  return digits;
}();

class json_parser {
 public:
  explicit json_parser(json_storage& storage) : storage_(storage) {}

  json_parser(json_storage& storage, std::string_view input)
      : json_parser(storage) {
    set_input(input);
  }

  json_index parse(std::string_view input) {
    storage_.nodes.clear();
    storage_.children.clear();
    storage_.strings.clear();
    pending_.clear();
    key_cache_.fill(0);
    next_key_.fill(0);
    set_input(input);
    return parse();
  }

  json_index parse() {
    if (static_cast<size_t>(end_ - begin_) >= no_json_index) {
      fail(json_errc::capacity);
    }
    if (end_ - p_ >= 3 && std::memcmp(p_, "\xef\xbb\xbf", 3) == 0) {
      p_ += 3;
    }
    reserve_storage(static_cast<size_t>(end_ - p_));
    skip_ws();
    auto root = parse_value(0);
    skip_ws();
    if (p_ != end_) {
      fail(json_errc::trailing_input);
    }
    return root;
  }

 private:
  void set_input(std::string_view input) noexcept {
    begin_ = input.empty() ? "" : input.data();
    p_ = begin_;
    end_ = begin_ + input.size();
  }

  void reserve_storage(size_t size) {
    if (size <= reserved_input_size_) {
      return;
    }
    auto node_capacity = size / 12 + 1;
    auto child_capacity = size / 16;
    auto string_capacity = std::min<size_t>(size / 8, 64 * 1024);
    if (size >= 128 * 1024) {
      // A small prefix distinguishes text-heavy input from dense values.
      // These are capacity estimates. The arenas can grow during parsing.
      constexpr size_t sample_size = 2048;
      const auto sample_end = p_ + sample_size;
      size_t values = 1;
      size_t keys = 0;
      size_t string_bytes = 0;
      auto cursor = p_;
      while (cursor != sample_end) {
        switch (*cursor++) {
          case '[':
          case '{':
          case ',':
            ++values;
            break;
          case ':':
            ++keys;
            break;
          case '"': {
            const auto first = cursor;
            while (cursor != sample_end) {
              const auto c = *cursor++;
              if (c == '"') {
                break;
              }
              if (c == '\\' && cursor != sample_end) {
                ++cursor;
              }
            }
            string_bytes += cursor - first;
            break;
          }
        }
      }
      const auto scale = static_cast<uint64_t>(size);
      // A short numeric prefix can underestimate the density of later arrays.
      if (string_bytes >= sample_size / 16) {
        node_capacity = static_cast<size_t>(
            std::clamp<uint64_t>((values + keys / 4) * scale / sample_size + 1,
                                 size / 24 + 1, node_capacity));
        child_capacity = static_cast<size_t>(
            std::min<uint64_t>(values * scale / sample_size, child_capacity));
      }
      string_capacity = static_cast<size_t>(std::min<uint64_t>(
          string_bytes * scale / sample_size, 4 * 1024 * 1024));
    }
    storage_.nodes.reserve(node_capacity);
    storage_.children.reserve(child_capacity);
    if (string_capacity > storage_.strings.capacity()) {
      storage_.strings.reserve(string_capacity);
    }
    reserved_input_size_ = size;
  }

  [[noreturn]] void fail(json_errc reason) const {
    throw json_parse_error(reason, static_cast<size_t>(p_ - begin_));
  }

  void skip_ws() noexcept { p_ = json_skip_space(p_, end_); }

  bool consume(char c) noexcept {
    if (p_ == end_ || *p_ != c) {
      return false;
    }
    ++p_;
    return true;
  }

  json_index literal(std::string_view word, json_node node) {
    if (static_cast<size_t>(end_ - p_) < word.size()) {
      fail(json_errc::unexpected_end);
    }
    if (std::memcmp(p_, word.data(), word.size()) != 0) {
      fail(json_errc::unexpected_token);
    }
    p_ += word.size();
    return storage_.add(node);
  }

  IGUANA_INLINE json_index parse_value(unsigned depth) {
    if (p_ == end_) {
      fail(json_errc::unexpected_end);
    }
    switch (*p_) {
      case 'n':
        return literal("null", {});
      case 't':
        return literal("true", {1, 0, json_kind::boolean});
      case 'f':
        return literal("false", {0, 0, json_kind::boolean});
      case '"':
        return storage_.add(parse_string());
      case '[':
        return parse_array(depth);
      case '{':
        return parse_object(depth);
      default:
        if (*p_ == '-' || json_digit(*p_)) {
          return storage_.add(parse_number());
        }
        fail(json_errc::unexpected_token);
    }
  }

  json_index open_scope(unsigned depth, json_kind kind) {
    if (depth == json_max_depth) {
      fail(json_errc::nesting_limit);
    }
    ++p_;
    skip_ws();
    return storage_.add({0, 0, kind});
  }

  json_index parse_array(unsigned depth) {
    auto index = open_scope(depth, json_kind::array);
    if (consume(']')) {
      return index;
    }
    const auto start = pending_.size();
    const auto first_child = dom_index(storage_.nodes.size());
    json_index count = 0;
    bool dense = true;
    do {
      skip_ws();
      auto value = parse_value(depth + 1);
      if (dense && storage_.nodes.size() != size_t(first_child) + count + 1) {
        for (json_index i = 0; i < count; ++i) {
          pending_.push_back({no_json_index, first_child + i});
        }
        dense = false;
      }
      if (!dense) {
        pending_.push_back({no_json_index, value});
      }
      ++count;
      skip_ws();
    } while (consume(','));
    if (!consume(']')) {
      fail(json_errc::expected_separator);
    }
    if (dense) {
      auto& node = storage_.nodes[index];
      node.size = count;
      node.set_children(first_child, no_json_index);
    }
    else {
      finish_scope(index, start);
    }
    return index;
  }

  json_index parse_object(unsigned depth) {
    auto index = open_scope(depth, json_kind::object);
    if (consume('}')) {
      return index;
    }
    const auto start = pending_.size();
    json_index previous = no_json_index;
    for (;;) {
      if (p_ == end_ || *p_ != '"') {
        fail(json_errc::expected_key);
      }
      auto key = parse_key(previous);
      previous = key;
      if (p_ == end_ || *p_ != ':') {
        skip_ws();
        if (p_ == end_ || *p_ != ':') {
          fail(json_errc::expected_colon);
        }
      }
      ++p_;
      if (p_ != end_ && *p_ == ' ') {
        ++p_;
      }
      skip_ws();
      auto value = parse_value(depth + 1);
      pending_.push_back({key, value});
      skip_ws();
      if (!consume(',')) {
        break;
      }
      skip_ws();
    }
    if (!consume('}')) {
      fail(json_errc::expected_separator);
    }
    auto first = pending_.begin() + start;
    if (pending_.size() - start > json_storage::linear_object_limit) {
      storage_.sort_members(first, pending_.end());
      auto same_key = [&](json_child a, json_child b) {
        return storage_.key(a) == storage_.key(b);
      };
      if (std::adjacent_find(first, pending_.end(), same_key) !=
          pending_.end()) {
        fail(json_errc::duplicate_key);
      }
    }
    else if (pending_.size() - start > 16) {
      validate_medium_object(first, pending_.end());
    }
    else {
      uint64_t seen = 0;
      for (auto it = first; it != pending_.end(); ++it) {
        auto key = storage_.key(*it);
        // A duplicate must share its bit. Resolve collisions with full keys.
        auto bit = uint64_t{1} << (key_hash(key) & 63);
        if ((seen & bit) && std::any_of(first, it, [&](json_child child) {
              return storage_.key(child) == key;
            })) {
          fail(json_errc::duplicate_key);
        }
        seen |= bit;
      }
    }
    finish_scope(index, start);
    return index;
  }

  static size_t key_hash(std::string_view key) noexcept {
    auto hash = key.size() * 11;
    if (!key.empty()) {
      hash += static_cast<unsigned char>(key.front()) * 17 +
              static_cast<unsigned char>(key.back()) * 13;
    }
    return hash;
  }

  void validate_medium_object(std::vector<json_child>::iterator first,
                              std::vector<json_child>::iterator last) {
    std::array<uint8_t, 128> positions{};
    for (auto it = first; it != last; ++it) {
      auto key = storage_.key(*it);
      auto slot = key_hash(key) & (positions.size() - 1);
      while (auto position = positions[slot]) {
        if (storage_.key(first[position - 1]) == key) {
          fail(json_errc::duplicate_key);
        }
        slot = (slot + 1) & (positions.size() - 1);
      }
      positions[slot] = static_cast<uint8_t>((it - first) + 1);
    }
  }

  void finish_scope(json_index index, size_t start) {
    auto& node = storage_.nodes[index];
    node.size = dom_index(pending_.size() - start);
    const auto offset = dom_index(storage_.children.size());
    dom_index(size_t(offset) + node.size);
    node.set_children(offset, node.size);
    storage_.children.insert(storage_.children.end(), pending_.begin() + start,
                             pending_.end());
    pending_.resize(start);
  }

  json_node parse_number() {
    const auto start = p_;
    const bool negative = consume('-');
    if (p_ == end_ || !json_digit(*p_)) {
      fail(json_errc::invalid_number);
    }
    const auto digits = p_;
    uint64_t value = static_cast<unsigned>(*p_++ - '0');
    if (value == 0 && p_ != end_ && json_digit(*p_)) {
      fail(json_errc::invalid_number);
    }
    while (p_ != end_ && json_digit(*p_)) {
      value = value * 10 + static_cast<unsigned>(*p_++ - '0');
    }
    auto digits_count = p_ - digits;
    if (p_ != end_ && *p_ == '.') {
      return parse_fraction(start, value, digits_count, negative);
    }
    if ((p_ == end_ || (*p_ != 'e' && *p_ != 'E')) && digits_count <= 20) {
      if (digits_count == 20) {
        auto result = std::from_chars(digits, p_, value);
        if (result.ec != std::errc{}) {
          return parse_real(start);
        }
      }
      if (negative) {
        constexpr auto min_magnitude = uint64_t{1} << 63;
        if (value == min_magnitude) {
          return json_node::integer(std::numeric_limits<int64_t>::min());
        }
        if (value < min_magnitude) {
          return json_node::integer(-static_cast<int64_t>(value));
        }
      }
      else if (value <=
               static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return json_node::integer(static_cast<int64_t>(value));
      }
      else {
        return {value, 0, json_kind::unsigned_integer};
      }
    }
    return parse_real(start);
  }

  json_node parse_fraction(const char* first, uint64_t significand,
                           std::ptrdiff_t integer_digits, bool negative) {
    const auto fraction = ++p_;
    if (p_ == end_ || !json_digit(*p_)) {
      fail(json_errc::invalid_number);
    }
    while (end_ - p_ >= 8 && fast_float::is_made_of_eight_digits_fast(p_)) {
      significand =
          significand * 100000000 + fast_float::parse_eight_digits_unrolled(p_);
      p_ += 8;
    }
    while (p_ != end_ && json_digit(*p_)) {
      significand = significand * 10 + static_cast<unsigned>(*p_++ - '0');
    }
    auto digits = p_ - fraction;
    if (integer_digits + digits <= 19 &&
        (p_ == end_ || (*p_ != 'e' && *p_ != 'E'))) {
      using format = fast_float::binary_format<double>;
      double value;
      if (significand <= format::max_mantissa_fast_path()) {
        value = static_cast<double>(significand) /
                format::exact_power_of_ten(digits);
        return json_node::real(negative ? -value : value);
      }
      auto mantissa = fast_float::compute_float<format>(-digits, significand);
      if (mantissa.power2 >= 0) {
        fast_float::to_float(negative, mantissa, value);
        return json_node::real(value);
      }
    }
    return parse_real(first);
  }

  json_node parse_real(const char* first) {
    double value{};
    auto result = fast_float::from_chars(first, end_, value);
    if (result.ec != std::errc{} || !std::isfinite(value)) {
      fail(json_errc::invalid_number);
    }
    p_ = result.ptr;
    return json_node::real(value);
  }

  unsigned hex4() {
    if (end_ - p_ < 4) {
      fail(json_errc::invalid_escape);
    }
    const auto a = json_hex_digits[static_cast<unsigned char>(p_[0])];
    const auto b = json_hex_digits[static_cast<unsigned char>(p_[1])];
    const auto c = json_hex_digits[static_cast<unsigned char>(p_[2])];
    const auto d = json_hex_digits[static_cast<unsigned char>(p_[3])];
    if ((a | b | c | d) > 15) {
      while (json_hex_digits[static_cast<unsigned char>(*p_)] <= 15) {
        ++p_;
      }
      fail(json_errc::invalid_escape);
    }
    p_ += 4;
    return (a << 12) | (b << 8) | (c << 4) | d;
  }

  struct escape_output {
    char* end;
    void push_back(char c) noexcept { *end++ = c; }
  };

  void escape(escape_output& out) {
    ++p_;
    if (p_ == end_) {
      fail(json_errc::unexpected_end);
    }
    const auto c = *p_++;
    switch (c) {
      case '"':
      case '\\':
      case '/':
        out.push_back(c);
        return;
      case 'b':
        out.push_back('\b');
        return;
      case 'f':
        out.push_back('\f');
        return;
      case 'n':
        out.push_back('\n');
        return;
      case 'r':
        out.push_back('\r');
        return;
      case 't':
        out.push_back('\t');
        return;
      case 'u': {
        auto code = hex4();
        if (code >= 0xD800 && code <= 0xDBFF) {
          if (!consume('\\') || !consume('u')) {
            fail(json_errc::invalid_escape);
          }
          auto low = hex4();
          if (low < 0xDC00 || low > 0xDFFF) {
            fail(json_errc::invalid_escape);
          }
          code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
        }
        else if (code >= 0xDC00 && code <= 0xDFFF) {
          fail(json_errc::invalid_escape);
        }
        encode_utf8(out, code);
        return;
      }
      default:
        fail(json_errc::invalid_escape);
    }
  }

  void append_escapes() {
    // An escape writes at most four UTF-8 bytes. Batch consecutive escapes
    // so a Unicode string can use one append for many decoded characters.
    char decoded[128];
    escape_output out{decoded};
    do {
      escape(out);
      if (out.end > decoded + sizeof(decoded) - 4) {
        storage_.strings.append(decoded, out.end - decoded);
        out.end = decoded;
      }
    } while (p_ != end_ && *p_ == '\\');
    storage_.strings.append(decoded, out.end - decoded);
  }

  IGUANA_INLINE json_node parse_string() {
    const auto first = ++p_;
    p_ = json_ascii_end(p_, end_);
    if (p_ != end_ && *p_ == '"') {
      return copy_string(first);
    }
    return parse_string_slow(first);
  }

  IGUANA_INLINE json_node copy_string(const char* first) {
    const auto offset = static_cast<json_index>(storage_.strings.size());
    const auto size = static_cast<json_index>(p_++ - first);
    storage_.strings.append(first, size);
    return {offset, size, json_kind::string};
  }

  IGUANA_INLINE json_index parse_key(json_index previous) {
    const auto first = ++p_;
    auto& prediction = next_key_[previous & (next_key_.size() - 1)];
    if (prediction) {
      // The table only records unescaped ASCII keys. Compare their bytes
      // and the closing quote before reusing the stored key.
      auto known = storage_.string(storage_.nodes[prediction - 1]);
      if (static_cast<size_t>(end_ - first) > known.size() &&
          first[known.size()] == '"' &&
          json_equal_keys(known, {first, known.size()})) {
        p_ = first + known.size() + 1;
        return prediction - 1;
      }
    }
    p_ = json_ascii_end(p_, end_);
    if (p_ == end_ || *p_ != '"') {
      return storage_.add(parse_string_slow(first));
    }
    std::string_view key(first, p_ - first);
    auto& entry = key_cache_[key_hash(key) & (key_cache_.size() - 1)];
    if (entry &&
        json_equal_keys(storage_.string(storage_.nodes[entry - 1]), key)) {
      ++p_;
      prediction = entry;
      return entry - 1;
    }
    auto index = storage_.add(copy_string(first));
    prediction = entry = index + 1;
    return index;
  }

  json_node parse_string_slow(const char* first) {
    const auto offset = static_cast<json_index>(storage_.strings.size());
    while (true) {
      if (p_ == end_) {
        fail(json_errc::unexpected_end);
      }
      if (*p_ == '"') {
        storage_.strings.append(first, p_ - first);
        ++p_;
        return {offset,
                static_cast<json_index>(storage_.strings.size() - offset),
                json_kind::string};
      }
      if (*p_ == '\\') {
        storage_.strings.append(first, p_ - first);
        append_escapes();
        first = p_;
      }
      else if (static_cast<unsigned char>(*p_) < 0x20) {
        fail(json_errc::invalid_string);
      }
      else {
        skip_utf8();
      }
      p_ = json_ascii_end(p_, end_);
    }
  }

  void skip_utf8() {
    do {
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
      if (end_ - p_ >= 16) {
        const auto bytes =
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(p_));
        if (auto count = json_utf8_prefix(bytes)) {
          const auto control = _mm_cmpeq_epi8(
              _mm_max_epu8(bytes, _mm_set1_epi8(31)), _mm_set1_epi8(31));
          const auto delimiter =
              _mm_or_si128(_mm_cmpeq_epi8(bytes, _mm_set1_epi8('"')),
                           _mm_cmpeq_epi8(bytes, _mm_set1_epi8('\\')));
          const auto stop = static_cast<unsigned>(
              _mm_movemask_epi8(_mm_or_si128(control, delimiter)));
          // An ASCII delimiter cannot occur inside a validated code point.
          if (stop) {
            p_ += countr_zero(stop);
            return;
          }
          p_ += count;
          continue;
        }
      }
#endif
      const auto next = json_utf8_end(p_, end_);
      if (!next) {
        fail(json_errc::invalid_utf8);
      }
      p_ = next;
    } while (p_ != end_ && static_cast<unsigned char>(*p_) >= 0x80);
  }

  json_storage& storage_;
  const char* begin_ = "";
  const char* p_ = begin_;
  const char* end_ = begin_;
  std::vector<json_child> pending_;
  size_t reserved_input_size_ = 0;
  std::array<json_index, 256> key_cache_{};
  std::array<json_index, 512> next_key_{};
};

}  // namespace detail
}  // namespace iguana
