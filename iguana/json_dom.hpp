#pragma once

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__AVX2__)
#include <immintrin.h>
#define IGUANA_JSON_DOM_HAS_AVX2 1
#endif

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define IGUANA_JSON_DOM_HAS_SSE2 1
#endif

#include "detail/charconv.h"

#if defined(_MSC_VER)
#define IGUANA_JSON_DOM_ALWAYS_INLINE __forceinline
#define IGUANA_JSON_DOM_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define IGUANA_JSON_DOM_ALWAYS_INLINE inline __attribute__((always_inline))
#define IGUANA_JSON_DOM_NOINLINE __attribute__((noinline))
#else
#define IGUANA_JSON_DOM_ALWAYS_INLINE inline
#define IGUANA_JSON_DOM_NOINLINE
#endif

namespace iguana::json {

enum class kind : uint8_t {
  null,
  boolean,
  int64,
  uint64,
  number,
  raw_number,
  string,
  array,
  object,
};

inline std::string_view kind_name(kind k) noexcept {
  switch (k) {
    case kind::null:
      return "null";
    case kind::boolean:
      return "boolean";
    case kind::int64:
      return "int64";
    case kind::uint64:
      return "uint64";
    case kind::number:
      return "number";
    case kind::raw_number:
      return "raw_number";
    case kind::string:
      return "string";
    case kind::array:
      return "array";
    case kind::object:
      return "object";
  }
  return "unknown";
}

enum class number_mode : uint8_t { eager, lazy };

struct parse_options {
  bool reject_duplicate_keys = false;
  bool validate_utf8 = true;
  bool validate_numbers = true;
  number_mode numbers = number_mode::eager;
};

class parse_error : public std::runtime_error {
 public:
  parse_error(size_t offset, std::string_view message)
      : std::runtime_error(make_message(offset, message)), offset_(offset) {}

  size_t offset() const noexcept { return offset_; }

 private:
  static std::string make_message(size_t offset, std::string_view message) {
    std::string out("json parse error at byte ");
    out += std::to_string(offset);
    out += ": ";
    out.append(message.data(), message.size());
    return out;
  }

  size_t offset_;
};

class document;
class value;
class array_view;
class object_view;

namespace detail {
template <number_mode Numbers, bool ValidateUtf8, bool ValidateNumbers,
          bool RejectDuplicateKeys>
class dom_parser;
class trusted_dom_parser;
template <class Consumer>
class structural_scanner;
struct node;
inline constexpr uint32_t invalid_index =
    (std::numeric_limits<uint32_t>::max)();
inline constexpr size_t max_pre_reserved_nodes = 16 * 1024 * 1024;
inline constexpr size_t max_pre_reserved_members = 8 * 1024 * 1024;
inline constexpr size_t max_pre_reserved_structurals = 32 * 1024 * 1024;

template <class>
inline constexpr bool always_false_v = false;

template <class T>
struct default_init_allocator : std::allocator<T> {
  using value_type = T;

  default_init_allocator() = default;

  template <class U>
  default_init_allocator(const default_init_allocator<U>&) noexcept {}

  template <class U>
  struct rebind {
    using other = default_init_allocator<U>;
  };

  template <class U>
  void construct(U* p) noexcept(std::is_nothrow_default_constructible_v<U>) {
    ::new (static_cast<void*>(p)) U;
  }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
};

template <class T>
class dom_vector {
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);

 public:
  dom_vector() = default;

  dom_vector(const dom_vector& other) { assign(other); }

  dom_vector& operator=(const dom_vector& other) {
    if (this != &other) {
      assign(other);
    }
    return *this;
  }

  dom_vector(dom_vector&& other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0)) {}

  dom_vector& operator=(dom_vector&& other) noexcept {
    if (this != &other) {
      ::operator delete(data_);
      data_ = std::exchange(other.data_, nullptr);
      size_ = std::exchange(other.size_, 0);
      capacity_ = std::exchange(other.capacity_, 0);
    }
    return *this;
  }

  ~dom_vector() { ::operator delete(data_); }

  size_t size() const noexcept { return size_; }
  size_t capacity() const noexcept { return capacity_; }
  bool empty() const noexcept { return size_ == 0; }
  T* data() noexcept { return data_; }
  const T* data() const noexcept { return data_; }

  T& operator[](size_t index) noexcept { return data_[index]; }
  const T& operator[](size_t index) const noexcept { return data_[index]; }
  T& back() noexcept { return data_[size_ - 1]; }
  const T& back() const noexcept { return data_[size_ - 1]; }

  void clear() noexcept { size_ = 0; }

  void resize_uninitialized(size_t new_size) {
    if (new_size > capacity_) {
      reserve(new_size);
    }
    size_ = new_size;
  }

  T* append_uninitialized(size_t count) {
    ensure_additional(count);
    auto* const result = data_ + size_;
    size_ += count;
    return result;
  }

  void append(const T* values, size_t count) {
    auto* const destination = append_uninitialized(count);
    if (count != 0) {
      std::memcpy(destination, values, count * sizeof(T));
    }
  }

  void reserve(size_t new_capacity) {
    if (new_capacity <= capacity_) {
      return;
    }

    auto* next = static_cast<T*>(::operator new(sizeof(T) * new_capacity));
    if (size_ != 0) {
      std::memcpy(next, data_, size_ * sizeof(T));
    }
    ::operator delete(data_);
    data_ = next;
    capacity_ = new_capacity;
  }

  void ensure_additional(size_t count) {
    if (capacity_ - size_ >= count) {
      return;
    }
    const auto required = size_ + count;
    const auto grown = capacity_ == 0 ? 64 : capacity_ * 2;
    reserve((std::max)(required, grown));
  }

  T& emplace_back() {
    ensure_one_more();
    auto* slot = data_ + size_++;
    ::new (static_cast<void*>(slot)) T;
    return *slot;
  }

  T& emplace_back_unchecked() noexcept {
    auto* slot = data_ + size_++;
    ::new (static_cast<void*>(slot)) T;
    return *slot;
  }

  void push_back(const T& value) {
    ensure_one_more();
    std::construct_at(data_ + size_++, value);
  }

  template <class... Args>
  T& emplace_back(Args&&... args) {
    ensure_one_more();
    auto* slot = data_ + size_++;
    return *std::construct_at(slot, std::forward<Args>(args)...);
  }

  template <class... Args>
  T& emplace_back_unchecked(Args&&... args) noexcept {
    auto* slot = data_ + size_++;
    return *std::construct_at(slot, std::forward<Args>(args)...);
  }

 private:
  void assign(const dom_vector& other) {
    clear();
    reserve(other.size_);
    size_ = other.size_;
    if (size_ != 0) {
      std::memcpy(data_, other.data_, size_ * sizeof(T));
    }
  }

  void ensure_one_more() {
    if (size_ == capacity_) [[unlikely]] {
      reserve(capacity_ == 0 ? 64 : capacity_ * 2);
    }
  }

  T* data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;
};
}  // namespace detail

class value {
 public:
  value() = default;

  bool valid() const noexcept { return doc_ != nullptr && id_ != invalid_id; }
  explicit operator bool() const noexcept { return valid(); }

  kind type() const;
  bool is_null() const { return type() == kind::null; }
  bool is_bool() const { return type() == kind::boolean; }
  bool is_int64() const { return type() == kind::int64; }
  bool is_uint64() const { return type() == kind::uint64; }
  bool is_number() const;
  bool is_string() const { return type() == kind::string; }
  bool is_array() const { return type() == kind::array; }
  bool is_object() const { return type() == kind::object; }

  bool as_bool() const;
  int64_t as_int64() const;
  uint64_t as_uint64() const;
  double as_double() const;
  std::string_view number_token() const;
  std::string_view as_string_view() const;
  array_view as_array() const;
  object_view as_object() const;

  size_t size() const;
  bool empty() const { return size() == 0; }

  value at(size_t index) const;
  value at(std::string_view key) const;
  value at(const char* key) const { return at(std::string_view(key)); }
  value at(int index) const {
    if (index < 0) {
      throw std::out_of_range("json array index is out of range");
    }
    return at(static_cast<size_t>(index));
  }
  value operator[](size_t index) const { return at(index); }
  value operator[](int index) const { return at(index); }
  value operator[](std::string_view key) const { return at(key); }
  value operator[](const char* key) const { return at(key); }
  value find(std::string_view key) const noexcept;
  value find(const char* key) const noexcept {
    return find(std::string_view(key));
  }
  bool contains(std::string_view key) const noexcept { return !!find(key); }
  bool contains(const char* key) const noexcept {
    return contains(std::string_view(key));
  }

  template <class T>
  T as() const {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      return as_bool();
    }
    else if constexpr (std::is_same_v<U, std::string_view>) {
      return as_string_view();
    }
    else if constexpr (std::is_same_v<U, std::string>) {
      return std::string(as_string_view());
    }
    else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>) {
      const auto v = as_int64();
      if (v < static_cast<int64_t>((std::numeric_limits<U>::min)()) ||
          v > static_cast<int64_t>((std::numeric_limits<U>::max)())) {
        throw std::out_of_range("json integer conversion is out of range");
      }
      return static_cast<U>(v);
    }
    else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>) {
      const auto v = as_uint64();
      if (v > static_cast<uint64_t>((std::numeric_limits<U>::max)())) {
        throw std::out_of_range(
            "json unsigned integer conversion is out of range");
      }
      return static_cast<U>(v);
    }
    else if constexpr (std::is_floating_point_v<U>) {
      return static_cast<U>(as_double());
    }
    else {
      static_assert(detail::always_false_v<U>,
                    "unsupported json::value::as<T>() type");
    }
  }

  template <class T>
  std::optional<T> try_as() const noexcept {
    try {
      return as<T>();
    } catch (...) {
      return std::nullopt;
    }
  }

  template <class T>
  T value_or(std::string_view key, T fallback) const {
    auto found = find(key);
    if (!found) {
      return fallback;
    }

    try {
      return found.as<T>();
    } catch (...) {
      return fallback;
    }
  }

 private:
  friend class document;
  friend class array_view;
  friend class object_view;

  static constexpr uint32_t invalid_id = detail::invalid_index;

  value(const document* doc, uint32_t id) : doc_(doc), id_(id) {}

  const detail::node& current() const;
  void require(kind expected) const;

  const document* doc_ = nullptr;
  uint32_t id_ = invalid_id;
};

class array_view {
 public:
  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = value;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value;

    iterator() = default;
    value operator*() const;
    iterator& operator++();
    iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }
    friend bool operator==(iterator lhs, iterator rhs) {
      return lhs.doc_ == rhs.doc_ && lhs.current_ == rhs.current_;
    }
    friend bool operator!=(iterator lhs, iterator rhs) { return !(lhs == rhs); }

   private:
    friend class array_view;
    iterator(const document* doc, uint32_t current, uint32_t end)
        : doc_(doc), current_(current), end_(end) {}

    const document* doc_ = nullptr;
    uint32_t current_ = detail::invalid_index;
    uint32_t end_ = detail::invalid_index;
  };

  array_view() = default;
  size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }
  value at(size_t index) const;
  value at(int index) const {
    if (index < 0) {
      throw std::out_of_range("json array index is out of range");
    }
    return at(static_cast<size_t>(index));
  }
  value operator[](size_t index) const { return at(index); }
  value operator[](int index) const { return at(index); }
  iterator begin() const { return iterator(doc_, first_, end_); }
  iterator end() const { return iterator(doc_, detail::invalid_index, end_); }

 private:
  friend class value;
  array_view(const document* doc, uint32_t first, uint32_t count,
             uint32_t end = detail::invalid_index)
      : doc_(doc), first_(first), count_(count), end_(end) {}

  const document* doc_ = nullptr;
  uint32_t first_ = 0;
  uint32_t count_ = 0;
  uint32_t end_ = detail::invalid_index;
};

struct object_item {
  std::string_view key;
  value val;
};

class object_view {
 public:
  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = object_item;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = object_item;

    iterator() = default;
    object_item operator*() const;
    iterator& operator++();
    iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }
    friend bool operator==(iterator lhs, iterator rhs) {
      return lhs.doc_ == rhs.doc_ && lhs.current_ == rhs.current_;
    }
    friend bool operator!=(iterator lhs, iterator rhs) { return !(lhs == rhs); }

   private:
    friend class object_view;
    iterator(const document* doc, uint32_t current, uint32_t end)
        : doc_(doc), current_(current), end_(end) {}

    const document* doc_ = nullptr;
    uint32_t current_ = detail::invalid_index;
    uint32_t end_ = detail::invalid_index;
  };

  object_view() = default;
  size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }
  value at(std::string_view key) const;
  value at(const char* key) const { return at(std::string_view(key)); }
  value operator[](std::string_view key) const { return at(key); }
  value operator[](const char* key) const { return at(key); }
  value find(std::string_view key) const noexcept;
  value find(const char* key) const noexcept {
    return find(std::string_view(key));
  }
  bool contains(std::string_view key) const noexcept { return !!find(key); }
  bool contains(const char* key) const noexcept {
    return contains(std::string_view(key));
  }
  iterator begin() const { return iterator(doc_, first_, end_); }
  iterator end() const { return iterator(doc_, detail::invalid_index, end_); }

 private:
  friend class value;
  object_view(const document* doc, uint32_t first, uint32_t count,
              uint32_t end = detail::invalid_index)
      : doc_(doc), first_(first), count_(count), end_(end) {}

  const document* doc_ = nullptr;
  uint32_t first_ = 0;
  uint32_t count_ = 0;
  uint32_t end_ = detail::invalid_index;
};

namespace detail {

enum class string_storage : uint8_t { input, pool };

struct string_ref {
  static constexpr uint64_t storage_bit = 1ull << 63u;
  static constexpr uint64_t length_mask = 0x7FFFFFFFull;

  uint64_t packed = 0;

  string_ref() = default;

  string_ref(string_storage storage, uint32_t offset, uint32_t length) noexcept
      : packed(static_cast<uint64_t>(offset) |
               (static_cast<uint64_t>(length) << 32u) |
               (storage == string_storage::pool ? storage_bit : 0u)) {}

  static string_ref input(uint32_t offset, uint32_t length) noexcept {
    string_ref ref;
    ref.packed =
        static_cast<uint64_t>(offset) | (static_cast<uint64_t>(length) << 32u);
    return ref;
  }

  static string_ref pool(uint32_t offset, uint32_t length) noexcept {
    string_ref ref;
    ref.packed = static_cast<uint64_t>(offset) |
                 (static_cast<uint64_t>(length) << 32u) | storage_bit;
    return ref;
  }

  string_storage storage() const noexcept {
    return (packed & storage_bit) != 0 ? string_storage::pool
                                       : string_storage::input;
  }

  uint32_t offset() const noexcept { return static_cast<uint32_t>(packed); }

  uint32_t length() const noexcept {
    return static_cast<uint32_t>((packed >> 32u) & length_mask);
  }
};

enum class tape_tag : uint8_t {
  null,
  boolean,
  int64,
  uint64,
  number,
  raw_number,
  string,
  array,
  object,
  key,
  int64_full,
  uint64_full,
};

inline constexpr uint64_t tape_tag_shift = 60u;
inline constexpr uint64_t tape_payload_mask = (uint64_t{1} << 60u) - 1u;
inline constexpr uint64_t tape_string_storage_bit = uint64_t{1} << 59u;
inline constexpr uint32_t tape_string_length_mask = 0x07FFFFFFu;
inline constexpr uint32_t tape_container_count_mask = 0x0FFFFFFFu;

inline constexpr uint64_t make_tape(tape_tag tag,
                                    uint64_t payload = 0) noexcept {
  return (static_cast<uint64_t>(tag) << tape_tag_shift) |
         (payload & tape_payload_mask);
}

inline constexpr tape_tag get_tape_tag(uint64_t word) noexcept {
  return static_cast<tape_tag>(word >> tape_tag_shift);
}

inline constexpr uint64_t get_tape_payload(uint64_t word) noexcept {
  return word & tape_payload_mask;
}

inline uint64_t pack_tape_string(string_ref ref) noexcept {
  return static_cast<uint64_t>(ref.offset()) |
         (static_cast<uint64_t>(ref.length()) << 32u) |
         (ref.storage() == string_storage::pool ? tape_string_storage_bit : 0u);
}

inline string_ref unpack_tape_string(uint64_t word) noexcept {
  const auto payload = get_tape_payload(word);
  return {((payload & tape_string_storage_bit) != 0) ? string_storage::pool
                                                     : string_storage::input,
          static_cast<uint32_t>(payload),
          static_cast<uint32_t>((payload >> 32u) & tape_string_length_mask)};
}

inline constexpr uint64_t pack_tape_container(uint32_t end,
                                              uint32_t count) noexcept {
  return static_cast<uint64_t>(end) | (static_cast<uint64_t>(count) << 32u);
}

inline constexpr int64_t unpack_tape_int64(uint64_t word) noexcept {
  const auto payload = get_tape_payload(word);
  return (payload & (uint64_t{1} << 59u)) != 0
             ? static_cast<int64_t>(payload | ~tape_payload_mask)
             : static_cast<int64_t>(payload);
}

struct node {
  union {
    int64_t i64;
    uint64_t u64;
    double f64;
  };
  uint32_t next;
  kind type;
  string_storage storage;
  uint16_t reserved;

  uint32_t first() const noexcept { return static_cast<uint32_t>(u64); }

  uint32_t count() const noexcept { return static_cast<uint32_t>(u64 >> 32u); }

  void set_span(uint32_t first, uint32_t count) noexcept {
    u64 = (static_cast<uint64_t>(count) << 32u) | first;
  }

  void set_first(uint32_t first) noexcept {
    u64 = (u64 & 0xFFFFFFFF00000000ull) | first;
  }

  void increment_count() noexcept { u64 += (1ull << 32u); }
};

struct member {
  string_ref key;
  uint32_t value;
  uint32_t next;

  member() = default;
  member(string_ref key_ref, uint32_t value_id) noexcept
      : key(key_ref), value(value_id) {}
};

static_assert(sizeof(node) == 16);
static_assert(sizeof(string_ref) == 8);
static_assert(sizeof(member) == 16);

struct parse_frame {
  kind type;
  uint32_t id;
  uint32_t first;
  uint32_t last;
  uint32_t count;
  string_ref key;
};

}  // namespace detail

class document {
 public:
  document() = default;

  document(const document& other)
      : input_(other.input_),
        input_view_(other.owns_input_
                        ? std::string_view(input_.data(), input_.size())
                        : other.input_view_),
        owns_input_(other.owns_input_),
        string_pool_(other.string_pool_),
        nodes_(other.nodes_),
        members_(other.members_),
        tape_(other.tape_),
        structural_indexes_(other.structural_indexes_),
        tape_mode_(other.tape_mode_),
        node_count_(other.node_count_),
        root_(other.root_) {}

  document& operator=(const document& other) {
    if (this == &other) {
      return *this;
    }

    input_ = other.input_;
    owns_input_ = other.owns_input_;
    input_view_ = owns_input_ ? std::string_view(input_.data(), input_.size())
                              : other.input_view_;
    string_pool_ = other.string_pool_;
    nodes_ = other.nodes_;
    members_ = other.members_;
    tape_ = other.tape_;
    structural_indexes_ = other.structural_indexes_;
    tape_mode_ = other.tape_mode_;
    node_count_ = other.node_count_;
    parse_stack_.clear();
    tape_parse_stack_.clear();
    root_ = other.root_;
    return *this;
  }

  document(document&& other) noexcept
      : input_(std::move(other.input_)),
        input_view_(other.owns_input_
                        ? std::string_view(input_.data(), input_.size())
                        : other.input_view_),
        owns_input_(other.owns_input_),
        string_pool_(std::move(other.string_pool_)),
        nodes_(std::move(other.nodes_)),
        members_(std::move(other.members_)),
        tape_(std::move(other.tape_)),
        structural_indexes_(std::move(other.structural_indexes_)),
        tape_mode_(other.tape_mode_),
        node_count_(other.node_count_),
        root_(other.root_) {
    other.input_view_ = {};
    other.root_ = value::invalid_id;
  }

  document& operator=(document&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    input_ = std::move(other.input_);
    owns_input_ = other.owns_input_;
    input_view_ = owns_input_ ? std::string_view(input_.data(), input_.size())
                              : other.input_view_;
    string_pool_ = std::move(other.string_pool_);
    nodes_ = std::move(other.nodes_);
    members_ = std::move(other.members_);
    tape_ = std::move(other.tape_);
    structural_indexes_ = std::move(other.structural_indexes_);
    tape_mode_ = other.tape_mode_;
    node_count_ = other.node_count_;
    parse_stack_.clear();
    tape_parse_stack_.clear();
    root_ = other.root_;
    other.input_view_ = {};
    other.root_ = value::invalid_id;
    return *this;
  }

  value root() const {
    if (root_ == value::invalid_id) {
      throw std::logic_error("json document is empty");
    }
    return value(this, root_);
  }
  value at(size_t index) const { return root().at(index); }
  value at(int index) const { return root().at(index); }
  value at(std::string_view key) const { return root().at(key); }
  value at(const char* key) const { return root().at(key); }
  value operator[](size_t index) const { return at(index); }
  value operator[](int index) const { return at(index); }
  value operator[](std::string_view key) const { return at(key); }
  value operator[](const char* key) const { return at(key); }
  value find(std::string_view key) const noexcept {
    if (root_ == value::invalid_id) {
      return {};
    }
    return value(this, root_).find(key);
  }
  value find(const char* key) const noexcept {
    return find(std::string_view(key));
  }
  bool contains(std::string_view key) const noexcept {
    return root().contains(key);
  }
  bool contains(const char* key) const noexcept {
    return contains(std::string_view(key));
  }

  size_t node_count() const noexcept {
    return tape_mode_ ? node_count_ : nodes_.size();
  }
  std::string_view source() const noexcept { return input_view_; }
  bool owns_input() const noexcept { return owns_input_; }

  void reserve(size_t input_size) {
    if (input_size >= detail::invalid_index) {
      throw std::length_error("json input exceeds uint32_t offset range");
    }
    reserve_for_input(input_size);
  }

 private:
  friend class value;
  friend class array_view;
  friend class array_view::iterator;
  friend class object_view;
  friend class object_view::iterator;
  template <number_mode, bool, bool, bool>
  friend class detail::dom_parser;
  friend class detail::trusted_dom_parser;
  template <class>
  friend class detail::structural_scanner;
  friend document parse(std::string_view input, parse_options options);
  friend document parse_view(std::string_view input, parse_options options);
  friend document parse_trusted_view(std::string_view input);
  friend void parse(document& doc, std::string_view input,
                    parse_options options);
  friend void parse_view(document& doc, std::string_view input,
                         parse_options options);
  friend void parse_trusted_view(document& doc, std::string_view input);

  using node = detail::node;
  using member = detail::member;
  using string_ref = detail::string_ref;
  using string_storage = detail::string_storage;

  static uint32_t checked_index(size_t value, std::string_view what) {
    if (value > (std::numeric_limits<uint32_t>::max)()) {
      std::string message;
      message.append(what.data(), what.size());
      message += " index exceeds uint32_t range";
      throw std::length_error(message);
    }
    return static_cast<uint32_t>(value);
  }

  void reserve_for_input(size_t input_size) {
    const auto tape_capacity =
        (std::min)(input_size + 1, detail::max_pre_reserved_nodes * 2);
    if (tape_.capacity() < tape_capacity) {
      tape_.reserve(tape_capacity);
    }
    if (tape_parse_stack_.capacity() < 256) {
      tape_parse_stack_.reserve(256);
    }
    const auto structural_capacity =
        (std::min)(input_size / 2 + 1, detail::max_pre_reserved_structurals);
    if (structural_indexes_.capacity() < structural_capacity) {
      structural_indexes_.reserve(structural_capacity);
    }
  }

  void reserve_trusted_for_input(size_t input_size) {
    const auto node_capacity =
        (std::min)(input_size / 2 + 1, detail::max_pre_reserved_nodes);
    const auto member_capacity =
        (std::min)(input_size / 4 + 1, detail::max_pre_reserved_members);
    if (nodes_.capacity() < node_capacity) {
      nodes_.reserve(node_capacity);
    }
    if (members_.capacity() < member_capacity) {
      members_.reserve(member_capacity);
    }
  }

  uint32_t add_node(node n) {
    const auto id = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back(n);
    return id;
  }

  std::string_view resolve(string_ref ref) const noexcept {
    if (ref.storage() == string_storage::pool) {
      return std::string_view(string_pool_.data() + ref.offset(), ref.length());
    }
    return std::string_view(input_view_.data() + ref.offset(), ref.length());
  }

  detail::tape_tag tape_tag_at(uint32_t id) const noexcept {
    return detail::get_tape_tag(tape_[id]);
  }

  uint64_t tape_payload_at(uint32_t id) const noexcept {
    return detail::get_tape_payload(tape_[id]);
  }

  uint32_t tape_container_end(uint32_t id) const noexcept {
    return static_cast<uint32_t>(tape_payload_at(id));
  }

  uint32_t tape_container_count(uint32_t id) const noexcept {
    return static_cast<uint32_t>(tape_payload_at(id) >> 32u);
  }

  uint32_t tape_next_value(uint32_t id) const noexcept {
    switch (tape_tag_at(id)) {
      case detail::tape_tag::int64:
      case detail::tape_tag::uint64:
        return id + 1;
      case detail::tape_tag::int64_full:
      case detail::tape_tag::uint64_full:
      case detail::tape_tag::number:
        return id + 2;
      case detail::tape_tag::array:
      case detail::tape_tag::object:
        return tape_container_end(id);
      default:
        return id + 1;
    }
  }

  std::string_view resolve_tape_string(uint32_t id) const noexcept {
    return resolve(detail::unpack_tape_string(tape_[id]));
  }

  std::string input_;
  std::string_view input_view_;
  bool owns_input_ = true;
  detail::dom_vector<char> string_pool_;
  detail::dom_vector<node> nodes_;
  detail::dom_vector<member> members_;
  detail::dom_vector<uint64_t> tape_;
  detail::dom_vector<uint32_t> structural_indexes_;
  std::vector<detail::parse_frame,
              detail::default_init_allocator<detail::parse_frame>>
      parse_stack_;
  std::vector<uint32_t, detail::default_init_allocator<uint32_t>>
      tape_parse_stack_;
  bool tape_mode_ = true;
  size_t node_count_ = 0;
  uint32_t root_ = value::invalid_id;
};

inline const detail::node& value::current() const {
  if (!valid()) {
    throw std::logic_error("json value is invalid");
  }
  return doc_->nodes_[id_];
}

inline void value::require(kind expected) const {
  const auto actual = type();
  if (actual != expected) {
    std::string message("json value is ");
    message.append(kind_name(actual));
    message += ", expected ";
    message.append(kind_name(expected));
    throw std::runtime_error(message);
  }
}

inline kind value::type() const {
  if (!valid()) {
    throw std::logic_error("json value is invalid");
  }
  if (!doc_->tape_mode_) {
    return current().type;
  }
  switch (doc_->tape_tag_at(id_)) {
    case detail::tape_tag::null:
      return kind::null;
    case detail::tape_tag::boolean:
      return kind::boolean;
    case detail::tape_tag::int64:
    case detail::tape_tag::int64_full:
      return kind::int64;
    case detail::tape_tag::uint64:
    case detail::tape_tag::uint64_full:
      return kind::uint64;
    case detail::tape_tag::number:
      return kind::number;
    case detail::tape_tag::raw_number:
      return kind::raw_number;
    case detail::tape_tag::string:
      return kind::string;
    case detail::tape_tag::array:
      return kind::array;
    case detail::tape_tag::object:
      return kind::object;
    case detail::tape_tag::key:
      break;
  }
  throw std::logic_error("json tape points to an object key");
}

inline bool value::is_number() const {
  const auto t = type();
  return t == kind::int64 || t == kind::uint64 || t == kind::number ||
         t == kind::raw_number;
}

inline bool value::as_bool() const {
  require(kind::boolean);
  return doc_->tape_mode_ ? doc_->tape_payload_at(id_) != 0
                          : current().u64 != 0;
}

inline std::string_view value::number_token() const {
  if (doc_->tape_mode_) {
    require(kind::raw_number);
    return doc_->resolve_tape_string(id_);
  }
  const auto& n = current();
  if (n.type != kind::raw_number) {
    std::string message("json value is ");
    message.append(kind_name(n.type));
    message += ", expected raw_number";
    throw std::runtime_error(message);
  }
  return doc_->resolve({n.storage, n.first(), n.count()});
}

inline int64_t value::as_int64() const {
  if (doc_->tape_mode_) {
    const auto tag = doc_->tape_tag_at(id_);
    if (tag == detail::tape_tag::int64) {
      return detail::unpack_tape_int64(doc_->tape_[id_]);
    }
    if (tag == detail::tape_tag::uint64 &&
        doc_->tape_payload_at(id_) <=
            static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
      return static_cast<int64_t>(doc_->tape_payload_at(id_));
    }
    if (tag == detail::tape_tag::int64_full) {
      return static_cast<int64_t>(doc_->tape_[id_ + 1]);
    }
    if (tag == detail::tape_tag::uint64_full &&
        doc_->tape_[id_ + 1] <=
            static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
      return static_cast<int64_t>(doc_->tape_[id_ + 1]);
    }
    if (tag == detail::tape_tag::raw_number) {
      const auto token = number_token();
      int64_t parsed = 0;
      const auto [ptr, ec] =
          std::from_chars(token.data(), token.data() + token.size(), parsed);
      if (ec == std::errc{} && ptr == token.data() + token.size()) {
        return parsed;
      }
    }
    std::string message("json value is ");
    message.append(kind_name(type()));
    message += ", expected int64";
    throw std::runtime_error(message);
  }
  const auto& n = current();
  if (n.type == kind::int64) {
    return n.i64;
  }
  if (n.type == kind::uint64 &&
      n.u64 <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
    return static_cast<int64_t>(n.u64);
  }
  if (n.type == kind::raw_number) {
    const auto token = number_token();
    int64_t parsed = 0;
    const auto [ptr, ec] =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (ec == std::errc{} && ptr == token.data() + token.size()) {
      return parsed;
    }
  }
  std::string message("json value is ");
  message.append(kind_name(n.type));
  message += ", expected int64";
  throw std::runtime_error(message);
}

inline uint64_t value::as_uint64() const {
  if (doc_->tape_mode_) {
    const auto tag = doc_->tape_tag_at(id_);
    if (tag == detail::tape_tag::uint64) {
      return doc_->tape_payload_at(id_);
    }
    if (tag == detail::tape_tag::int64 &&
        detail::unpack_tape_int64(doc_->tape_[id_]) >= 0) {
      return static_cast<uint64_t>(detail::unpack_tape_int64(doc_->tape_[id_]));
    }
    if (tag == detail::tape_tag::uint64_full) {
      return doc_->tape_[id_ + 1];
    }
    if (tag == detail::tape_tag::int64_full &&
        static_cast<int64_t>(doc_->tape_[id_ + 1]) >= 0) {
      return doc_->tape_[id_ + 1];
    }
    if (tag == detail::tape_tag::raw_number) {
      const auto token = number_token();
      uint64_t parsed = 0;
      const auto [ptr, ec] =
          std::from_chars(token.data(), token.data() + token.size(), parsed);
      if (ec == std::errc{} && ptr == token.data() + token.size()) {
        return parsed;
      }
    }
    std::string message("json value is ");
    message.append(kind_name(type()));
    message += ", expected uint64";
    throw std::runtime_error(message);
  }
  const auto& n = current();
  if (n.type == kind::uint64) {
    return n.u64;
  }
  if (n.type == kind::int64 && n.i64 >= 0) {
    return static_cast<uint64_t>(n.i64);
  }
  if (n.type == kind::raw_number) {
    const auto token = number_token();
    uint64_t parsed = 0;
    const auto [ptr, ec] =
        std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (ec == std::errc{} && ptr == token.data() + token.size()) {
      return parsed;
    }
  }
  std::string message("json value is ");
  message.append(kind_name(n.type));
  message += ", expected uint64";
  throw std::runtime_error(message);
}

inline double value::as_double() const {
  if (doc_->tape_mode_) {
    const auto tag = doc_->tape_tag_at(id_);
    if (tag == detail::tape_tag::number) {
      return std::bit_cast<double>(doc_->tape_[id_ + 1]);
    }
    if (tag == detail::tape_tag::int64) {
      return static_cast<double>(detail::unpack_tape_int64(doc_->tape_[id_]));
    }
    if (tag == detail::tape_tag::uint64) {
      return static_cast<double>(doc_->tape_payload_at(id_));
    }
    if (tag == detail::tape_tag::int64_full) {
      return static_cast<double>(static_cast<int64_t>(doc_->tape_[id_ + 1]));
    }
    if (tag == detail::tape_tag::uint64_full) {
      return static_cast<double>(doc_->tape_[id_ + 1]);
    }
    if (tag == detail::tape_tag::raw_number) {
      const auto token = number_token();
      double parsed = 0.0;
      const auto [ptr, ec] = iguana::detail::from_chars<false>(
          token.data(), token.data() + token.size(), parsed);
      if (ec == std::errc{} && ptr == token.data() + token.size()) {
        return parsed;
      }
    }
    std::string message("json value is ");
    message.append(kind_name(type()));
    message += ", expected number";
    throw std::runtime_error(message);
  }
  const auto& n = current();
  if (n.type == kind::number) {
    return n.f64;
  }
  if (n.type == kind::int64) {
    return static_cast<double>(n.i64);
  }
  if (n.type == kind::uint64) {
    return static_cast<double>(n.u64);
  }
  if (n.type == kind::raw_number) {
    const auto token = number_token();
    double parsed = 0.0;
    const auto [ptr, ec] = iguana::detail::from_chars<false>(
        token.data(), token.data() + token.size(), parsed);
    if (ec == std::errc{} && ptr == token.data() + token.size()) {
      return parsed;
    }
  }
  std::string message("json value is ");
  message.append(kind_name(n.type));
  message += ", expected number";
  throw std::runtime_error(message);
}

inline std::string_view value::as_string_view() const {
  require(kind::string);
  if (doc_->tape_mode_) {
    return doc_->resolve_tape_string(id_);
  }
  const auto& n = current();
  return doc_->resolve({n.storage, n.first(), n.count()});
}

inline array_view value::as_array() const {
  require(kind::array);
  if (doc_->tape_mode_) {
    const auto count = doc_->tape_container_count(id_);
    const auto end = doc_->tape_container_end(id_);
    return array_view(doc_, count == 0 ? detail::invalid_index : id_ + 1, count,
                      end);
  }
  const auto& n = current();
  return array_view(doc_, n.first(), n.count());
}

inline object_view value::as_object() const {
  require(kind::object);
  if (doc_->tape_mode_) {
    const auto count = doc_->tape_container_count(id_);
    const auto end = doc_->tape_container_end(id_);
    return object_view(doc_, count == 0 ? detail::invalid_index : id_ + 1,
                       count, end);
  }
  const auto& n = current();
  return object_view(doc_, n.first(), n.count());
}

inline size_t value::size() const {
  if (doc_->tape_mode_) {
    switch (doc_->tape_tag_at(id_)) {
      case detail::tape_tag::string:
      case detail::tape_tag::raw_number:
        return detail::unpack_tape_string(doc_->tape_[id_]).length();
      case detail::tape_tag::array:
      case detail::tape_tag::object:
        return doc_->tape_container_count(id_);
      default:
        return 0;
    }
  }
  const auto& n = current();
  switch (n.type) {
    case kind::string:
    case kind::array:
    case kind::object:
      return n.count();
    default:
      return 0;
  }
}

inline value value::at(size_t index) const { return as_array().at(index); }

inline value value::at(std::string_view key) const {
  return as_object().at(key);
}

inline value value::find(std::string_view key) const noexcept {
  if (!valid()) {
    return {};
  }
  if (type() != kind::object) {
    return {};
  }
  if (doc_->tape_mode_) {
    const auto count = doc_->tape_container_count(id_);
    return object_view(doc_, count == 0 ? detail::invalid_index : id_ + 1,
                       count, doc_->tape_container_end(id_))
        .find(key);
  }
  const auto& n = doc_->nodes_[id_];
  return object_view(doc_, n.first(), n.count()).find(key);
}

inline array_view::iterator& array_view::iterator::operator++() {
  if (current_ != detail::invalid_index) {
    if (doc_->tape_mode_) {
      const auto next = doc_->tape_next_value(current_);
      current_ = next >= end_ ? detail::invalid_index : next;
    }
    else {
      current_ = doc_->nodes_[current_].next;
    }
  }
  return *this;
}

inline value array_view::iterator::operator*() const {
  return value(doc_, current_);
}

inline value array_view::at(size_t index) const {
  if (!doc_) {
    throw std::logic_error("json array view is invalid");
  }
  if (index >= count_) {
    throw std::out_of_range("json array index is out of range");
  }

  auto current = first_;
  for (size_t i = 0; i < index; ++i) {
    current = doc_->tape_mode_ ? doc_->tape_next_value(current)
                               : doc_->nodes_[current].next;
  }
  return value(doc_, current);
}

inline object_item object_view::iterator::operator*() const {
  if (doc_->tape_mode_) {
    return {doc_->resolve_tape_string(current_), value(doc_, current_ + 1)};
  }
  const auto& m = doc_->members_[current_];
  return {doc_->resolve(m.key), value(doc_, m.value)};
}

inline object_view::iterator& object_view::iterator::operator++() {
  if (current_ != detail::invalid_index) {
    if (doc_->tape_mode_) {
      const auto next = doc_->tape_next_value(current_ + 1);
      current_ = next >= end_ ? detail::invalid_index : next;
    }
    else {
      current_ = doc_->members_[current_].next;
    }
  }
  return *this;
}

inline value object_view::find(std::string_view key) const noexcept {
  if (!doc_) {
    return {};
  }

  if (doc_->tape_mode_) {
    value result;
    auto current = first_;
    for (uint32_t i = 0; i < count_; ++i) {
      if (doc_->resolve_tape_string(current) == key) {
        result = value(doc_, current + 1);
      }
      current = doc_->tape_next_value(current + 1);
    }
    return result;
  }

  value result;
  for (auto current = first_; current != detail::invalid_index;
       current = doc_->members_[current].next) {
    const auto& m = doc_->members_[current];
    if (doc_->resolve(m.key) == key) {
      result = value(doc_, m.value);
    }
  }
  return result;
}

inline value object_view::at(std::string_view key) const {
  auto found = find(key);
  if (!found) {
    std::string message("json object does not contain key: ");
    message.append(key.data(), key.size());
    throw std::out_of_range(message);
  }
  return found;
}

namespace detail {

#ifdef IGUANA_JSON_DOM_HAS_AVX2
class simd_utf8_checker {
 public:
  static bool validate(std::string_view input) noexcept {
    simd_utf8_checker checker;
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
    size_t offset = 0;
    while (input.size() - offset >= 64) {
      checker.check_block(data + offset);
      offset += 64;
    }
    if (offset != input.size()) {
      alignas(32) unsigned char tail[64]{};
      std::memcpy(tail, data + offset, input.size() - offset);
      checker.check_block(tail);
    }
    return checker.valid();
  }

  static bool has_dense_non_ascii_sample(std::string_view input) noexcept {
    constexpr size_t minimum_size = 4096;
    constexpr size_t block_size = 64;
    constexpr unsigned minimum_non_ascii_bytes = 8;
    if (input.size() < minimum_size) {
      return false;
    }

    const auto last = input.size() - block_size;
    const size_t offsets[] = {0, last / 3, (last / 3) * 2, last};
    unsigned non_ascii_bytes = 0;
    for (const auto offset : offsets) {
      const auto* block =
          reinterpret_cast<const __m256i*>(input.data() + offset);
      const auto first = _mm256_loadu_si256(block);
      const auto second = _mm256_loadu_si256(block + 1);
      non_ascii_bytes += static_cast<unsigned>(
          std::popcount(static_cast<uint32_t>(_mm256_movemask_epi8(first))));
      non_ascii_bytes += static_cast<unsigned>(
          std::popcount(static_cast<uint32_t>(_mm256_movemask_epi8(second))));
      if (non_ascii_bytes >= minimum_non_ascii_bytes) {
        return true;
      }
    }
    return false;
  }

 private:
  static __m256i previous1(__m256i current, __m256i previous) noexcept {
    return _mm256_alignr_epi8(
        current, _mm256_permute2x128_si256(previous, current, 0x21), 15);
  }

  static __m256i previous2(__m256i current, __m256i previous) noexcept {
    return _mm256_alignr_epi8(
        current, _mm256_permute2x128_si256(previous, current, 0x21), 14);
  }

  static __m256i previous3(__m256i current, __m256i previous) noexcept {
    return _mm256_alignr_epi8(
        current, _mm256_permute2x128_si256(previous, current, 0x21), 13);
  }

  static __m256i high_nibble(__m256i value) noexcept {
    return _mm256_and_si256(_mm256_srli_epi16(value, 4),
                            _mm256_set1_epi8(0x0F));
  }

  template <class... Bytes>
  static __m128i make_table(Bytes... values) noexcept {
    static_assert(sizeof...(values) == 16);
    return _mm_setr_epi8(static_cast<char>(values)...);
  }

  static __m256i special_cases(__m256i current, __m256i prev1) noexcept {
    constexpr unsigned char too_short = 1u << 0;
    constexpr unsigned char too_long = 1u << 1;
    constexpr unsigned char overlong_3 = 1u << 2;
    constexpr unsigned char too_large = 1u << 3;
    constexpr unsigned char surrogate = 1u << 4;
    constexpr unsigned char overlong_2 = 1u << 5;
    constexpr unsigned char overlong_4 = 1u << 6;
    constexpr unsigned char too_large_1000 = 1u << 6;
    constexpr unsigned char two_conts = 1u << 7;
    constexpr unsigned char carry = too_short | too_long | two_conts;

    const auto byte_1_high_table = _mm256_broadcastsi128_si256(make_table(
        too_long, too_long, too_long, too_long, too_long, too_long, too_long,
        too_long, two_conts, two_conts, two_conts, two_conts,
        too_short | overlong_2, too_short, too_short | overlong_3 | surrogate,
        too_short | too_large | too_large_1000 | overlong_4));
    const auto byte_1_low_table = _mm256_broadcastsi128_si256(make_table(
        carry | overlong_3 | overlong_2 | overlong_4, carry | overlong_2, carry,
        carry, carry | too_large, carry | too_large | too_large_1000,
        carry | too_large | too_large_1000, carry | too_large | too_large_1000,
        carry | too_large | too_large_1000, carry | too_large | too_large_1000,
        carry | too_large | too_large_1000, carry | too_large | too_large_1000,
        carry | too_large | too_large_1000,
        carry | too_large | too_large_1000 | surrogate,
        carry | too_large | too_large_1000,
        carry | too_large | too_large_1000));
    const auto byte_2_high_table = _mm256_broadcastsi128_si256(
        make_table(too_short, too_short, too_short, too_short, too_short,
                   too_short, too_short, too_short,
                   too_long | overlong_2 | two_conts | overlong_3 |
                       too_large_1000 | overlong_4,
                   too_long | overlong_2 | two_conts | overlong_3 | too_large,
                   too_long | overlong_2 | two_conts | surrogate | too_large,
                   too_long | overlong_2 | two_conts | surrogate | too_large,
                   too_short, too_short, too_short, too_short));

    const auto byte_1_high =
        _mm256_shuffle_epi8(byte_1_high_table, high_nibble(prev1));
    const auto byte_1_low = _mm256_shuffle_epi8(
        byte_1_low_table, _mm256_and_si256(prev1, _mm256_set1_epi8(0x0F)));
    const auto byte_2_high =
        _mm256_shuffle_epi8(byte_2_high_table, high_nibble(current));
    return _mm256_and_si256(_mm256_and_si256(byte_1_high, byte_1_low),
                            byte_2_high);
  }

  static __m256i incomplete_at_end(__m256i input) noexcept {
    const auto maximum = _mm256_setr_epi8(
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, static_cast<char>(0xEF),
        static_cast<char>(0xDF), static_cast<char>(0xBF));
    return _mm256_subs_epu8(input, maximum);
  }

  void check_chunk(__m256i current, __m256i previous_chunk) noexcept {
    const auto prev1 = previous1(current, previous_chunk);
    const auto prev2 = previous2(current, previous_chunk);
    const auto prev3 = previous3(current, previous_chunk);
    const auto must23 =
        _mm256_or_si256(_mm256_subs_epu8(prev2, _mm256_set1_epi8(0x60)),
                        _mm256_subs_epu8(prev3, _mm256_set1_epi8(0x70)));
    const auto length_error = _mm256_xor_si256(
        _mm256_and_si256(must23, _mm256_set1_epi8(static_cast<char>(0x80))),
        special_cases(current, prev1));
    error_ = _mm256_or_si256(error_, length_error);
  }

  void check_block(const unsigned char* data) noexcept {
    const auto first =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));
    const auto second =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + 32));
    if (_mm256_movemask_epi8(_mm256_or_si256(first, second)) == 0) {
      error_ = _mm256_or_si256(error_, incomplete_);
      previous_ = _mm256_setzero_si256();
      incomplete_ = _mm256_setzero_si256();
      return;
    }
    check_chunk(first, previous_);
    check_chunk(second, first);
    incomplete_ = incomplete_at_end(second);
    previous_ = second;
  }

  bool valid() noexcept {
    error_ = _mm256_or_si256(error_, incomplete_);
    return _mm256_testz_si256(error_, error_) != 0;
  }

  __m256i error_ = _mm256_setzero_si256();
  __m256i previous_ = _mm256_setzero_si256();
  __m256i incomplete_ = _mm256_setzero_si256();
};
#endif

template <class Consumer>
class structural_scanner {
 public:
  structural_scanner(document& doc, Consumer& consumer)
      : input_(doc.source()), consumer_(consumer) {}

  structural_scanner(std::string_view input, Consumer& consumer)
      : input_(input), consumer_(consumer) {}

  void run() {
    const auto* const data = input_.data();
    size_t offset = 0;

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto quote = _mm256_set1_epi8('"');
    const auto backslash = _mm256_set1_epi8('\\');
    const auto whitespace_table = _mm256_broadcastsi128_si256(
        _mm_setr_epi8(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112,
                      100, '\r', 100, 100));
    const auto structural_table = _mm256_broadcastsi128_si256(
        _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{', ',', '}', 0, 0));
    const auto curlify = _mm256_set1_epi8(0x20);
    const auto high_bit = _mm256_set1_epi8(static_cast<char>(0x80));
    const auto control_limit = _mm256_set1_epi8(static_cast<char>(0x20 ^ 0x80));

    struct raw_masks {
      uint64_t quotes;
      uint64_t backslashes;
      uint64_t structurals;
      uint64_t whitespace;
      uint64_t controls;
    };
    auto classify_block = [&](size_t block_offset) noexcept {
      const auto chunk0 = _mm256_loadu_si256(
          reinterpret_cast<const __m256i*>(data + block_offset));
      const auto chunk1 = _mm256_loadu_si256(
          reinterpret_cast<const __m256i*>(data + block_offset + 32));
      return raw_masks{
          combine_masks(_mm256_cmpeq_epi8(chunk0, quote),
                        _mm256_cmpeq_epi8(chunk1, quote)),
          combine_masks(_mm256_cmpeq_epi8(chunk0, backslash),
                        _mm256_cmpeq_epi8(chunk1, backslash)),
          combine_masks(
              classify_structurals(chunk0, structural_table, curlify),
              classify_structurals(chunk1, structural_table, curlify)),
          combine_masks(classify_whitespace(chunk0, whitespace_table),
                        classify_whitespace(chunk1, whitespace_table)),
          combine_masks(classify_controls(chunk0, high_bit, control_limit),
                        classify_controls(chunk1, high_bit, control_limit))};
    };
    auto process_block = [&](size_t block_offset, const raw_masks& masks) {
      process_masks(block_offset, masks.quotes, masks.backslashes,
                    masks.structurals, masks.whitespace, masks.controls);
    };

    while (input_.size() - offset >= 128) {
      const auto first = classify_block(offset);
      const auto second = classify_block(offset + 64);
      process_block(offset, first);
      process_block(offset + 64, second);
      offset += 128;
    }
    if (input_.size() - offset >= 64) {
      const auto masks = classify_block(offset);
      process_block(offset, masks);
      offset += 64;
    }
#endif

    process_tail(offset);
    if (in_string_) {
      throw parse_error(input_.size(), "unterminated string");
    }
  }

 private:
  static bool is_ws(unsigned char c) noexcept {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
  }

  static bool is_structural(unsigned char c) noexcept {
    return c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':';
  }

#ifdef IGUANA_JSON_DOM_HAS_AVX2
  static uint64_t combine_masks(__m256i low, __m256i high) noexcept {
    return static_cast<uint32_t>(_mm256_movemask_epi8(low)) |
           (static_cast<uint64_t>(
                static_cast<uint32_t>(_mm256_movemask_epi8(high)))
            << 32u);
  }

  static __m256i classify_whitespace(__m256i chunk, __m256i table) noexcept {
    return _mm256_cmpeq_epi8(chunk, _mm256_shuffle_epi8(table, chunk));
  }

  static __m256i classify_structurals(__m256i chunk, __m256i table,
                                      __m256i curlify) noexcept {
    const auto expected = _mm256_shuffle_epi8(table, chunk);
    return _mm256_cmpeq_epi8(_mm256_or_si256(chunk, curlify), expected);
  }

  static __m256i classify_controls(__m256i chunk, __m256i high_bit,
                                   __m256i limit) noexcept {
    return _mm256_cmpgt_epi8(limit, _mm256_xor_si256(chunk, high_bit));
  }

#endif

  void add_index(size_t offset) {
    consumer_.consume_structural(static_cast<uint32_t>(offset));
  }

  void add_index_unchecked(size_t offset) {
    consumer_.consume_structural(static_cast<uint32_t>(offset));
  }

  uint64_t escaped_characters(uint64_t backslashes) noexcept {
    if (backslashes == 0) {
      const auto escaped = next_is_escaped_;
      next_is_escaped_ = 0;
      return escaped;
    }

    constexpr uint64_t odd_bits = 0xAAAAAAAAAAAAAAAAull;
    const auto potential_escape = backslashes & ~next_is_escaped_;
    const auto maybe_escaped = potential_escape << 1u;
    const auto escape_and_terminal =
        ((maybe_escaped | odd_bits) - potential_escape) ^ odd_bits;
    const auto escaped = escape_and_terminal ^ (backslashes | next_is_escaped_);
    const auto escape = escape_and_terminal & backslashes;
    next_is_escaped_ = escape >> 63u;
    return escaped;
  }

  static uint64_t prefix_xor(uint64_t bits) noexcept {
    bits ^= bits << 1u;
    bits ^= bits << 2u;
    bits ^= bits << 4u;
    bits ^= bits << 8u;
    bits ^= bits << 16u;
    bits ^= bits << 32u;
    return bits;
  }

  void process_masks(size_t block_offset, uint64_t quotes, uint64_t backslashes,
                     uint64_t structurals, uint64_t whitespace,
                     uint64_t controls) {
    const auto escaped = escaped_characters(backslashes);
    const auto unescaped_quotes = quotes & ~escaped;
    const auto string_mask = prefix_xor(unescaped_quotes) ^ prev_in_string_;
    prev_in_string_ =
        static_cast<uint64_t>(static_cast<int64_t>(string_mask) >> 63u);
    in_string_ = prev_in_string_ != 0;

    const auto invalid_controls =
        (controls & string_mask) | (controls & ~string_mask & ~whitespace);
    if (invalid_controls != 0) {
      const auto bit = static_cast<size_t>(std::countr_zero(invalid_controls));
      throw parse_error(block_offset + bit,
                        (string_mask & (uint64_t{1} << bit)) != 0
                            ? "control character in string"
                            : "invalid whitespace character");
    }

    const auto scalar = ~(structurals | whitespace);
    const auto nonquote_scalar = scalar & ~unescaped_quotes;
    const auto follows_scalar = (nonquote_scalar << 1u) | previous_scalar_;
    previous_scalar_ = nonquote_scalar >> 63u;
    const auto potential_tokens = structurals | (scalar & ~follows_scalar);
    const auto tokens = potential_tokens & ~(string_mask ^ unescaped_quotes);
    consumer_.consume_structurals(block_offset, tokens);
  }

  void process_tail(size_t offset) {
    bool next_is_escaped = next_is_escaped_ != 0;
    while (offset < input_.size()) {
      const auto c = static_cast<unsigned char>(input_[offset]);
      const bool escaped = next_is_escaped;
      next_is_escaped = c == '\\' && !escaped;
      if (in_string_) {
        if (c < 0x20u) {
          throw parse_error(offset, "control character in string");
        }
        if (c == '"' && !escaped) {
          in_string_ = false;
          prev_in_string_ = 0;
          previous_scalar_ = 0;
        }
      }
      else if (c == '"' && !escaped) {
        add_index(offset);
        in_string_ = true;
        prev_in_string_ = ~uint64_t{0};
        previous_scalar_ = 0;
      }
      else if (is_structural(c)) {
        add_index(offset);
        previous_scalar_ = 0;
      }
      else if (c <= 0x20u) {
        if (!is_ws(c)) {
          throw parse_error(offset, "invalid whitespace character");
        }
        previous_scalar_ = 0;
      }
      else if (!previous_scalar_) {
        add_index(offset);
        previous_scalar_ = 1;
      }
      ++offset;
    }
    next_is_escaped_ = next_is_escaped ? 1u : 0u;
  }

  std::string_view input_;
  Consumer& consumer_;
  bool in_string_ = false;
  uint64_t previous_scalar_ = 0;
  uint64_t prev_in_string_ = 0;
  uint64_t next_is_escaped_ = 0;
};

class structural_index_consumer {
 public:
  explicit structural_index_consumer(dom_vector<uint32_t>& indexes)
      : indexes_(indexes) {}

  void consume_structural(uint32_t offset) { indexes_.push_back(offset); }

  void consume_structurals(size_t block_offset, uint64_t tokens) {
    auto count = static_cast<size_t>(std::popcount(tokens));
    indexes_.ensure_additional(count);
    const auto base = static_cast<uint32_t>(block_offset);
    auto write_one = [&]() noexcept {
      const auto bit = static_cast<uint32_t>(std::countr_zero(tokens));
      indexes_.emplace_back_unchecked() = base + bit;
      tokens &= tokens - 1;
    };
    while (count >= 8) {
      write_one();
      write_one();
      write_one();
      write_one();
      write_one();
      write_one();
      write_one();
      write_one();
      count -= 8;
    }
    while (count >= 4) {
      write_one();
      write_one();
      write_one();
      write_one();
      count -= 4;
    }
    while (tokens != 0) {
      write_one();
    }
  }

 private:
  dom_vector<uint32_t>& indexes_;
};

template <number_mode Numbers, bool ValidateUtf8, bool ValidateNumbers,
          bool RejectDuplicateKeys>
class dom_parser {
 public:
  explicit dom_parser(document& doc) : doc_(doc), input_(doc.source()) {}

  void run() {
    scan_structurals_and_validate_utf8();
    run_indexed();
  }

  bool scan_structurals_and_validate_utf8() {
    doc_.structural_indexes_.clear();
    structural_index_consumer consumer(doc_.structural_indexes_);
    structural_scanner<structural_index_consumer> scanner(doc_, consumer);
    scanner.run();
#ifdef IGUANA_JSON_DOM_HAS_AVX2
    if constexpr (ValidateUtf8) {
      return simd_utf8_checker::has_dense_non_ascii_sample(input_) &&
             simd_utf8_checker::validate(input_);
    }
#endif
    return false;
  }

  void run_preindexed() { run_indexed(); }

  void consume_structural(uint32_t offset) {
    validate_pending_scalar(offset);
    pos_ = offset;
    const auto token = input_[pos_];

    switch (stream_state_) {
      case stream_state::value:
      case stream_state::array_value_or_end: {
        if (stream_state_ == stream_state::array_value_or_end && token == ']') {
          close_stream_container(kind::array);
          return;
        }
        if (token == 'n') {
          const auto id = parse_null();
          begin_pending_scalar();
          accept_value(id);
          complete_stream_value();
          return;
        }
        if (token == 't' || token == 'f') {
          const auto id = parse_bool();
          begin_pending_scalar();
          accept_value(id);
          complete_stream_value();
          return;
        }
        if (token == '"') {
          const auto id = parse_string_node();
          accept_value(id);
          complete_stream_value();
          return;
        }
        if ((token >= '0' && token <= '9') || token == '-') {
          const auto id = parse_number();
          begin_pending_scalar();
          accept_value(id);
          complete_stream_value();
          return;
        }
        if (token == '[') {
          const auto id = add_typed_node(kind::array);
          accept_value(id);
          push_frame(id);
          stream_state_ = stream_state::array_value_or_end;
          return;
        }
        if (token == '{') {
          const auto id = add_typed_node(kind::object);
          accept_value(id);
          push_frame(id);
          stream_state_ = stream_state::object_key_or_end;
          return;
        }
        fail("expected value");
      }

      case stream_state::object_key_or_end:
        if (token == '}') {
          close_stream_container(kind::object);
          return;
        }
        [[fallthrough]];
      case stream_state::object_key: {
        if (token != '"') {
          fail("expected object key");
        }
        const auto key = parse_string_ref();
        if constexpr (RejectDuplicateKeys) {
          const auto key_view = doc_.resolve(key);
          const auto container = current_container_id();
          auto current = container + 1;
          const auto count = current_container_count();
          for (uint32_t i = 0; i < count; ++i) {
            if (doc_.resolve_tape_string(current) == key_view) {
              std::string message("duplicated object key: ");
              message.append(key_view.data(), key_view.size());
              fail(message);
            }
            current = doc_.tape_next_value(current + 1);
          }
        }
        append_key(key);
        stream_state_ = stream_state::colon;
        return;
      }

      case stream_state::colon:
        if (token != ':') {
          fail("expected ':'");
        }
        stream_state_ = stream_state::value;
        return;

      case stream_state::array_separator:
        if (token == ']') {
          close_stream_container(kind::array);
          return;
        }
        if (token != ',') {
          fail("expected ',' or ']'");
        }
        stream_state_ = stream_state::value;
        return;

      case stream_state::object_separator:
        if (token == '}') {
          close_stream_container(kind::object);
          return;
        }
        if (token != ',') {
          fail("expected ',' or '}'");
        }
        stream_state_ = stream_state::object_key;
        return;

      case stream_state::done:
        fail("unexpected trailing data");
    }
  }

 private:
  enum class stream_state : uint8_t {
    value,
    array_value_or_end,
    object_key_or_end,
    object_key,
    colon,
    array_separator,
    object_separator,
    done,
  };

  void run_streaming() {
    doc_.tape_parse_stack_.clear();
    stream_state_ = stream_state::value;
    pending_scalar_ = false;
    structural_scanner<dom_parser> scanner(doc_, *this);
    scanner.run();
    validate_pending_scalar(input_.size());
    if (stream_state_ != stream_state::done) {
      pos_ = input_.size();
      if (doc_.root_ == invalid_index) {
        fail("empty input");
      }
      fail("unexpected end of input");
    }
  }

  void begin_pending_scalar() noexcept {
    pending_scalar_ = true;
    pending_scalar_end_ = pos_;
  }

  void validate_pending_scalar(size_t next_token) {
    if (!pending_scalar_) {
      return;
    }
    auto end = pending_scalar_end_;
    while (end < next_token && is_ws(input_[end])) {
      ++end;
    }
    if (end != next_token) {
      fail_at(end, "invalid token");
    }
    pending_scalar_ = false;
  }

  void complete_stream_value() noexcept {
    if (doc_.tape_parse_stack_.empty()) {
      stream_state_ = stream_state::done;
    }
    else if (current_container_is_array()) {
      stream_state_ = stream_state::array_separator;
    }
    else {
      stream_state_ = stream_state::object_separator;
    }
  }

  void close_stream_container(kind expected) {
    if (doc_.tape_parse_stack_.empty() ||
        current_container_is_array() != (expected == kind::array)) {
      fail(expected == kind::array ? "unexpected ']'" : "unexpected '}'");
    }
    close_frame();
    complete_stream_value();
  }

  void run_recursive() {
    skip_ws();
    if (pos_ == input_.size()) {
      fail("empty input");
    }
    doc_.root_ = parse_value_after_ws();
    skip_ws();
    if (pos_ != input_.size()) {
      fail("unexpected trailing data");
    }
  }

  void run_indexed() {
    doc_.tape_parse_stack_.clear();
    if (doc_.structural_indexes_.empty()) {
      pos_ = 0;
      fail("empty input");
    }
    next_structural_ = doc_.structural_indexes_.data();
    structural_end_ = next_structural_ + doc_.structural_indexes_.size();

    goto value_begin;

  array_value_begin:
    select_current_token("expected value");
    if (input_[pos_] == '"') [[likely]] {
      const auto id = parse_string_node();
      advance_token();
      append_to_current(id);
      goto array_value_done;
    }
    if (input_[pos_] == '-' || is_digit(input_[pos_])) {
      const auto id = parse_number();
      finish_scalar_token();
      append_to_current(id);
      goto array_value_done;
    }
    goto value_selected;

  object_value_begin:
    select_current_token("expected value");
    if (input_[pos_] == '"') [[likely]] {
      const auto id = parse_string_node();
      advance_token();
      append_to_current(id);
      goto object_value_done;
    }
    if (input_[pos_] == '-' || is_digit(input_[pos_])) {
      const auto id = parse_number();
      finish_scalar_token();
      append_to_current(id);
      goto object_value_done;
    }
    goto value_selected;

  value_begin:
    select_current_token("expected value");
  value_selected:
    switch (input_[pos_]) {
      case '"': {
        const auto id = parse_string_node();
        advance_token();
        accept_value(id);
        goto value_done;
      }
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        const auto id = parse_number();
        finish_scalar_token();
        accept_value(id);
        goto value_done;
      }
      case '{': {
        const auto id = add_typed_node(kind::object);
        accept_value(id);
        push_frame(id);
        advance_token();
        if (current_token_is('}')) {
          advance_token();
          close_frame();
          goto value_done;
        }
        goto object_key_begin;
      }
      case '[': {
        const auto id = add_typed_node(kind::array);
        accept_value(id);
        push_frame(id);
        advance_token();
        if (current_token_is(']')) {
          advance_token();
          close_frame();
          goto value_done;
        }
        goto array_value_begin;
      }
      case 'n': {
        const auto id = parse_null();
        finish_scalar_token();
        accept_value(id);
        goto value_done;
      }
      case 't':
      case 'f': {
        const auto id = parse_bool();
        finish_scalar_token();
        accept_value(id);
        goto value_done;
      }
      default:
        fail("expected value");
    }

  object_key_begin: {
    select_current_token("expected object key");
    if (input_[pos_] != '"') {
      fail("expected object key");
    }
    const auto key = parse_string_ref();
    if constexpr (RejectDuplicateKeys) {
      const auto key_view = doc_.resolve(key);
      const auto container = current_container_id();
      auto current = container + 1;
      const auto count = current_container_count();
      for (uint32_t i = 0; i < count; ++i) {
        if (doc_.resolve_tape_string(current) == key_view) {
          std::string message("duplicated object key: ");
          message.append(key_view.data(), key_view.size());
          fail(message);
        }
        current = doc_.tape_next_value(current + 1);
      }
    }
    append_key(key);
    advance_token();
    expect_current_token(':');
    advance_token();
    goto object_value_begin;
  }

  value_done:
    if (doc_.tape_parse_stack_.empty()) {
      if (next_structural_ != structural_end_) {
        pos_ = *next_structural_;
        fail("unexpected trailing data");
      }
      return;
    }
    if (current_container_is_array()) {
      goto array_value_done;
    }
    goto object_value_done;

  array_value_done:
    select_current_token("expected ',' or ']'");
    if (input_[pos_] == ']') {
      advance_token();
      close_frame();
      goto value_done;
    }
    if (input_[pos_] != ',') {
      fail("expected ',' or ']'");
    }
    advance_token();
    if (current_token_is(']')) {
      pos_ = *next_structural_;
      fail("trailing comma in array");
    }
    goto array_value_begin;

  object_value_done:
    select_current_token("expected ',' or '}'");
    if (input_[pos_] == '}') {
      advance_token();
      close_frame();
      goto value_done;
    }
    if (input_[pos_] != ',') {
      fail("expected ',' or '}'");
    }
    advance_token();
    if (current_token_is('}')) {
      pos_ = *next_structural_;
      fail("trailing comma in object");
    }
    goto object_key_begin;
  }

  void select_current_token(std::string_view error) {
    if (next_structural_ == structural_end_) {
      pos_ = input_.size();
      fail(error);
    }
    pos_ = *next_structural_;
  }

  bool current_token_is(char expected) const noexcept {
    return next_structural_ != structural_end_ &&
           input_[*next_structural_] == expected;
  }

  void expect_current_token(char expected) {
    if (!current_token_is(expected)) {
      if (next_structural_ != structural_end_) {
        pos_ = *next_structural_;
      }
      else {
        pos_ = input_.size();
      }
      std::string message("expected '");
      message += expected;
      message += "'";
      fail(message);
    }
  }

  void advance_token() noexcept { ++next_structural_; }

  void finish_scalar_token() {
    const auto next = next_structural_ + 1 != structural_end_
                          ? static_cast<size_t>(next_structural_[1])
                          : input_.size();
    auto end = pos_;
    while (end < next && is_ws(input_[end])) {
      ++end;
    }
    if (end != next) {
      fail_at(end, "invalid token");
    }
    advance_token();
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void push_frame(uint32_t id) {
    doc_.tape_parse_stack_.push_back(id);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void append_to_current(uint32_t) noexcept {
    doc_.tape_[current_container_id()] += uint64_t{1} << 32u;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void accept_value(uint32_t value_id) {
    if (doc_.tape_parse_stack_.empty()) {
      doc_.root_ = value_id;
    }
    else {
      append_to_current(value_id);
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void close_frame() noexcept {
    const auto id = current_container_id();
    doc_.tape_parse_stack_.pop_back();
    doc_.tape_[id] |= static_cast<uint32_t>(doc_.tape_.size());
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE bool current_container_is_array()
      const noexcept {
    return doc_.tape_tag_at(current_container_id()) == detail::tape_tag::array;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t
  current_container_count() const noexcept {
    return doc_.tape_container_count(current_container_id());
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t current_container_id() const noexcept {
    return doc_.tape_parse_stack_.back();
  }

  [[noreturn]] IGUANA_JSON_DOM_NOINLINE void fail(
      std::string_view message) const {
    throw parse_error(pos_, message);
  }

  [[noreturn]] IGUANA_JSON_DOM_NOINLINE void fail_at(
      size_t offset, std::string_view message) const {
    throw parse_error(offset, message);
  }

  static bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

  static bool is_hex(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
  }

  static uint32_t hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
      return static_cast<uint32_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
      return static_cast<uint32_t>(c - 'a' + 10);
    }
    return static_cast<uint32_t>(c - 'A' + 10);
  }

  static bool is_ws(char c) noexcept {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
  }

  static bool is_number_token_char(char c) noexcept {
    return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' ||
           c == 'e' || c == 'E';
  }

  static bool is_continuation(unsigned char c) noexcept {
    return (c & 0xC0u) == 0x80u;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void skip_ws() noexcept {
    if (pos_ == input_.size() || !is_ws(input_[pos_])) {
      return;
    }

    const auto* const base = input_.data();
    const auto* p = base + pos_;
    const auto* const end = base + input_.size();

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto space256 = _mm256_set1_epi8(' ');
    const auto line_feed256 = _mm256_set1_epi8('\n');
    const auto carriage_return256 = _mm256_set1_epi8('\r');
    const auto tab256 = _mm256_set1_epi8('\t');
    while (static_cast<size_t>(end - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      const auto mask =
          static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(
              _mm256_or_si256(_mm256_cmpeq_epi8(chunk, space256),
                              _mm256_cmpeq_epi8(chunk, line_feed256)),
              _mm256_or_si256(_mm256_cmpeq_epi8(chunk, carriage_return256),
                              _mm256_cmpeq_epi8(chunk, tab256)))));
      if (mask == 0xFFFFFFFFu) {
        p += 32;
        continue;
      }
      pos_ = static_cast<size_t>((p + first_set_bit(~mask)) - base);
      return;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto space = _mm_set1_epi8(' ');
    const auto line_feed = _mm_set1_epi8('\n');
    const auto carriage_return = _mm_set1_epi8('\r');
    const auto tab = _mm_set1_epi8('\t');
    while (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto mask = static_cast<uint32_t>(_mm_movemask_epi8(
          _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, space),
                                    _mm_cmpeq_epi8(chunk, line_feed)),
                       _mm_or_si128(_mm_cmpeq_epi8(chunk, carriage_return),
                                    _mm_cmpeq_epi8(chunk, tab)))));
      if (mask == 0xFFFFu) {
        p += 16;
        continue;
      }
      const auto non_ws = (~mask) & 0xFFFFu;
      pos_ = static_cast<size_t>((p + first_set_bit(non_ws)) - base);
      return;
    }
#endif

    while (p != end) {
      if (!is_ws(*p)) {
        pos_ = static_cast<size_t>(p - base);
        return;
      }
      ++p;
    }
    pos_ = input_.size();
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void expect(char expected) {
    if (pos_ == input_.size() || input_[pos_] != expected) {
      std::string message("expected '");
      message += expected;
      message += "'";
      fail(message);
    }
    ++pos_;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE bool consume(char c) noexcept {
    if (pos_ < input_.size() && input_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }

  void match_literal(std::string_view literal) {
    if (input_.size() - pos_ < literal.size() ||
        input_.compare(pos_, literal.size(), literal) != 0) {
      fail("invalid literal");
    }
    pos_ += literal.size();
  }

  void match_null() {
    if (input_.size() - pos_ < 4 || input_[pos_] != 'n' ||
        input_[pos_ + 1] != 'u' || input_[pos_ + 2] != 'l' ||
        input_[pos_ + 3] != 'l') {
      fail("invalid literal");
    }
    pos_ += 4;
  }

  void match_true() {
    if (input_.size() - pos_ < 4 || input_[pos_] != 't' ||
        input_[pos_ + 1] != 'r' || input_[pos_ + 2] != 'u' ||
        input_[pos_ + 3] != 'e') {
      fail("invalid literal");
    }
    pos_ += 4;
  }

  void match_false() {
    if (input_.size() - pos_ < 5 || input_[pos_] != 'f' ||
        input_[pos_ + 1] != 'a' || input_[pos_ + 2] != 'l' ||
        input_[pos_ + 3] != 's' || input_[pos_ + 4] != 'e') {
      fail("invalid literal");
    }
    pos_ += 5;
  }

  uint32_t checked_u32(size_t value, std::string_view what) const {
    return document::checked_index(value, what);
  }

  uint32_t checked_string_length(size_t value) const {
    if (value > string_ref::length_mask) {
      throw std::length_error("json string length exceeds packed DOM range");
    }
    return static_cast<uint32_t>(value);
  }

  static uint32_t narrow_u32(size_t value) noexcept {
    return static_cast<uint32_t>(value);
  }

  uint32_t add_typed_node(kind type) {
    const auto id = narrow_u32(doc_.tape_.size());
    const auto tag = type == kind::array ? detail::tape_tag::array
                                         : detail::tape_tag::object;
    doc_.tape_.emplace_back_unchecked(detail::make_tape(tag));
    ++doc_.node_count_;
    return id;
  }

  uint32_t add_raw_number_node(size_t offset, size_t length) {
    return append_string_entry(
        detail::tape_tag::raw_number,
        string_ref::input(narrow_u32(offset), narrow_u32(length)), true);
  }

  uint32_t append_string_entry(detail::tape_tag tag, string_ref ref,
                               bool value_entry) {
    if (ref.length() > detail::tape_string_length_mask) {
      throw std::length_error("json string exceeds tape length range");
    }
    const auto id = narrow_u32(doc_.tape_.size());
    doc_.tape_.emplace_back_unchecked(
        detail::make_tape(tag, detail::pack_tape_string(ref)));
    if (value_entry) {
      ++doc_.node_count_;
    }
    return id;
  }

  void append_key(string_ref ref) {
    append_string_entry(detail::tape_tag::key, ref, false);
  }

  uint32_t append_number_entry(detail::tape_tag tag,
                               uint64_t payload) noexcept {
    const auto id = narrow_u32(doc_.tape_.size());
    doc_.tape_.emplace_back_unchecked(detail::make_tape(tag));
    doc_.tape_.emplace_back_unchecked(payload);
    ++doc_.node_count_;
    return id;
  }

  uint32_t append_int64_entry(int64_t value) noexcept {
    constexpr int64_t inline_min = -(int64_t{1} << 59u);
    constexpr int64_t inline_max = (int64_t{1} << 59u) - 1;
    if (value >= inline_min && value <= inline_max) {
      const auto id = narrow_u32(doc_.tape_.size());
      doc_.tape_.emplace_back_unchecked(detail::make_tape(
          detail::tape_tag::int64,
          static_cast<uint64_t>(value) & detail::tape_payload_mask));
      ++doc_.node_count_;
      return id;
    }
    return append_number_entry(detail::tape_tag::int64_full,
                               static_cast<uint64_t>(value));
  }

  uint32_t append_uint64_entry(uint64_t value) noexcept {
    if (value <= detail::tape_payload_mask) {
      const auto id = narrow_u32(doc_.tape_.size());
      doc_.tape_.emplace_back_unchecked(
          detail::make_tape(detail::tape_tag::uint64, value));
      ++doc_.node_count_;
      return id;
    }
    return append_number_entry(detail::tape_tag::uint64_full, value);
  }

  uint32_t parse_value() {
    skip_ws();
    return parse_value_after_ws();
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_value_after_ws() {
    if (pos_ == input_.size()) {
      fail("expected value");
    }

    switch (input_[pos_]) {
      case 'n':
        return parse_null();
      case 't':
      case 'f':
        return parse_bool();
      case '"':
        return parse_string_node();
      case '[':
        return parse_array();
      case '{':
        return parse_object();
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return parse_number();
      default:
        fail("expected value");
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_null() {
    match_null();
    const auto id = narrow_u32(doc_.tape_.size());
    doc_.tape_.emplace_back_unchecked(
        detail::make_tape(detail::tape_tag::null));
    ++doc_.node_count_;
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_bool() {
    const auto id = narrow_u32(doc_.tape_.size());
    uint64_t payload = 0;
    if (input_[pos_] == 't') {
      match_true();
      payload = 1;
    }
    else {
      match_false();
    }
    doc_.tape_.emplace_back_unchecked(
        detail::make_tape(detail::tape_tag::boolean, payload));
    ++doc_.node_count_;
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_string_node() {
    auto ref = parse_string_ref();
    return append_string_entry(detail::tape_tag::string, ref, true);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_number() {
    const size_t start = pos_;
    const auto* const base = input_.data();
    const auto* p = base + pos_;
    const auto* const end = base + input_.size();

    if constexpr (Numbers == number_mode::lazy && !ValidateNumbers) {
      p = scan_number_token(p, end);
      pos_ = static_cast<size_t>(p - base);

      return add_raw_number_node(start, pos_ - start);
    }

    if (p == end) {
      fail_at(start, "invalid number");
    }

    if (*p == '-') {
      ++p;
      if (p == end) {
        fail_at(static_cast<size_t>(p - base), "invalid number");
      }
    }

    if (*p == '0') {
      ++p;
      if (p != end && is_digit(*p)) {
        fail_at(static_cast<size_t>(p - base),
                "invalid number with leading zero");
      }
    }
    else if (*p >= '1' && *p <= '9') {
      ++p;
      p = scan_digits(p, end);
    }
    else {
      fail_at(static_cast<size_t>(p - base), "invalid number");
    }

    bool floating = false;
    if (p != end && *p == '.') {
      floating = true;
      ++p;
      if (p == end || !is_digit(*p)) {
        fail_at(static_cast<size_t>(p - base),
                "expected digit after decimal point");
      }
      ++p;
      p = scan_digits(p, end);
    }

    if (p != end && (*p == 'e' || *p == 'E')) {
      floating = true;
      ++p;
      if (p != end && (*p == '+' || *p == '-')) {
        ++p;
      }
      if (p == end || !is_digit(*p)) {
        fail_at(static_cast<size_t>(p - base), "expected digit in exponent");
      }
      ++p;
      p = scan_digits(p, end);
    }

    pos_ = static_cast<size_t>(p - base);
    const auto token = std::string_view(base + start, p - (base + start));
    if constexpr (Numbers == number_mode::lazy) {
      return add_raw_number_node(start, token.size());
    }

    if (!floating && token.front() == '-') {
      int64_t value = 0;
      const auto [ptr, ec] =
          std::from_chars(token.data(), token.data() + token.size(), value);
      if (ec == std::errc{} && ptr == token.data() + token.size()) {
        return append_int64_entry(value);
      }
    }
    else if (!floating) {
      uint64_t value = 0;
      const auto [ptr, ec] =
          std::from_chars(token.data(), token.data() + token.size(), value);
      if (ec == std::errc{} && ptr == token.data() + token.size()) {
        return append_uint64_entry(value);
      }
    }

    double value = 0.0;
    const auto [ptr, ec] = iguana::detail::from_chars<false>(
        token.data(), token.data() + token.size(), value);
    if (ec != std::errc{} || ptr != token.data() + token.size()) {
      fail_at(start, "number is outside supported range");
    }
    return append_number_entry(detail::tape_tag::number,
                               std::bit_cast<uint64_t>(value));
  }

  const char* scan_digits(const char* p, const char* end) const noexcept {
#ifdef IGUANA_JSON_DOM_HAS_SSE2
    if (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto ge_zero = _mm_cmpgt_epi8(chunk, _mm_set1_epi8('/'));
      const auto le_nine = _mm_cmpgt_epi8(_mm_set1_epi8(':'), chunk);
      const auto digit_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_and_si128(ge_zero, le_nine)));
      if (digit_mask != 0xFFFFu) {
        return p + first_set_bit((~digit_mask) & 0xFFFFu);
      }
      p += 16;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto slash256 = _mm256_set1_epi8('/');
    const auto colon256 = _mm256_set1_epi8(':');
    while (static_cast<size_t>(end - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      const auto ge_zero = _mm256_cmpgt_epi8(chunk, slash256);
      const auto le_nine = _mm256_cmpgt_epi8(colon256, chunk);
      const auto digit_mask = static_cast<uint32_t>(
          _mm256_movemask_epi8(_mm256_and_si256(ge_zero, le_nine)));
      if (digit_mask == 0xFFFFFFFFu) {
        p += 32;
        continue;
      }
      return p + first_set_bit(~digit_mask);
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto slash = _mm_set1_epi8('/');
    const auto colon = _mm_set1_epi8(':');
    while (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto ge_zero = _mm_cmpgt_epi8(chunk, slash);
      const auto le_nine = _mm_cmpgt_epi8(colon, chunk);
      const auto digit_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_and_si128(ge_zero, le_nine)));
      if (digit_mask == 0xFFFFu) {
        p += 16;
        continue;
      }
      return p + first_set_bit((~digit_mask) & 0xFFFFu);
    }
#endif

    while (p != end && is_digit(*p)) {
      ++p;
    }
    return p;
  }

  const char* scan_number_token(const char* p, const char* end) const noexcept {
#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto slash256 = _mm256_set1_epi8('/');
    const auto colon256 = _mm256_set1_epi8(':');
    const auto minus256 = _mm256_set1_epi8('-');
    const auto plus256 = _mm256_set1_epi8('+');
    const auto dot256 = _mm256_set1_epi8('.');
    const auto lower_e256 = _mm256_set1_epi8('e');
    const auto upper_e256 = _mm256_set1_epi8('E');
    while (static_cast<size_t>(end - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      const auto ge_zero = _mm256_cmpgt_epi8(chunk, slash256);
      const auto le_nine = _mm256_cmpgt_epi8(colon256, chunk);
      auto accepted = _mm256_and_si256(ge_zero, le_nine);
      accepted = _mm256_or_si256(accepted, _mm256_cmpeq_epi8(chunk, minus256));
      accepted = _mm256_or_si256(accepted, _mm256_cmpeq_epi8(chunk, plus256));
      accepted = _mm256_or_si256(accepted, _mm256_cmpeq_epi8(chunk, dot256));
      accepted =
          _mm256_or_si256(accepted, _mm256_cmpeq_epi8(chunk, lower_e256));
      accepted =
          _mm256_or_si256(accepted, _mm256_cmpeq_epi8(chunk, upper_e256));
      const auto mask = static_cast<uint32_t>(_mm256_movemask_epi8(accepted));
      if (mask == 0xFFFFFFFFu) {
        p += 32;
        continue;
      }
      return p + first_set_bit(~mask);
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto slash = _mm_set1_epi8('/');
    const auto colon = _mm_set1_epi8(':');
    const auto minus = _mm_set1_epi8('-');
    const auto plus = _mm_set1_epi8('+');
    const auto dot = _mm_set1_epi8('.');
    const auto lower_e = _mm_set1_epi8('e');
    const auto upper_e = _mm_set1_epi8('E');
    while (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto ge_zero = _mm_cmpgt_epi8(chunk, slash);
      const auto le_nine = _mm_cmpgt_epi8(colon, chunk);
      auto accepted = _mm_and_si128(ge_zero, le_nine);
      accepted = _mm_or_si128(accepted, _mm_cmpeq_epi8(chunk, minus));
      accepted = _mm_or_si128(accepted, _mm_cmpeq_epi8(chunk, plus));
      accepted = _mm_or_si128(accepted, _mm_cmpeq_epi8(chunk, dot));
      accepted = _mm_or_si128(accepted, _mm_cmpeq_epi8(chunk, lower_e));
      accepted = _mm_or_si128(accepted, _mm_cmpeq_epi8(chunk, upper_e));
      const auto mask = static_cast<uint32_t>(_mm_movemask_epi8(accepted));
      if (mask == 0xFFFFu) {
        p += 16;
        continue;
      }
      return p + first_set_bit((~mask) & 0xFFFFu);
    }
#endif

    while (p != end && is_number_token_char(*p)) {
      ++p;
    }
    return p;
  }

  void append_array_child(uint32_t child_id, uint32_t& first_child,
                          uint32_t& last_child, uint32_t& count) {
    if (count == 0) {
      first_child = child_id;
    }
    else {
      doc_.nodes_[last_child].next = child_id;
    }
    last_child = child_id;
    ++count;
  }

  void finish_array(uint32_t array_id, uint32_t first_child,
                    uint32_t last_child, uint32_t count) noexcept {
    doc_.nodes_[array_id].set_span(first_child, count);
    if (last_child != invalid_index) {
      doc_.nodes_[last_child].next = invalid_index;
    }
  }

  void append_object_member(string_ref key, uint32_t value_id,
                            uint32_t& first_member, uint32_t& last_member,
                            uint32_t& count) {
    const auto member_id = narrow_u32(doc_.members_.size());
    doc_.members_.emplace_back_unchecked(key, value_id);
    if constexpr (RejectDuplicateKeys) {
      doc_.members_[member_id].next = invalid_index;
    }
    if (count == 0) {
      first_member = member_id;
    }
    else {
      doc_.members_[last_member].next = member_id;
    }
    last_member = member_id;
    ++count;
  }

  void finish_object(uint32_t object_id, uint32_t first_member,
                     uint32_t last_member, uint32_t count) noexcept {
    doc_.nodes_[object_id].set_span(first_member, count);
    if (last_member != invalid_index) {
      doc_.members_[last_member].next = invalid_index;
    }
  }

  uint32_t parse_array() {
    const auto id = add_typed_node(kind::array);
    uint32_t first_child = invalid_index;
    uint32_t last_child = invalid_index;
    uint32_t count = 0;

    ++pos_;
    skip_ws();
    if (consume(']')) {
      finish_array(id, first_child, last_child, count);
      return id;
    }

    while (true) {
      const auto child = parse_value_after_ws();
      append_array_child(child, first_child, last_child, count);

      skip_ws();
      if (consume(']')) {
        finish_array(id, first_child, last_child, count);
        return id;
      }

      expect(',');
      skip_ws();
      if (pos_ < input_.size() && input_[pos_] == ']') {
        fail("trailing comma in array");
      }
    }
  }

  uint32_t parse_object() {
    const auto id = add_typed_node(kind::object);
    uint32_t first_member = invalid_index;
    uint32_t last_member = invalid_index;
    uint32_t count = 0;

    ++pos_;
    skip_ws();
    if (consume('}')) {
      finish_object(id, first_member, last_member, count);
      return id;
    }

    while (true) {
      if (pos_ == input_.size() || input_[pos_] != '"') {
        fail("expected object key");
      }
      auto key = parse_string_ref();

      if constexpr (RejectDuplicateKeys) {
        const auto key_view = doc_.resolve(key);
        for (auto current = first_member; current != invalid_index;
             current = doc_.members_[current].next) {
          if (doc_.resolve(doc_.members_[current].key) == key_view) {
            std::string message("duplicated object key: ");
            message.append(key_view.data(), key_view.size());
            fail(message);
          }
        }
      }

      skip_ws();
      expect(':');
      skip_ws();
      const auto child = parse_value_after_ws();
      append_object_member(key, child, first_member, last_member, count);

      skip_ws();
      if (consume('}')) {
        finish_object(id, first_member, last_member, count);
        return id;
      }

      expect(',');
      skip_ws();
      if (pos_ < input_.size() && input_[pos_] == '}') {
        fail("trailing comma in object");
      }
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE string_ref parse_string_ref() {
    ++pos_;
    const auto content_start = pos_;
    const auto event = scan_simple_string_end(content_start);
    if (event != npos && input_[event] == '"') {
      pos_ = event + 1;
      return {string_storage::input, narrow_u32(content_start),
              checked_string_length(event - content_start)};
    }

    return parse_string_ref_slow(content_start, event);
  }

  IGUANA_JSON_DOM_NOINLINE string_ref
  parse_string_ref_slow(size_t content_start, size_t event) {
    if (event == npos) {
      fail("unterminated string");
    }

    pos_ = event;
    while (pos_ < input_.size()) {
      const unsigned char uc = static_cast<unsigned char>(input_[pos_]);
      if (uc == '"') {
        const auto content_end = pos_;
        ++pos_;
        return {string_storage::input, narrow_u32(content_start),
                checked_string_length(content_end - content_start)};
      }

      if (uc == '\\') {
        return parse_escaped_string_ref(content_start);
      }

      if (uc < 0x20u) {
        fail("control character in string");
      }

      if constexpr (ValidateUtf8) {
        if (uc >= 0x80u) {
          pos_ += validate_utf8_sequence(pos_);
        }
        else {
          ++pos_;
        }
      }
      else {
        ++pos_;
      }
    }

    fail("unterminated string");
  }

  string_ref parse_escaped_string_ref(size_t content_start) {
    prepare_string_pool();
    const auto pooled_offset = doc_.string_pool_.size();
    const auto maximum_output = input_.size() - content_start;
    char* const output_start =
        doc_.string_pool_.append_uninitialized(maximum_output);
    const char* src = input_.data() + content_start;
    const char* const end = input_.data() + input_.size();
    char* dst = output_start;

    auto finish = [&]() {
      const auto length = static_cast<size_t>(dst - output_start);
      pos_ = static_cast<size_t>(src - input_.data()) + 1;
      doc_.string_pool_.resize_uninitialized(pooled_offset + length);
      return string_ref::pool(narrow_u32(pooled_offset),
                              checked_string_length(length));
    };

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto quote = _mm256_set1_epi8('"');
    const auto slash = _mm256_set1_epi8('\\');
    while (static_cast<size_t>(end - src) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src));
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), chunk);
      auto event_mask = static_cast<uint32_t>(_mm256_movemask_epi8(
          _mm256_or_si256(_mm256_cmpeq_epi8(chunk, quote),
                          _mm256_cmpeq_epi8(chunk, slash))));
      if constexpr (ValidateUtf8) {
        event_mask |= static_cast<uint32_t>(_mm256_movemask_epi8(chunk));
      }
      if (event_mask == 0) {
        src += 32;
        dst += 32;
        continue;
      }

      const auto event = first_set_bit(event_mask);
      src += event;
      dst += event;
      const auto c = static_cast<unsigned char>(*src);
      if (c == '"') {
        return finish();
      }
      if (c == '\\') {
        decode_escape(src, dst);
        continue;
      }
      if constexpr (ValidateUtf8) {
        const auto length =
            validate_utf8_sequence(static_cast<size_t>(src - input_.data()));
        src += length;
        dst += length;
      }
    }
#endif

    while (src != end) {
      const auto c = static_cast<unsigned char>(*src);
      if (c == '"') {
        return finish();
      }
      if (c == '\\') {
        decode_escape(src, dst);
        continue;
      }
      if (c < 0x20u) {
        fail_at(static_cast<size_t>(src - input_.data()),
                "control character in string");
      }
      if constexpr (ValidateUtf8) {
        if (c >= 0x80u) {
          const auto length =
              validate_utf8_sequence(static_cast<size_t>(src - input_.data()));
          std::memcpy(dst, src, length);
          src += length;
          dst += length;
          continue;
        }
      }
      *dst++ = *src++;
    }

    fail_at(input_.size(), "unterminated string");
  }

  void decode_escape(const char*& src, char*& dst) {
    const auto escape_offset = static_cast<size_t>(src - input_.data());
    if (end_pointer() - src < 2) {
      fail_at(escape_offset, "unterminated escape");
    }

    const char escaped = src[1];
    src += 2;
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        *dst++ = escaped;
        return;
      case 'b':
        *dst++ = '\b';
        return;
      case 'f':
        *dst++ = '\f';
        return;
      case 'n':
        *dst++ = '\n';
        return;
      case 'r':
        *dst++ = '\r';
        return;
      case 't':
        *dst++ = '\t';
        return;
      case 'u': {
        auto codepoint = parse_hex4(src);
        if (codepoint >= 0xD800u && codepoint <= 0xDBFFu) {
          if (end_pointer() - src < 6 || src[0] != '\\' || src[1] != 'u') {
            fail_at(static_cast<size_t>(src - input_.data()),
                    "missing low surrogate");
          }
          src += 2;
          const auto low = parse_hex4(src);
          if (low < 0xDC00u || low > 0xDFFFu) {
            fail_at(static_cast<size_t>(src - input_.data()) - 4,
                    "invalid low surrogate");
          }
          codepoint =
              0x10000u + ((codepoint - 0xD800u) << 10u) + (low - 0xDC00u);
        }
        else if (codepoint >= 0xDC00u && codepoint <= 0xDFFFu) {
          fail_at(escape_offset, "unexpected low surrogate");
        }
        append_utf8(dst, codepoint);
        return;
      }
      default:
        fail_at(escape_offset, "invalid escape");
    }
  }

  uint32_t parse_hex4(const char*& src) const {
    if (end_pointer() - src < 4) {
      fail_at(static_cast<size_t>(src - input_.data()),
              "incomplete unicode escape");
    }

    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
      if (!is_hex(src[i])) {
        fail_at(static_cast<size_t>(src + i - input_.data()),
                "invalid unicode escape");
      }
      value = (value << 4u) | hex_value(src[i]);
    }
    src += 4;
    return value;
  }

  static void append_utf8(char*& dst, uint32_t codepoint) noexcept {
    if (codepoint <= 0x7Fu) {
      *dst++ = static_cast<char>(codepoint);
    }
    else if (codepoint <= 0x7FFu) {
      *dst++ = static_cast<char>(0xC0u | (codepoint >> 6u));
      *dst++ = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
    else if (codepoint <= 0xFFFFu) {
      *dst++ = static_cast<char>(0xE0u | (codepoint >> 12u));
      *dst++ = static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu));
      *dst++ = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
    else {
      *dst++ = static_cast<char>(0xF0u | (codepoint >> 18u));
      *dst++ = static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu));
      *dst++ = static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu));
      *dst++ = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
  }

  const char* end_pointer() const noexcept {
    return input_.data() + input_.size();
  }

  static constexpr size_t npos = (std::numeric_limits<size_t>::max)();

  static size_t first_set_bit(uint32_t mask) noexcept {
    return static_cast<size_t>(std::countr_zero(mask));
  }

  size_t scan_simple_string_end(size_t start) const noexcept {
    const auto* const base = input_.data();
    const auto* p = base + start;
    const auto* const end = base + input_.size();

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto quote128 = _mm_set1_epi8('"');
    const auto slash128 = _mm_set1_epi8('\\');
    if (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto quote_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, quote128)));
      const auto slash_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, slash128)));
      uint32_t high_mask = 0;
      if constexpr (ValidateUtf8) {
        high_mask = static_cast<uint32_t>(_mm_movemask_epi8(chunk));
      }
      const auto mask = quote_mask | slash_mask | high_mask;
      if (mask != 0) {
        return static_cast<size_t>((p + first_set_bit(mask)) - base);
      }
      p += 16;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto quote256 = _mm256_set1_epi8('"');
    const auto slash256 = _mm256_set1_epi8('\\');
    while (static_cast<size_t>(end - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      const auto quote_mask = static_cast<uint32_t>(
          _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, quote256)));
      const auto slash_mask = static_cast<uint32_t>(
          _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, slash256)));
      uint32_t high_mask = 0;
      if constexpr (ValidateUtf8) {
        high_mask = static_cast<uint32_t>(_mm256_movemask_epi8(chunk));
      }
      const auto mask = quote_mask | slash_mask | high_mask;
      if (mask != 0) {
        const auto index = first_set_bit(mask);
        return static_cast<size_t>((p + index) - base);
      }
      p += 32;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    while (static_cast<size_t>(end - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto quote_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, quote128)));
      const auto slash_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, slash128)));
      uint32_t high_mask = 0;
      if constexpr (ValidateUtf8) {
        high_mask = static_cast<uint32_t>(_mm_movemask_epi8(chunk));
      }
      const auto mask = quote_mask | slash_mask | high_mask;
      if (mask != 0) {
        const auto index = first_set_bit(mask);
        return static_cast<size_t>((p + index) - base);
      }
      p += 16;
    }
#endif

    while (p != end) {
      const auto c = static_cast<unsigned char>(*p);
      if (c == '"') {
        return static_cast<size_t>(p - base);
      }
      if (c == '\\' || (ValidateUtf8 && c >= 0x80u)) {
        return static_cast<size_t>(p - base);
      }
      ++p;
    }
    return npos;
  }

  void prepare_string_pool() {
    if (doc_.string_pool_.capacity() != 0) {
      return;
    }
    doc_.string_pool_.reserve(input_.size());
  }

  void append_escape(std::string& out) {
    if (pos_ == input_.size()) {
      fail("unterminated escape");
    }

    const char escaped = input_[pos_++];
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        out += escaped;
        return;
      case 'b':
        out += '\b';
        return;
      case 'f':
        out += '\f';
        return;
      case 'n':
        out += '\n';
        return;
      case 'r':
        out += '\r';
        return;
      case 't':
        out += '\t';
        return;
      case 'u': {
        auto codepoint = parse_hex4();
        if (codepoint >= 0xD800u && codepoint <= 0xDBFFu) {
          if (pos_ + 1 >= input_.size() || input_[pos_] != '\\' ||
              input_[pos_ + 1] != 'u') {
            fail("missing low surrogate");
          }
          pos_ += 2;
          const auto low = parse_hex4();
          if (low < 0xDC00u || low > 0xDFFFu) {
            fail("invalid low surrogate");
          }
          codepoint =
              0x10000u + ((codepoint - 0xD800u) << 10u) + (low - 0xDC00u);
        }
        else if (codepoint >= 0xDC00u && codepoint <= 0xDFFFu) {
          fail("unexpected low surrogate");
        }
        append_utf8(out, codepoint);
        return;
      }
      default:
        fail("invalid escape");
    }
  }

  uint32_t parse_hex4() {
    if (input_.size() - pos_ < 4) {
      fail("incomplete unicode escape");
    }

    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
      const auto c = input_[pos_ + i];
      if (!is_hex(c)) {
        fail_at(pos_ + i, "invalid unicode escape");
      }
      value = (value << 4u) | hex_value(c);
    }
    pos_ += 4;
    return value;
  }

  static void append_utf8(std::string& out, uint32_t codepoint) {
    if (codepoint <= 0x7Fu) {
      out += static_cast<char>(codepoint);
    }
    else if (codepoint <= 0x7FFu) {
      out += static_cast<char>(0xC0u | (codepoint >> 6u));
      out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
    else if (codepoint <= 0xFFFFu) {
      out += static_cast<char>(0xE0u | (codepoint >> 12u));
      out += static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu));
      out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
    else {
      out += static_cast<char>(0xF0u | (codepoint >> 18u));
      out += static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu));
      out += static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu));
      out += static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
  }

  size_t validate_utf8_sequence(size_t offset) const {
    const auto remaining = input_.size() - offset;
    const auto c0 = static_cast<unsigned char>(input_[offset]);

    auto require_cont = [&](size_t index) {
      if (index >= input_.size() ||
          !is_continuation(static_cast<unsigned char>(input_[index]))) {
        fail_at(index, "invalid utf-8 in string");
      }
    };

    if (c0 >= 0xC2u && c0 <= 0xDFu) {
      if (remaining < 2) {
        fail_at(offset, "truncated utf-8 in string");
      }
      require_cont(offset + 1);
      return 2;
    }

    if (c0 == 0xE0u) {
      if (remaining < 3) {
        fail_at(offset, "truncated utf-8 in string");
      }
      const auto c1 = static_cast<unsigned char>(input_[offset + 1]);
      if (c1 < 0xA0u || c1 > 0xBFu) {
        fail_at(offset + 1, "invalid utf-8 in string");
      }
      require_cont(offset + 2);
      return 3;
    }

    if ((c0 >= 0xE1u && c0 <= 0xECu) || (c0 >= 0xEEu && c0 <= 0xEFu)) {
      if (remaining < 3) {
        fail_at(offset, "truncated utf-8 in string");
      }
      require_cont(offset + 1);
      require_cont(offset + 2);
      return 3;
    }

    if (c0 == 0xEDu) {
      if (remaining < 3) {
        fail_at(offset, "truncated utf-8 in string");
      }
      const auto c1 = static_cast<unsigned char>(input_[offset + 1]);
      if (c1 < 0x80u || c1 > 0x9Fu) {
        fail_at(offset + 1, "invalid utf-8 surrogate in string");
      }
      require_cont(offset + 2);
      return 3;
    }

    if (c0 == 0xF0u) {
      if (remaining < 4) {
        fail_at(offset, "truncated utf-8 in string");
      }
      const auto c1 = static_cast<unsigned char>(input_[offset + 1]);
      if (c1 < 0x90u || c1 > 0xBFu) {
        fail_at(offset + 1, "invalid utf-8 in string");
      }
      require_cont(offset + 2);
      require_cont(offset + 3);
      return 4;
    }

    if (c0 >= 0xF1u && c0 <= 0xF3u) {
      if (remaining < 4) {
        fail_at(offset, "truncated utf-8 in string");
      }
      require_cont(offset + 1);
      require_cont(offset + 2);
      require_cont(offset + 3);
      return 4;
    }

    if (c0 == 0xF4u) {
      if (remaining < 4) {
        fail_at(offset, "truncated utf-8 in string");
      }
      const auto c1 = static_cast<unsigned char>(input_[offset + 1]);
      if (c1 < 0x80u || c1 > 0x8Fu) {
        fail_at(offset + 1, "invalid utf-8 in string");
      }
      require_cont(offset + 2);
      require_cont(offset + 3);
      return 4;
    }

    fail_at(offset, "invalid utf-8 in string");
  }

  string_ref add_pooled_string(const std::string& value) {
    const auto offset =
        checked_u32(doc_.string_pool_.size(), "json string pool");
    const auto length = checked_string_length(value.size());
    doc_.string_pool_.append(value.data(), value.size());
    return string_ref::pool(offset, length);
  }

  document& doc_;
  std::string_view input_;
  size_t pos_ = 0;
  const uint32_t* next_structural_ = nullptr;
  const uint32_t* structural_end_ = nullptr;
  size_t pending_scalar_end_ = 0;
  stream_state stream_state_ = stream_state::value;
  bool pending_scalar_ = false;
};

class trusted_dom_parser {
 public:
  explicit trusted_dom_parser(document& doc)
      : doc_(doc),
        input_(doc.source()),
        base_(input_.data()),
        p_(input_.data()),
        end_(input_.data() + input_.size()) {
    doc_.tape_mode_ = false;
  }

  void run() { run_iterative(); }

 private:
  using parse_frame = detail::parse_frame;

  [[noreturn]] IGUANA_JSON_DOM_NOINLINE void fail(
      std::string_view message) const {
    throw parse_error(offset(), message);
  }

  [[noreturn]] IGUANA_JSON_DOM_NOINLINE void fail_at(
      const char* p, std::string_view message) const {
    throw parse_error(static_cast<size_t>(p - base_), message);
  }

  void run_iterative() {
    doc_.parse_stack_.clear();
    doc_.reserve_trusted_for_input(input_.size());
    if (doc_.parse_stack_.capacity() < 256) {
      doc_.parse_stack_.reserve(256);
    }
    skip_ws();
    if (p_ == end_) {
      fail("empty input");
    }

    goto value_begin;

  value_begin:
    if (*p_ == '"') {
      const auto id = parse_string_node();
      accept_value(id);
      goto value_done;
    }
    if ((*p_ >= '0' && *p_ <= '9') || *p_ == '-') {
      const auto id = parse_number();
      accept_value(id);
      goto value_done;
    }
    if (*p_ == '{') {
      const auto id = add_typed_node(kind::object);
      accept_value(id);
      push_frame(kind::object, id);
      ++p_;
      skip_ws_if_needed();
      if (*p_ == '}') {
        ++p_;
        close_frame();
        goto value_done;
      }
      goto object_key_begin;
    }
    if (*p_ == '[') {
      const auto id = add_typed_node(kind::array);
      accept_value(id);
      push_frame(kind::array, id);
      ++p_;
      skip_ws_if_needed();
      if (*p_ == ']') {
        ++p_;
        close_frame();
        goto value_done;
      }
      goto value_begin;
    }
    if (*p_ == 'n') {
      const auto id = parse_null();
      accept_value(id);
      goto value_done;
    }
    if (*p_ == 't' || *p_ == 'f') {
      const auto id = parse_bool();
      accept_value(id);
      goto value_done;
    }
    fail("expected value");

  object_key_begin:
    doc_.parse_stack_.back().key = parse_key_ref();
    skip_ws_if_needed();
    ++p_;
    skip_ws_if_needed();
    goto value_begin;

  value_done:
    if (doc_.parse_stack_.empty()) {
      skip_ws();
      if (p_ != end_) {
        fail("unexpected trailing data");
      }
      return;
    }

    if (doc_.parse_stack_.back().type == kind::array) {
      goto array_value_done;
    }
    goto object_value_done;

  array_value_done:
    skip_ws_if_needed();
    if (*p_ == ',') {
      ++p_;
      skip_ws_if_needed();
      goto value_begin;
    }
    ++p_;
    close_frame();
    goto value_done;

  object_value_done:
    skip_ws_if_needed();
    if (*p_ == ',') {
      ++p_;
      skip_ws_if_needed();
      goto object_key_begin;
    }
    ++p_;
    close_frame();
    goto value_done;
  }

  size_t offset() const noexcept { return static_cast<size_t>(p_ - base_); }

  static bool is_ws(char c) noexcept {
    return static_cast<unsigned char>(c) <= static_cast<unsigned char>(' ');
  }

  static bool is_number_delimiter(char c) noexcept {
    return c == ',' || c == ']' || c == '}' || is_ws(c);
  }

  static bool is_hex(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
  }

  static uint32_t hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
      return static_cast<uint32_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
      return static_cast<uint32_t>(c - 'a' + 10);
    }
    return static_cast<uint32_t>(c - 'A' + 10);
  }

  static size_t first_set_bit(uint32_t mask) noexcept {
    return static_cast<size_t>(std::countr_zero(mask));
  }

  static uint64_t repeat_byte(unsigned char c) noexcept {
    return 0x0101010101010101ull * static_cast<uint64_t>(c);
  }

  static uint64_t has_zero_byte(uint64_t word) noexcept {
    return (word - 0x0101010101010101ull) & ~word & 0x8080808080808080ull;
  }

  static uint64_t match_byte(uint64_t word, unsigned char c) noexcept {
    return has_zero_byte(word ^ repeat_byte(c));
  }

  static uint64_t has_byte_less_than(uint64_t word, unsigned char c) noexcept {
    return (word - repeat_byte(c)) & ~word & repeat_byte(0x80u);
  }

  static uint64_t load_u64(const char* p) noexcept {
    uint64_t word = 0;
    std::memcpy(&word, p, sizeof(word));
    return word;
  }

  static uint32_t narrow_u32(size_t value) noexcept {
    return static_cast<uint32_t>(value);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void push_frame(kind type, uint32_t id) {
    doc_.parse_stack_.push_back(
        parse_frame{type, id, invalid_index, invalid_index, 0, {}});
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void append_to_current(uint32_t value_id) {
    auto& frame = doc_.parse_stack_.back();
    if (frame.type == kind::array) {
      if (frame.count == 0) {
        frame.first = value_id;
      }
      else {
        doc_.nodes_[frame.last].next = value_id;
      }
      frame.last = value_id;
      ++frame.count;
      return;
    }

    const auto member_id = narrow_u32(doc_.members_.size());
    doc_.members_.emplace_back_unchecked(frame.key, value_id);
    if (frame.count == 0) {
      frame.first = member_id;
    }
    else {
      doc_.members_[frame.last].next = member_id;
    }
    frame.last = member_id;
    ++frame.count;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void accept_value(uint32_t value_id) {
    if (doc_.parse_stack_.empty()) {
      doc_.root_ = value_id;
      return;
    }
    append_to_current(value_id);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void close_frame() noexcept {
    const auto frame = doc_.parse_stack_.back();
    doc_.parse_stack_.pop_back();
    doc_.nodes_[frame.id].set_span(frame.first, frame.count);
    if (frame.last == invalid_index) {
      return;
    }
    if (frame.type == kind::array) {
      doc_.nodes_[frame.last].next = invalid_index;
    }
    else {
      doc_.members_[frame.last].next = invalid_index;
    }
  }

  uint32_t checked_string_length(size_t value) const {
    if (value > string_ref::length_mask) {
      throw std::length_error("json string length exceeds packed DOM range");
    }
    return static_cast<uint32_t>(value);
  }

  node& emplace_node(uint32_t& id) {
    id = narrow_u32(doc_.nodes_.size());
    return doc_.nodes_.emplace_back_unchecked();
  }

  uint32_t add_typed_node(kind type) {
    uint32_t id = 0;
    auto& n = emplace_node(id);
    n.type = type;
    return id;
  }

  uint32_t add_raw_number_node(const char* start, const char* end) {
    uint32_t id = 0;
    auto& n = emplace_node(id);
    n.type = kind::raw_number;
    n.storage = string_storage::input;
    n.set_span(narrow_u32(static_cast<size_t>(start - base_)),
               narrow_u32(static_cast<size_t>(end - start)));
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void skip_ws() noexcept {
    if (p_ == end_ || !is_ws(*p_)) {
      return;
    }
    skip_ws_after_first();
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void skip_ws_after_first() noexcept {
#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto space256 = _mm256_set1_epi8(' ');
    const auto line_feed256 = _mm256_set1_epi8('\n');
    const auto carriage_return256 = _mm256_set1_epi8('\r');
    const auto tab256 = _mm256_set1_epi8('\t');
    while (static_cast<size_t>(end_ - p_) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p_));
      const auto mask =
          static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(
              _mm256_or_si256(_mm256_cmpeq_epi8(chunk, space256),
                              _mm256_cmpeq_epi8(chunk, line_feed256)),
              _mm256_or_si256(_mm256_cmpeq_epi8(chunk, carriage_return256),
                              _mm256_cmpeq_epi8(chunk, tab256)))));
      if (mask == 0xFFFFFFFFu) {
        p_ += 32;
        continue;
      }
      p_ += first_set_bit(~mask);
      return;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto space = _mm_set1_epi8(' ');
    const auto line_feed = _mm_set1_epi8('\n');
    const auto carriage_return = _mm_set1_epi8('\r');
    const auto tab = _mm_set1_epi8('\t');
    while (static_cast<size_t>(end_ - p_) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p_));
      const auto mask = static_cast<uint32_t>(_mm_movemask_epi8(
          _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, space),
                                    _mm_cmpeq_epi8(chunk, line_feed)),
                       _mm_or_si128(_mm_cmpeq_epi8(chunk, carriage_return),
                                    _mm_cmpeq_epi8(chunk, tab)))));
      if (mask == 0xFFFFu) {
        p_ += 16;
        continue;
      }
      p_ += first_set_bit((~mask) & 0xFFFFu);
      return;
    }
#endif

    while (p_ != end_ && is_ws(*p_)) {
      ++p_;
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void skip_ws_if_needed() noexcept {
    if (p_ != end_ && is_ws(*p_)) {
      skip_ws_after_first();
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_value_after_ws() {
    if ((*p_ >= '0' && *p_ <= '9') || *p_ == '-') {
      return parse_number();
    }

    switch (*p_) {
      case 'n':
        return parse_null();
      case 't':
      case 'f':
        return parse_bool();
      case '"':
        return parse_string_node();
      case '[':
        return parse_array();
      case '{':
        return parse_object();
      default:
        fail("expected value");
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_null() {
    p_ += 4;
    uint32_t id = 0;
    auto& n = emplace_node(id);
    n.type = kind::null;
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_bool() {
    uint32_t id = 0;
    auto& n = emplace_node(id);
    n.type = kind::boolean;
    if (*p_ == 't') {
      p_ += 4;
      n.u64 = 1;
    }
    else {
      p_ += 5;
      n.u64 = 0;
    }
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_string_node() {
    const auto ref = parse_string_ref();
    uint32_t id = 0;
    auto& n = emplace_node(id);
    n.type = kind::string;
    n.storage = ref.storage();
    n.set_span(ref.offset(), ref.length());
    return id;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_number() {
    const char* const start = p_;
    p_ = scan_number_token(p_);
    return add_raw_number_node(start, p_);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE const char* scan_number_token(
      const char* p) const noexcept {
    const char* const swar_end =
        p + (std::min)(static_cast<size_t>(end_ - p), static_cast<size_t>(16));
    while (static_cast<size_t>(swar_end - p) >= 8) {
      const auto word = load_u64(p);
      const auto mask =
          match_byte(word, ',') | match_byte(word, ']') |
          match_byte(word, '}') |
          has_byte_less_than(word, static_cast<unsigned char>(' ' + 1));
      if (mask != 0) {
        return p + (std::countr_zero(mask) >> 3u);
      }
      p += 8;
    }
    while (p != swar_end) {
      if (is_number_delimiter(*p)) {
        return p;
      }
      ++p;
    }

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto comma256 = _mm256_set1_epi8(',');
    const auto close_array256 = _mm256_set1_epi8(']');
    const auto close_object256 = _mm256_set1_epi8('}');
    const auto space_plus_one256 = _mm256_set1_epi8(' ' + 1);
    while (static_cast<size_t>(end_ - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      auto delimiter = _mm256_cmpeq_epi8(chunk, comma256);
      delimiter =
          _mm256_or_si256(delimiter, _mm256_cmpeq_epi8(chunk, close_array256));
      delimiter =
          _mm256_or_si256(delimiter, _mm256_cmpeq_epi8(chunk, close_object256));
      delimiter = _mm256_or_si256(delimiter,
                                  _mm256_cmpgt_epi8(space_plus_one256, chunk));
      const auto mask = static_cast<uint32_t>(_mm256_movemask_epi8(delimiter));
      if (mask != 0) {
        return p + first_set_bit(mask);
      }
      p += 32;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto comma = _mm_set1_epi8(',');
    const auto close_array = _mm_set1_epi8(']');
    const auto close_object = _mm_set1_epi8('}');
    const auto space_plus_one = _mm_set1_epi8(' ' + 1);
    while (static_cast<size_t>(end_ - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      auto delimiter = _mm_cmpeq_epi8(chunk, comma);
      delimiter = _mm_or_si128(delimiter, _mm_cmpeq_epi8(chunk, close_array));
      delimiter = _mm_or_si128(delimiter, _mm_cmpeq_epi8(chunk, close_object));
      delimiter =
          _mm_or_si128(delimiter, _mm_cmplt_epi8(chunk, space_plus_one));
      const auto mask = static_cast<uint32_t>(_mm_movemask_epi8(delimiter));
      if (mask != 0) {
        return p + first_set_bit(mask);
      }
      p += 16;
    }
#endif

    while (p != end_ && !is_number_delimiter(*p)) {
      ++p;
    }
    return p;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void append_array_child(uint32_t child_id,
                                                        uint32_t& first_child,
                                                        uint32_t& last_child,
                                                        uint32_t& count) {
    if (count == 0) {
      first_child = child_id;
    }
    else {
      doc_.nodes_[last_child].next = child_id;
    }
    last_child = child_id;
    ++count;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void finish_array(
      uint32_t array_id, uint32_t first_child, uint32_t count,
      uint32_t last_child) noexcept {
    doc_.nodes_[array_id].set_span(first_child, count);
    if (last_child != invalid_index) {
      doc_.nodes_[last_child].next = invalid_index;
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void append_object_member(
      string_ref key, uint32_t value_id, uint32_t& first_member,
      uint32_t& last_member, uint32_t& count) {
    const auto member_id = narrow_u32(doc_.members_.size());
    doc_.members_.emplace_back_unchecked(key, value_id);
    if (count == 0) {
      first_member = member_id;
    }
    else {
      doc_.members_[last_member].next = member_id;
    }
    last_member = member_id;
    ++count;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE void finish_object(
      uint32_t object_id, uint32_t first_member, uint32_t count,
      uint32_t last_member) noexcept {
    doc_.nodes_[object_id].set_span(first_member, count);
    if (last_member != invalid_index) {
      doc_.members_[last_member].next = invalid_index;
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_array() {
    const auto id = add_typed_node(kind::array);
    uint32_t first_child = invalid_index;
    uint32_t last_child = invalid_index;
    uint32_t count = 0;

    ++p_;
    skip_ws_if_needed();
    if (*p_ == ']') {
      ++p_;
      finish_array(id, first_child, count, last_child);
      return id;
    }

    while (true) {
      const auto child = parse_value_after_ws();
      append_array_child(child, first_child, last_child, count);

      skip_ws_if_needed();
      if (*p_ == ']') {
        ++p_;
        finish_array(id, first_child, count, last_child);
        return id;
      }
      ++p_;
      skip_ws_if_needed();
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE uint32_t parse_object() {
    const auto id = add_typed_node(kind::object);
    uint32_t first_member = invalid_index;
    uint32_t last_member = invalid_index;
    uint32_t count = 0;

    ++p_;
    skip_ws_if_needed();
    if (*p_ == '}') {
      ++p_;
      finish_object(id, first_member, count, last_member);
      return id;
    }

    while (true) {
      const auto key = parse_key_ref();
      skip_ws_if_needed();
      ++p_;
      skip_ws_if_needed();
      const auto child = parse_value_after_ws();
      append_object_member(key, child, first_member, last_member, count);

      skip_ws_if_needed();
      if (*p_ == '}') {
        ++p_;
        finish_object(id, first_member, count, last_member);
        return id;
      }
      ++p_;
      skip_ws_if_needed();
    }
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE string_ref parse_string_ref() {
    ++p_;
    const char* const content_start = p_;
    const char* const event = scan_simple_string_end(content_start);
    if (event != nullptr && *event == '"') {
      p_ = event + 1;
      return string_ref::input(
          narrow_u32(static_cast<size_t>(content_start - base_)),
          checked_string_length(static_cast<size_t>(event - content_start)));
    }

    p_ = event == nullptr ? end_ : event;
    return parse_string_ref_slow(content_start);
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE string_ref parse_key_ref() {
    ++p_;
    const char* const content_start = p_;
    const char* const event = scan_key_string_end(content_start);
    if (event != nullptr && *event == '"') {
      p_ = event + 1;
      return string_ref::input(
          narrow_u32(static_cast<size_t>(content_start - base_)),
          checked_string_length(static_cast<size_t>(event - content_start)));
    }

    p_ = event == nullptr ? end_ : event;
    return parse_string_ref_slow(content_start);
  }

  string_ref parse_string_ref_slow(const char* content_start) {
    const char* segment_start = content_start;
    size_t pooled_offset = 0;
    bool has_escape = false;

    while (p_ < end_) {
      const unsigned char uc = static_cast<unsigned char>(*p_);
      if (uc == '"') {
        const char* const content_end = p_;
        ++p_;
        if (!has_escape) {
          return string_ref::input(
              narrow_u32(static_cast<size_t>(content_start - base_)),
              checked_string_length(
                  static_cast<size_t>(content_end - content_start)));
        }
        doc_.string_pool_.append(segment_start, content_end - segment_start);
        return string_ref::pool(
            narrow_u32(pooled_offset),
            checked_string_length(doc_.string_pool_.size() - pooled_offset));
      }

      if (uc == '\\') {
        if (!has_escape) {
          has_escape = true;
          prepare_string_pool();
          pooled_offset = doc_.string_pool_.size();
        }
        doc_.string_pool_.append(segment_start, p_ - segment_start);
        ++p_;
        append_escape(doc_.string_pool_);
        segment_start = p_;
        continue;
      }

      ++p_;
    }

    fail("unterminated string");
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE const char* scan_key_string_end(
      const char* p) const noexcept {
    const char* const prefix_end =
        p + (std::min)(static_cast<size_t>(end_ - p), static_cast<size_t>(64));
    while (static_cast<size_t>(prefix_end - p) >= 8) {
      const auto word = load_u64(p);
      const auto mask = match_byte(word, '"') | match_byte(word, '\\');
      if (mask != 0) {
        const auto index = static_cast<size_t>(std::countr_zero(mask) >> 3u);
        return p + index;
      }
      p += 8;
    }
    while (p != prefix_end) {
      if (*p == '"') {
        return p;
      }
      if (*p == '\\') {
        return p;
      }
      ++p;
    }

    const char* const quote = scan_quote(p);
    if (quote == nullptr) {
      return nullptr;
    }
    const auto length = static_cast<size_t>(quote - p);
    const auto* slash = static_cast<const char*>(std::memchr(p, '\\', length));
    return slash == nullptr ? quote : slash;
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE const char* scan_quote(
      const char* p) const noexcept {
    return static_cast<const char*>(
        std::memchr(p, '"', static_cast<size_t>(end_ - p)));
  }

  IGUANA_JSON_DOM_ALWAYS_INLINE const char* scan_simple_string_end(
      const char* p) const noexcept {
    const char* const swar_end =
        p + (std::min)(static_cast<size_t>(end_ - p), static_cast<size_t>(16));
    while (static_cast<size_t>(swar_end - p) >= 8) {
      const auto word = load_u64(p);
      const auto mask = match_byte(word, '"') | match_byte(word, '\\');
      if (mask != 0) {
        const auto index = static_cast<size_t>(std::countr_zero(mask) >> 3u);
        return p + index;
      }
      p += 8;
    }
    while (p != swar_end) {
      if (*p == '"') {
        return p;
      }
      if (*p == '\\') {
        return p;
      }
      ++p;
    }

#ifdef IGUANA_JSON_DOM_HAS_AVX2
    const auto quote256 = _mm256_set1_epi8('"');
    const auto slash256 = _mm256_set1_epi8('\\');
    while (static_cast<size_t>(end_ - p) >= 32) {
      const auto chunk =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
      const auto quote_mask = static_cast<uint32_t>(
          _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, quote256)));
      const auto slash_mask = static_cast<uint32_t>(
          _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, slash256)));
      const auto mask = quote_mask | slash_mask;
      if (mask != 0) {
        const auto index = first_set_bit(mask);
        return p + index;
      }
      p += 32;
    }
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
    const auto quote = _mm_set1_epi8('"');
    const auto slash = _mm_set1_epi8('\\');
    while (static_cast<size_t>(end_ - p) >= 16) {
      const auto chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
      const auto quote_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, quote)));
      const auto slash_mask = static_cast<uint32_t>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, slash)));
      const auto mask = quote_mask | slash_mask;
      if (mask != 0) {
        const auto index = first_set_bit(mask);
        return p + index;
      }
      p += 16;
    }
#endif

    while (p != end_) {
      if (*p == '"') {
        return p;
      }
      if (*p == '\\') {
        return p;
      }
      ++p;
    }
    return nullptr;
  }

  void prepare_string_pool() {
    if (doc_.string_pool_.capacity() != 0) {
      return;
    }
    doc_.string_pool_.reserve(input_.size());
  }

  void append_escape(dom_vector<char>& out) {
    if (p_ == end_) {
      fail("unterminated escape");
    }

    const char escaped = *p_++;
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        out.push_back(escaped);
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
        auto codepoint = parse_hex4();
        if (codepoint >= 0xD800u && codepoint <= 0xDBFFu) {
          if (end_ - p_ < 2 || p_[0] != '\\' || p_[1] != 'u') {
            fail("missing low surrogate");
          }
          p_ += 2;
          const auto low = parse_hex4();
          if (low < 0xDC00u || low > 0xDFFFu) {
            fail("invalid low surrogate");
          }
          codepoint =
              0x10000u + ((codepoint - 0xD800u) << 10u) + (low - 0xDC00u);
        }
        else if (codepoint >= 0xDC00u && codepoint <= 0xDFFFu) {
          fail("unexpected low surrogate");
        }
        append_utf8(out, codepoint);
        return;
      }
      default:
        fail("invalid escape");
    }
  }

  uint32_t parse_hex4() {
    if (end_ - p_ < 4) {
      fail("incomplete unicode escape");
    }

    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
      const auto c = p_[i];
      if (!is_hex(c)) {
        fail_at(p_ + i, "invalid unicode escape");
      }
      value = (value << 4u) | hex_value(c);
    }
    p_ += 4;
    return value;
  }

  static void append_utf8(dom_vector<char>& out, uint32_t codepoint) {
    if (codepoint <= 0x7Fu) {
      out.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7FFu) {
      out.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
      out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    }
    else if (codepoint <= 0xFFFFu) {
      out.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
      out.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
      out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    }
    else {
      out.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
      out.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
      out.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
      out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    }
  }

  string_ref add_pooled_string(const std::string& value) {
    const auto offset =
        document::checked_index(doc_.string_pool_.size(), "json string pool");
    const auto length = checked_string_length(value.size());
    doc_.string_pool_.append(value.data(), value.size());
    return string_ref::pool(offset, length);
  }

  document& doc_;
  std::string_view input_;
  const char* base_ = nullptr;
  const char* p_ = nullptr;
  const char* end_ = nullptr;
};

template <number_mode Numbers, bool ValidateUtf8, bool ValidateNumbers,
          bool RejectDuplicateKeys>
inline void run_dom_parser(document& doc) {
#ifdef IGUANA_JSON_DOM_HAS_AVX2
  if constexpr (ValidateUtf8) {
    dom_parser<Numbers, true, ValidateNumbers, RejectDuplicateKeys> validator(
        doc);
    if (validator.scan_structurals_and_validate_utf8()) {
      dom_parser<Numbers, false, ValidateNumbers, RejectDuplicateKeys> parser(
          doc);
      parser.run_preindexed();
    }
    else {
      validator.run_preindexed();
    }
    return;
  }
#endif
  dom_parser<Numbers, ValidateUtf8, ValidateNumbers, RejectDuplicateKeys>
      parser(doc);
  parser.run();
}

inline void run_dom_parser(document& doc, parse_options options) {
  if (options.numbers == number_mode::lazy && !options.validate_utf8 &&
      !options.validate_numbers && !options.reject_duplicate_keys) {
    trusted_dom_parser parser(doc);
    parser.run();
    return;
  }

  if (options.numbers == number_mode::eager) {
    if (options.validate_utf8) {
      if (options.validate_numbers) {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::eager, true, true, true>(doc);
        }
        else {
          run_dom_parser<number_mode::eager, true, true, false>(doc);
        }
      }
      else {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::eager, true, false, true>(doc);
        }
        else {
          run_dom_parser<number_mode::eager, true, false, false>(doc);
        }
      }
    }
    else {
      if (options.validate_numbers) {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::eager, false, true, true>(doc);
        }
        else {
          run_dom_parser<number_mode::eager, false, true, false>(doc);
        }
      }
      else {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::eager, false, false, true>(doc);
        }
        else {
          run_dom_parser<number_mode::eager, false, false, false>(doc);
        }
      }
    }
  }
  else {
    if (options.validate_utf8) {
      if (options.validate_numbers) {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::lazy, true, true, true>(doc);
        }
        else {
          run_dom_parser<number_mode::lazy, true, true, false>(doc);
        }
      }
      else {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::lazy, true, false, true>(doc);
        }
        else {
          run_dom_parser<number_mode::lazy, true, false, false>(doc);
        }
      }
    }
    else {
      if (options.validate_numbers) {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::lazy, false, true, true>(doc);
        }
        else {
          run_dom_parser<number_mode::lazy, false, true, false>(doc);
        }
      }
      else {
        if (options.reject_duplicate_keys) {
          run_dom_parser<number_mode::lazy, false, false, true>(doc);
        }
        else {
          run_dom_parser<number_mode::lazy, false, false, false>(doc);
        }
      }
    }
  }
}

}  // namespace detail

inline void parse(document& doc, std::string_view input,
                  parse_options options = {}) {
  if (input.size() >= detail::invalid_index) {
    throw std::length_error("json input exceeds uint32_t offset range");
  }
  doc.input_.assign(input.data(), input.size());
  doc.input_view_ = std::string_view(doc.input_.data(), doc.input_.size());
  doc.owns_input_ = true;
  doc.string_pool_.clear();
  doc.nodes_.clear();
  doc.members_.clear();
  doc.tape_.clear();
  doc.tape_mode_ = true;
  doc.node_count_ = 0;
  doc.root_ = detail::invalid_index;
  doc.reserve_for_input(input.size());
  detail::run_dom_parser(doc, options);
}

inline document parse(std::string_view input, parse_options options = {}) {
  document doc;
  parse(doc, input, options);
  return doc;
}

inline void parse_view(document& doc, std::string_view input,
                       parse_options options = {}) {
  if (input.size() >= detail::invalid_index) {
    throw std::length_error("json input exceeds uint32_t offset range");
  }
  doc.input_.clear();
  doc.input_view_ = input;
  doc.owns_input_ = false;
  doc.string_pool_.clear();
  doc.nodes_.clear();
  doc.members_.clear();
  doc.tape_.clear();
  doc.tape_mode_ = true;
  doc.node_count_ = 0;
  doc.root_ = detail::invalid_index;
  doc.reserve_for_input(input.size());
  detail::run_dom_parser(doc, options);
}

inline document parse_view(std::string_view input, parse_options options = {}) {
  document doc;
  parse_view(doc, input, options);
  return doc;
}

inline void parse_trusted_view(document& doc, std::string_view input) {
  if (input.size() >= detail::invalid_index) {
    throw std::length_error("json input exceeds uint32_t offset range");
  }
  doc.input_.clear();
  doc.input_view_ = input;
  doc.owns_input_ = false;
  doc.string_pool_.clear();
  doc.nodes_.clear();
  doc.members_.clear();
  doc.tape_.clear();
  doc.tape_mode_ = false;
  doc.node_count_ = 0;
  doc.root_ = detail::invalid_index;
  doc.reserve_for_input(input.size());
  detail::trusted_dom_parser parser(doc);
  parser.run();
}

inline document parse_trusted_view(std::string_view input) {
  document doc;
  parse_trusted_view(doc, input);
  return doc;
}

inline document parse_view(const std::string& input,
                           parse_options options = {}) {
  return parse_view(std::string_view(input.data(), input.size()), options);
}

inline document parse_view(const char* input, parse_options options = {}) {
  if (input == nullptr) {
    throw std::invalid_argument("json input pointer is null");
  }
  return parse_view(std::string_view(input), options);
}

inline void parse_view(document& doc, const std::string& input,
                       parse_options options = {}) {
  parse_view(doc, std::string_view(input.data(), input.size()), options);
}

inline void parse_view(document& doc, const char* input,
                       parse_options options = {}) {
  if (input == nullptr) {
    throw std::invalid_argument("json input pointer is null");
  }
  parse_view(doc, std::string_view(input), options);
}

inline document parse(const std::string& input, parse_options options = {}) {
  return parse(std::string_view(input.data(), input.size()), options);
}

inline document parse(const char* input, parse_options options = {}) {
  if (input == nullptr) {
    throw std::invalid_argument("json input pointer is null");
  }
  return parse(std::string_view(input), options);
}

inline void parse(document& doc, const std::string& input,
                  parse_options options = {}) {
  parse(doc, std::string_view(input.data(), input.size()), options);
}

inline void parse(document& doc, const char* input,
                  parse_options options = {}) {
  if (input == nullptr) {
    throw std::invalid_argument("json input pointer is null");
  }
  parse(doc, std::string_view(input), options);
}

inline document parse(std::span<const char> input, parse_options options = {}) {
  return parse(std::string_view(input.data(), input.size()), options);
}

inline void parse(document& doc, std::span<const char> input,
                  parse_options options = {}) {
  parse(doc, std::string_view(input.data(), input.size()), options);
}

inline document parse(std::span<const std::byte> input,
                      parse_options options = {}) {
  return parse(std::string_view(reinterpret_cast<const char*>(input.data()),
                                input.size()),
               options);
}

inline void parse(document& doc, std::span<const std::byte> input,
                  parse_options options = {}) {
  parse(doc,
        std::string_view(reinterpret_cast<const char*>(input.data()),
                         input.size()),
        options);
}

inline document parse_view(std::span<const char> input,
                           parse_options options = {}) {
  return parse_view(std::string_view(input.data(), input.size()), options);
}

inline void parse_view(document& doc, std::span<const char> input,
                       parse_options options = {}) {
  parse_view(doc, std::string_view(input.data(), input.size()), options);
}

inline document parse_view(std::span<const std::byte> input,
                           parse_options options = {}) {
  return parse_view(
      std::string_view(reinterpret_cast<const char*>(input.data()),
                       input.size()),
      options);
}

inline void parse_view(document& doc, std::span<const std::byte> input,
                       parse_options options = {}) {
  parse_view(doc,
             std::string_view(reinterpret_cast<const char*>(input.data()),
                              input.size()),
             options);
}

}  // namespace iguana::json

#undef IGUANA_JSON_DOM_ALWAYS_INLINE
#undef IGUANA_JSON_DOM_NOINLINE

#ifdef IGUANA_JSON_DOM_HAS_AVX2
#undef IGUANA_JSON_DOM_HAS_AVX2
#endif

#ifdef IGUANA_JSON_DOM_HAS_SSE2
#undef IGUANA_JSON_DOM_HAS_SSE2
#endif
