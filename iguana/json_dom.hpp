#pragma once

#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/json_dom_compare.hpp"

namespace iguana {

class json;
class json_parser;
class json_ref;
template <bool Object, bool Mutable, bool Direct = false>
class json_range;

namespace detail {

struct json_comparison;

template <typename T>
inline constexpr bool json_scalar_v =
    std::is_arithmetic_v<T> || std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::string_view> || std::is_same_v<T, std::nullptr_t>;

template <typename T>
struct json_container {
  static constexpr bool is_array = false;
  static constexpr bool is_object = false;
};

template <typename T, typename Allocator>
struct json_container<std::vector<T, Allocator>> {
  using element = T;
  static constexpr bool is_array = true;
  static constexpr bool is_object = false;
};

template <typename T, typename Compare, typename Allocator>
struct json_container<std::map<std::string, T, Compare, Allocator>> {
  using element = T;
  static constexpr bool is_array = false;
  static constexpr bool is_object = true;
};

template <typename T>
inline constexpr bool json_container_v =
    json_container<T>::is_array || json_container<T>::is_object;

template <typename T>
constexpr bool json_readable(bool allow_string_view = true) {
  if constexpr (json_container_v<T>) {
    return json_readable<typename json_container<T>::element>(false);
  }
  else {
    return json_scalar_v<T> &&
           (allow_string_view || !std::is_same_v<T, std::string_view>);
  }
}

template <typename Char>
using json_char_key = std::enable_if_t<
    std::is_same_v<Char, char> || std::is_same_v<Char, const char>, int>;

template <typename T, typename U>
std::optional<T> json_integer_cast(U value) noexcept {
  if constexpr (std::is_signed_v<U> && !std::is_signed_v<T>) {
    if (value < 0 ||
        static_cast<uint64_t>(value) > std::numeric_limits<T>::max()) {
      return std::nullopt;
    }
  }
  else if constexpr (!std::is_signed_v<U>) {
    if (value > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
      return std::nullopt;
    }
  }
  else if (value < std::numeric_limits<T>::min() ||
           value > std::numeric_limits<T>::max()) {
    return std::nullopt;
  }
  return static_cast<T>(value);
}

inline const json_storage& null_json_storage() {
  static const json_storage storage{{json_node{}}, {}, {}};
  return storage;
}

struct json_member_path {
  std::shared_ptr<const json_member_path> parent;
  std::string key;
  unsigned depth;

  json_index find(const json_storage& storage,
                  json_index anchor) const noexcept {
    auto index = parent ? parent->find(storage, anchor) : anchor;
    return index == no_json_index ? index : storage.find(index, key);
  }

  json_index insert(json_storage& storage, json_index anchor) const {
    auto index = parent ? parent->insert(storage, anchor) : anchor;
    auto& kind = storage.nodes[index].kind;
    if (kind == json_kind::null) {
      kind = json_kind::object;
    }
    if (kind != json_kind::object) {
      throw std::invalid_argument("JSON value is not an object");
    }
    return storage.insert(index, key);
  }
};

inline void write_json_string(std::string& out, std::string_view text) {
  out.push_back('"');
  const auto* first = text.empty() ? "" : text.data();
  const auto* last = first + text.size();
  for (const auto* p = first; p != last;) {
    p = json_ascii_end(p, last);
    if (p == last) {
      break;
    }
    auto byte = static_cast<unsigned char>(*p);
    if (byte >= 0x80) {
      p = json_utf8_end(p, last);
      if (!p) {
        throw std::invalid_argument("invalid UTF-8 in JSON string");
      }
      continue;
    }
    out.append(first, p - first);
    out.push_back('\\');
    switch (byte) {
      case '"':
      case '\\':
        out.push_back(*p);
        break;
      case '\b':
        out.push_back('b');
        break;
      case '\f':
        out.push_back('f');
        break;
      case '\n':
        out.push_back('n');
        break;
      case '\r':
        out.push_back('r');
        break;
      case '\t':
        out.push_back('t');
        break;
      default: {
        constexpr char hex[] = "0123456789abcdef";
        out.append("u00");
        out.push_back(hex[byte >> 4]);
        out.push_back(hex[byte & 0xf]);
      }
    }
    first = ++p;
  }
  out.append(first, last - first);
  out.push_back('"');
}

inline void write_json_node(std::string& out, const json_storage& storage,
                            json_index index, int indent, size_t depth) {
  auto node = storage.nodes[index];
  char number[65];
  switch (node.kind) {
    case json_kind::null:
      out.append("null");
      return;
    case json_kind::boolean:
      out.append(node.data ? "true" : "false");
      return;
    case json_kind::integer:
      out.append(number, std::to_chars(number, number + sizeof(number),
                                       node.as_integer())
                                 .ptr -
                             number);
      return;
    case json_kind::unsigned_integer:
      out.append(number,
                 std::to_chars(number, number + sizeof(number), node.data).ptr -
                     number);
      return;
    case json_kind::real:
      if (!std::isfinite(node.as_real())) {
        throw std::invalid_argument("JSON number must be finite");
      }
      out.append(number, detail::to_chars(number, node.as_real()) - number);
      return;
    case json_kind::string:
      write_json_string(out, storage.string(node));
      return;
    case json_kind::array:
    case json_kind::object:
      break;
  }

  if (depth == json_max_depth) {
    throw std::length_error("JSON nesting exceeds 512 containers");
  }
  const bool object = node.kind == json_kind::object;
  out.push_back(object ? '{' : '[');
  for (json_index i = 0; i < node.size; ++i) {
    if (i != 0) {
      out.push_back(',');
    }
    if (indent >= 0) {
      out.push_back('\n');
      out.append((depth + 1) * static_cast<size_t>(indent), ' ');
    }
    auto child = storage.child(node, i);
    if (object) {
      write_json_string(out, storage.key(child));
      out.push_back(':');
      if (indent >= 0) {
        out.push_back(' ');
      }
    }
    write_json_node(out, storage, child.value, indent, depth + 1);
  }
  if (indent >= 0 && node.size != 0) {
    out.push_back('\n');
    out.append(depth * static_cast<size_t>(indent), ' ');
  }
  out.push_back(object ? '}' : ']');
}

}  // namespace detail

// A view is valid while its document lives and the viewed subtree is retained.
class json_view {
  using kind = detail::json_kind;

 public:
  json_view() : storage_(&detail::null_json_storage()), index_(0) {}

  bool is_missing() const noexcept {
    return read_index() == detail::no_json_index;
  }
  bool is_null() const noexcept {
    auto index = read_index();
    return index != detail::no_json_index &&
           storage_->nodes[index].kind == kind::null;
  }
  bool is_boolean() const noexcept { return node().kind == kind::boolean; }
  bool is_bool() const noexcept { return is_boolean(); }
  bool is_integer() const noexcept {
    auto type = node().kind;
    return type == kind::integer || type == kind::unsigned_integer;
  }
  bool is_double() const noexcept { return node().kind == kind::real; }
  bool is_number() const noexcept {
    return detail::is_json_number(node().kind);
  }
  bool is_string() const noexcept { return node().kind == kind::string; }
  bool is_array() const noexcept { return node().kind == kind::array; }
  bool is_object() const noexcept { return node().kind == kind::object; }

  size_t size() const noexcept {
    auto index = read_index();
    if (index == detail::no_json_index) {
      return 0;
    }
    auto value = storage_->nodes[index];
    if (value.kind == kind::array || value.kind == kind::object) {
      return value.size;
    }
    return value.kind == kind::null ? 0 : 1;
  }
  bool empty() const noexcept { return size() == 0; }

  std::optional<json_view> find(std::string_view key) const {
    auto index = read_index();
    if (index == detail::no_json_index) {
      return std::nullopt;
    }
    auto found = storage_->find(index, key);
    if (found == detail::no_json_index) {
      return std::nullopt;
    }
    return json_view(*storage_, found);
  }

  std::optional<json_view> find(size_t index) const {
    const auto& array = node();
    if (array.kind != kind::array || index >= array.size) {
      return std::nullopt;
    }
    return json_view(
        *storage_,
        storage_->child(array, static_cast<detail::json_index>(index)).value);
  }
  bool contains(std::string_view key) const { return find(key).has_value(); }
  bool contains(size_t index) const { return find(index).has_value(); }

  json_view at(std::string_view key) const {
    auto index = require_index();
    if (storage_->nodes[index].kind != kind::object) {
      throw std::invalid_argument("JSON value is not an object");
    }
    auto found = storage_->find(index, key);
    if (found != detail::no_json_index) {
      return json_view(*storage_, found);
    }
    throw std::out_of_range("JSON object key not found: " + std::string(key));
  }
  json_view at(size_t index) const {
    const auto& array = storage_->nodes[require_index()];
    if (array.kind != kind::array) {
      throw std::invalid_argument("JSON value is not an array");
    }
    if (index >= array.size) {
      throw std::out_of_range("JSON array index out of range");
    }
    return json_view(
        *storage_,
        storage_->child(array, static_cast<detail::json_index>(index)).value);
  }
  json_view operator[](std::string_view key) const { return at(key); }
  template <typename Char, detail::json_char_key<Char> = 0>
  json_view operator[](Char* key) const {
    return at(std::string_view(key));
  }
  json_view operator[](size_t index) const { return at(index); }

  template <typename T>
  std::optional<T> try_get() const {
    return try_get_at<T>(read_index());
  }

  template <typename T>
  T get() const {
    if (auto result = try_get_at<T>(require_index())) {
      return std::move(*result);
    }
    throw std::invalid_argument(
        "JSON type mismatch or numeric value out of range");
  }

 private:
  template <typename T>
  std::optional<T> try_get_at(detail::json_index index) const {
    if (index == detail::no_json_index) {
      return std::nullopt;
    }
    const auto& node = storage_->nodes[index];
    using value_type = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<value_type, bool>) {
      if (node.kind == kind::boolean) {
        return node.data != 0;
      }
    }
    else if constexpr (std::is_integral_v<value_type>) {
      if (node.kind == kind::integer) {
        return detail::json_integer_cast<T>(node.as_integer());
      }
      if (node.kind == kind::unsigned_integer) {
        return detail::json_integer_cast<T>(node.data);
      }
    }
    else if constexpr (std::is_floating_point_v<value_type>) {
      if (node.kind == kind::integer) {
        return static_cast<T>(node.as_integer());
      }
      if (node.kind == kind::unsigned_integer) {
        return static_cast<T>(node.data);
      }
      if (node.kind == kind::real) {
        const auto value = node.as_real();
        if constexpr (std::is_same_v<value_type, float>) {
          if (std::abs(value) > std::numeric_limits<float>::max()) {
            return std::nullopt;
          }
        }
        return static_cast<T>(value);
      }
    }
    else if constexpr (std::is_same_v<value_type, std::string> ||
                       std::is_same_v<value_type, std::string_view>) {
      if (node.kind == kind::string) {
        return T(storage_->string(node));
      }
    }
    else if constexpr (std::is_same_v<value_type, std::nullptr_t>) {
      if (node.kind == kind::null) {
        return nullptr;
      }
    }
    else if constexpr (detail::json_container_v<value_type>) {
      static_assert(
          detail::json_readable<value_type>(),
          "JSON container conversions require owning scalar elements");
      using container = detail::json_container<value_type>;
      if (node.kind != (container::is_array ? kind::array : kind::object)) {
        return std::nullopt;
      }
      T result;
      if constexpr (container::is_array) {
        result.reserve(node.size);
      }
      for (detail::json_index i = 0; i < node.size; ++i) {
        auto child = storage_->child(node, i);
        auto value = try_get_at<typename container::element>(child.value);
        if (!value) {
          return std::nullopt;
        }
        if constexpr (container::is_array) {
          result.push_back(std::move(*value));
        }
        else {
          result.emplace(storage_->key(child), std::move(*value));
        }
      }
      return result;
    }
    else {
      static_assert(!sizeof(T),
                    "get<T>() requires a supported JSON value type");
    }
    return std::nullopt;
  }

 public:
  template <typename T, std::enable_if_t<detail::json_readable<T>(), int> = 0>
  operator T() const {
    return get<T>();
  }

  template <typename T, typename Key, typename... Keys>
  std::optional<T> try_get(Key key, Keys... keys) const {
    if (auto child = find(key)) {
      return child->template try_get<T>(keys...);
    }
    return std::nullopt;
  }

  template <typename T>
  T value(std::string_view key, T fallback) const {
    if (auto member = find(key)) {
      return member->template get<T>();
    }
    return fallback;
  }
  std::string value(std::string_view key, const char* fallback) const {
    return value(key, std::string(fallback));
  }

  json_range<true, false> items() const;
  json_range<false, false> elements() const;
  auto begin() const;
  auto end() const;

  std::string dump(int indent = -1) const {
    std::string output;
    detail::write_json_node(output, *storage_, require_index(), indent, 0);
    return output;
  }

  friend std::ostream& operator<<(std::ostream& stream,
                                  const json_view& value) {
    return stream << value.dump();
  }

 protected:
  json_view(const detail::json_storage& storage, detail::json_index index,
            std::shared_ptr<const detail::json_member_path> path = {})
      : storage_(&storage), index_(index), path_(std::move(path)) {}

  detail::json_index read_index() const noexcept {
    return path_ ? path_->find(*storage_, index_) : index_;
  }

  detail::json_index require_index() const {
    auto index = read_index();
    if (index == detail::no_json_index) {
      throw std::out_of_range("JSON object key not found: " + path_->key);
    }
    return index;
  }

  const detail::json_node& node() const noexcept {
    static constexpr detail::json_node missing{};
    auto index = read_index();
    return index == detail::no_json_index ? missing : storage_->nodes[index];
  }

  const detail::json_storage* storage_;
  detail::json_index index_;
  std::shared_ptr<const detail::json_member_path> path_;

  friend class json;
  friend class json_parser;
  friend class json_ref;
  friend struct detail::json_comparison;
  template <bool Object, bool Mutable, bool Direct>
  friend class json_range;
};

class json_ref : public json_view {
  using kind = detail::json_kind;

 public:
  json_ref(const json_ref&) = default;
  json_ref& operator=(const json_ref& value) {
    return *this = json_view(value);
  }
  json_ref& operator=(const json& value);
  json_ref& operator=(std::initializer_list<json> values);
  json_ref& operator=(json_view value) {
    auto source = value.require_index();
    if (storage_ == value.storage_ && read_index() == source) {
      return *this;
    }
    mutable_node() = storage().clone(*value.storage_, source);
    return *this;
  }
  json_ref& operator=(std::nullptr_t) {
    mutable_node() = {};
    return *this;
  }
  json_ref& operator=(bool value) {
    mutable_node() = {static_cast<uint64_t>(value), 0, kind::boolean};
    return *this;
  }
  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                             int> = 0>
  json_ref& operator=(T value) {
    if constexpr (std::is_signed_v<T>) {
      mutable_node() = detail::json_node::integer(value);
    }
    else if (value <=
             static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
      mutable_node() = detail::json_node::integer(static_cast<int64_t>(value));
    }
    else {
      mutable_node() = {static_cast<uint64_t>(value), 0,
                        kind::unsigned_integer};
    }
    return *this;
  }
  template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
  json_ref& operator=(T value) {
    auto number = static_cast<double>(value);
    if (!std::isfinite(number)) {
      throw std::invalid_argument("JSON number must be finite");
    }
    mutable_node() = detail::json_node::real(number);
    return *this;
  }
  json_ref& operator=(std::string_view value) {
    mutable_node() = storage().string_node(value);
    return *this;
  }
  json_ref& operator=(const char* value) {
    return value ? (*this = std::string_view(value)) : (*this = nullptr);
  }
  json_ref& operator=(const std::string& value) {
    return *this = std::string_view(value);
  }
  template <typename T, std::enable_if_t<detail::json_container_v<T>, int> = 0>
  json_ref& operator=(const T& value) {
    constexpr auto type =
        detail::json_container<T>::is_array ? kind::array : kind::object;
    auto staging = storage().add({0, 0, type});
    json_ref output(storage(), staging);
    if constexpr (detail::json_container<T>::is_array) {
      for (const auto& element : value) {
        output.push_back(element);
      }
    }
    else {
      for (const auto& entry : value) {
        output.insert_member(entry.first) = entry.second;
      }
    }
    mutable_node() = storage().nodes[staging];
    return *this;
  }

  using json_view::at;
  using json_view::operator[];
  using json_view::elements;
  using json_view::find;
  using json_view::items;

  std::optional<json_ref> find(std::string_view key) {
    if (auto found = json_view::find(key)) {
      return json_ref(storage(), found->index_);
    }
    return std::nullopt;
  }
  std::optional<json_ref> find(size_t index) {
    if (auto found = json_view::find(index)) {
      return json_ref(storage(), found->index_);
    }
    return std::nullopt;
  }

  json_ref at(std::string_view key) {
    return json_ref(storage(), json_view::at(key).index_);
  }
  json_ref at(size_t index) {
    return json_ref(storage(), json_view::at(index).index_);
  }
  json_ref operator[](std::string_view key) {
    auto index = read_index();
    if (index != detail::no_json_index) {
      auto type = storage().nodes[index].kind;
      if (type != kind::null && type != kind::object) {
        throw std::invalid_argument("JSON value is not an object");
      }
      auto found = storage().find(index, key);
      if (found != detail::no_json_index) {
        return json_ref(storage(), found);
      }
    }
    auto depth = index == detail::no_json_index ? path_->depth + 1 : 1;
    if (depth > detail::json_max_depth) {
      throw std::length_error("JSON nesting exceeds 512 containers");
    }
    auto path = std::make_shared<const detail::json_member_path>(
        detail::json_member_path{
            index == detail::no_json_index ? path_ : nullptr, std::string(key),
            depth});
    return json_ref(storage(), index == detail::no_json_index ? index_ : index,
                    std::move(path));
  }
  template <typename Char, detail::json_char_key<Char> = 0>
  json_ref operator[](Char* key) {
    return (*this)[std::string_view(key)];
  }
  json_ref operator[](size_t index) { return at(index); }

  template <typename T>
  void push_back(T&& value) {
    if (!is_missing() && !is_null() && !is_array()) {
      throw std::invalid_argument("JSON value is not an array");
    }
    auto index = storage().add();
    json_ref element(storage(), index);
    element = std::forward<T>(value);
    auto parent = write_index();
    auto& array = storage().nodes[parent];
    if (array.kind == kind::null) {
      array.kind = kind::array;
    }
    storage().insert_child(parent, array.size) = {detail::no_json_index, index};
  }

  void push_back(std::initializer_list<json> values);

  bool erase(std::string_view key) {
    if (is_missing()) {
      return false;
    }
    if (!is_object()) {
      throw std::invalid_argument("JSON value is not an object");
    }
    auto position = storage().find_position(node(), key);
    if (position == detail::no_json_index) {
      return false;
    }
    storage().erase_child(read_index(), position);
    return true;
  }

  void erase(size_t position) {
    at(position);
    storage().erase_child(read_index(),
                          static_cast<detail::json_index>(position));
  }

  json_range<true, true> items();
  json_range<false, true> elements();
  auto begin();
  auto end();
  using json_view::begin;
  using json_view::end;

 private:
  json_ref(detail::json_storage& storage, detail::json_index index,
           std::shared_ptr<const detail::json_member_path> path = {})
      : json_view(storage, index, std::move(path)) {}

  detail::json_storage& storage() noexcept {
    return const_cast<detail::json_storage&>(*storage_);
  }
  detail::json_index write_index() {
    if (path_) {
      index_ = path_->insert(storage(), index_);
      path_.reset();
    }
    return index_;
  }

  detail::json_node& mutable_node() { return storage().nodes[write_index()]; }

  // Used by builders that already know this handle refers to an object.
  json_ref insert_member(std::string_view key) {
    return json_ref(storage(), storage().insert(index_, key));
  }

  friend class json;
  template <bool Object, bool Mutable, bool Direct>
  friend class json_range;
};

namespace detail {

// Keep the node handle interface on direct iteration and expose the key when
// unpacked.
template <typename Ref>
class json_iteration_value : public Ref {
 public:
  json_iteration_value(Ref value, json_index key) : Ref(value), key_(key) {}
  json_iteration_value(const json_iteration_value&) = default;
  using Ref::operator=;

  json_iteration_value& operator=(const json_iteration_value& other) {
    Ref::operator=(other);
    if constexpr (std::is_same_v<Ref, json_view>) {
      key_ = other.key_;
    }
    return *this;
  }

  template <size_t I>
  friend auto get(const json_iteration_value& entry) {
    static_assert(I < 2, "JSON object entries contain a key and a value");
    if constexpr (I == 0) {
      if (entry.key_ == no_json_index) {
        throw std::invalid_argument(
            "JSON array elements do not have object keys");
      }
      return entry.storage_->string(entry.storage_->nodes[entry.key_]);
    }
    else {
      return static_cast<const Ref&>(entry);
    }
  }

 private:
  json_index key_;
};

}  // namespace detail

template <bool Object, bool Mutable, bool Direct>
class json_range {
  using storage_type = std::conditional_t<Mutable, detail::json_storage,
                                          const detail::json_storage>;
  using ref_type = std::conditional_t<Mutable, json_ref, json_view>;

 public:
  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<
        Direct, detail::json_iteration_value<ref_type>,
        std::conditional_t<Object, std::pair<std::string_view, ref_type>,
                           ref_type>>;
    using reference = value_type;
    using pointer = void;

    iterator() = default;
    reference operator*() const {
      auto child = storage_->child(node_, position_);
      if constexpr (Direct) {
        return {ref_type(*storage_, child.value), child.key};
      }
      else if constexpr (Object) {
        return {storage_->key(child), ref_type(*storage_, child.value)};
      }
      else {
        return ref_type(*storage_, child.value);
      }
    }
    iterator& operator++() {
      ++position_;
      return *this;
    }
    iterator operator++(int) {
      auto previous = *this;
      ++*this;
      return previous;
    }
    friend bool operator==(iterator lhs, iterator rhs) {
      return lhs.storage_ == rhs.storage_ && lhs.node_.data == rhs.node_.data &&
             lhs.position_ == rhs.position_;
    }
    friend bool operator!=(iterator lhs, iterator rhs) { return !(lhs == rhs); }

   private:
    iterator(storage_type* storage, detail::json_node node,
             detail::json_index position)
        : storage_(storage), node_(node), position_(position) {}
    storage_type* storage_ = nullptr;
    detail::json_node node_;
    detail::json_index position_ = 0;
    friend class json_range;
  };

  iterator begin() const noexcept { return {storage_, node_, 0}; }
  iterator end() const noexcept { return {storage_, node_, node_.size}; }
  size_t size() const noexcept { return node_.size; }
  bool empty() const noexcept { return size() == 0; }

 private:
  json_range(storage_type& storage, detail::json_node node)
      : storage_(&storage), node_(node) {
    if constexpr (Direct) {
      if (node.kind != detail::json_kind::array &&
          node.kind != detail::json_kind::object) {
        throw std::invalid_argument("JSON value is not an array or object");
      }
    }
    else if (node.kind !=
             (Object ? detail::json_kind::object : detail::json_kind::array)) {
      throw std::invalid_argument(Object ? "JSON value is not an object"
                                         : "JSON value is not an array");
    }
  }

  storage_type* storage_;
  detail::json_node node_;
  friend class json_ref;
  friend class json_view;
};

inline json_range<true, false> json_view::items() const {
  return {*storage_, node()};
}
inline json_range<false, false> json_view::elements() const {
  return {*storage_, node()};
}
inline auto json_view::begin() const {
  return json_range<false, false, true>{*storage_, node()}.begin();
}
inline auto json_view::end() const {
  return json_range<false, false, true>{*storage_, node()}.end();
}
inline json_range<true, true> json_ref::items() { return {storage(), node()}; }
inline json_range<false, true> json_ref::elements() {
  return {storage(), node()};
}
inline auto json_ref::begin() {
  return json_range<false, true, true>{storage(), node()}.begin();
}
inline auto json_ref::end() {
  return json_range<false, true, true>{storage(), node()}.end();
}

// Parsing again invalidates views into the previous result.
class json_parser {
 public:
  json_view parse(std::string_view input) & {
    if (!state_) {
      state_ = std::make_unique<state>();
    }
    auto& storage = state_->storage;
    std::string input_copy;
    if (!input.empty() && !storage.strings.empty()) {
      const char* first = storage.strings.data();
      std::less<const char*> less;
      if (!less(input.data(), first) &&
          less(input.data(), first + storage.strings.size())) {
        input_copy.assign(input);
        input = input_copy;
      }
    }
    auto index = state_->parser.parse(input);
    return json_view(storage, index);
  }

  json_view parse(std::string_view input, std::error_code& ec) & {
    try {
      auto result = parse(input);
      ec.clear();
      return result;
    } catch (const json_parse_error& error) {
      ec = error.code();
      return {};
    }
  }

  std::optional<json_view> try_parse(std::string_view input) & {
    std::error_code ec;
    auto result = parse(input, ec);
    if (ec) {
      return std::nullopt;
    }
    return result;
  }

 private:
  struct state {
    detail::json_storage storage;
    detail::json_parser parser{storage};
  };

  std::unique_ptr<state> state_;
};

class json {
  using kind = detail::json_kind;

 public:
  json() = default;
  json(std::nullptr_t) {}
  json(json_view value) { root() = value; }
  json(const char* value) { root() = value; }
  json(std::string_view value) { root() = value; }
  json(const std::string& value) : json(std::string_view(value)) {}

  template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
  json(T value) {
    root() = value;
  }
  template <typename T, std::enable_if_t<detail::json_container_v<T>, int> = 0>
  json(const T& value) {
    root() = value;
  }

  json(const json& other) : json(other.view()) {}
  json(json&& other) noexcept = default;
  json& operator=(json&& other) noexcept = default;
  json& operator=(const json& other) {
    if (this != &other) {
      *this = json(other);
    }
    return *this;
  }

  json(std::initializer_list<json> values) {
    auto ref = root();
    if (std::all_of(values.begin(), values.end(), [](const json& value) {
          return value.is_array() && value.size() == 2 && value[0].is_string();
        })) {
      ref.mutable_node().kind = kind::object;
      for (const auto& element : values) {
        ref.insert_member(element[0].get<std::string_view>()) = element[1];
      }
    }
    else {
      ref.mutable_node().kind = kind::array;
      for (const auto& element : values) {
        ref.push_back(element);
      }
    }
  }

  static json object(
      std::initializer_list<std::pair<std::string_view, json>> values = {}) {
    json result;
    auto ref = result.root();
    ref.mutable_node().kind = kind::object;
    for (const auto& entry : values) {
      ref.insert_member(entry.first) = entry.second;
    }
    return result;
  }

  static json array(std::initializer_list<json> values = {}) {
    json result;
    auto ref = result.root();
    ref.mutable_node().kind = kind::array;
    for (const auto& element : values) {
      ref.push_back(element);
    }
    return result;
  }

  static json parse(std::string_view input) {
    json result;
    result.storage_ = std::make_unique<detail::json_storage>();
    result.index_ = detail::json_parser(*result.storage_, input).parse();
    return result;
  }

  static json parse(std::string_view input, std::error_code& ec) {
    try {
      auto result = parse(input);
      ec.clear();
      return result;
    } catch (const json_parse_error& error) {
      ec = error.code();
      return {};
    }
  }

  static std::optional<json> try_parse(std::string_view input) {
    std::error_code ec;
    auto result = parse(input, ec);
    if (ec) {
      return std::nullopt;
    }
    return result;
  }

  json_view view() const& {
    return storage_ ? json_view(*storage_, index_) : json_view{};
  }
  json_view view() const&& = delete;
  operator json_view() const& { return view(); }
  operator json_view() const&& = delete;

  json_ref operator[](std::string_view key) & { return root()[key]; }
  json_view operator[](std::string_view key) const& { return view()[key]; }
  json operator[](std::string_view key) && { return std::move(*this).at(key); }
  json operator[](std::string_view key) const&& { return json(view()[key]); }
  template <typename Char, detail::json_char_key<Char> = 0>
  json_ref operator[](Char* key) & {
    return root()[key];
  }
  template <typename Char, detail::json_char_key<Char> = 0>
  json_view operator[](Char* key) const& {
    return view()[key];
  }
  template <typename Char, detail::json_char_key<Char> = 0>
  json operator[](Char* key) && {
    return std::move(*this).at(std::string_view(key));
  }
  template <typename Char, detail::json_char_key<Char> = 0>
  json operator[](Char* key) const&& {
    return json(view()[key]);
  }
  json_ref operator[](size_t index) & { return root()[index]; }
  json_view operator[](size_t index) const& { return view()[index]; }
  json operator[](size_t index) && { return std::move(*this).at(index); }
  json operator[](size_t index) const&& { return json(view()[index]); }

  json_ref at(std::string_view key) & { return root().at(key); }
  json_view at(std::string_view key) const& { return view().at(key); }
  json at(std::string_view key) && {
    return std::move(*this).subtree(view().at(key));
  }
  json at(std::string_view key) const&& { return json(view().at(key)); }
  json_ref at(size_t index) & { return root().at(index); }
  json_view at(size_t index) const& { return view().at(index); }
  json at(size_t index) && {
    return std::move(*this).subtree(view().at(index));
  }
  json at(size_t index) const&& { return json(view().at(index)); }

  template <typename T>
  T get() const {
    return view().template get<T>();
  }

  template <typename T,
            std::enable_if_t<detail::json_readable<T>() &&
                                 !std::is_same_v<T, std::string_view>,
                             int> = 0>
  operator T() const {
    return get<T>();
  }

  operator std::string_view() const& { return get<std::string_view>(); }
  operator std::string_view() const&& = delete;

  template <typename T, typename... Keys>
  std::optional<T> try_get(Keys... keys) const {
    return view().template try_get<T>(keys...);
  }

  template <typename T>
  T value(std::string_view key, T fallback) const {
    return view().value(key, std::move(fallback));
  }
  std::string value(std::string_view key, const char* fallback) const {
    return view().value(key, fallback);
  }

  std::optional<json_ref> find(std::string_view key) & {
    if (!storage_) {
      return std::nullopt;
    }
    return root().find(key);
  }
  auto find(std::string_view key) const& { return view().find(key); }
  std::optional<json_view> find(std::string_view key) const&& = delete;
  std::optional<json_ref> find(size_t index) & {
    return storage_ ? root().find(index) : std::nullopt;
  }
  auto find(size_t index) const& { return view().find(index); }
  std::optional<json_view> find(size_t index) const&& = delete;
  bool contains(std::string_view key) const { return view().contains(key); }
  bool contains(size_t index) const { return view().contains(index); }
  bool is_null() const { return view().is_null(); }
  bool is_boolean() const { return view().is_boolean(); }
  bool is_bool() const { return is_boolean(); }
  bool is_integer() const { return view().is_integer(); }
  bool is_double() const { return view().is_double(); }
  bool is_number() const { return view().is_number(); }
  bool is_string() const { return view().is_string(); }
  bool is_array() const { return view().is_array(); }
  bool is_object() const { return view().is_object(); }
  size_t size() const { return view().size(); }
  bool empty() const { return view().empty(); }

  auto items() & { return root().items(); }
  auto items() const& { return view().items(); }
  json_range<true, false> items() const&& = delete;
  auto elements() & { return root().elements(); }
  auto elements() const& { return view().elements(); }
  json_range<false, false> elements() const&& = delete;
  auto begin() & { return root().begin(); }
  auto end() & { return root().end(); }
  auto begin() const& { return view().begin(); }
  auto end() const& { return view().end(); }

  template <typename T>
  void push_back(T&& value) {
    root().push_back(std::forward<T>(value));
  }
  void push_back(std::initializer_list<json> values) {
    root().push_back(values);
  }

  bool erase(std::string_view key) { return root().erase(key); }
  void erase(size_t position) { root().erase(position); }

  // Rebuild from the live subtree and release unused arena storage.
  void compact() { *this = json(view()); }

  std::string dump(int indent = -1) const { return view().dump(indent); }

  friend std::ostream& operator<<(std::ostream& stream, const json& value) {
    return stream << value.dump();
  }

 private:
  json_ref root() {
    if (!storage_) {
      storage_ = std::make_unique<detail::json_storage>();
      index_ = storage_->add();
    }
    return json_ref(*storage_, index_);
  }

  json subtree(json_view child) && {
    index_ = child.index_;
    return std::move(*this);
  }

  std::unique_ptr<detail::json_storage> storage_;
  detail::json_index index_ = 0;
};

inline json_ref& json_ref::operator=(const json& value) {
  return *this = value.view();
}

inline json_ref& json_ref::operator=(std::initializer_list<json> values) {
  return *this = json(values);
}

inline void json_ref::push_back(std::initializer_list<json> values) {
  push_back(json(values));
}

namespace detail {

template <typename T>
inline constexpr bool json_node_operand_v =
    std::is_same_v<T, json> ||
    std::is_convertible_v<const T*, const json_view*>;

template <typename T>
inline constexpr bool json_scalar_operand_v =
    json_scalar_v<std::decay_t<T>> ||
    std::is_same_v<std::decay_t<T>, const char*> ||
    std::is_same_v<std::decay_t<T>, char*>;

template <typename L, typename R>
using json_comparable =
    std::enable_if_t<(json_node_operand_v<L> &&
                      (json_node_operand_v<R> || json_scalar_operand_v<R>)) ||
                         (json_scalar_operand_v<L> && json_node_operand_v<R>),
                     int>;

struct json_comparison {
  static json_operand operand(const json_view& value) {
    return json_operand::from_node(*value.storage_, value.read_index());
  }
  static json_operand operand(const json& value) {
    return operand(value.view());
  }
  static json_operand operand(std::nullptr_t) { return {}; }
  static json_operand operand(bool value) {
    return {{static_cast<uint64_t>(value), 0, json_kind::boolean}, {}};
  }
  static json_operand operand(std::string_view value) {
    return {{0, 0, json_kind::string}, value};
  }
  static json_operand operand(const char* value) {
    return value ? operand(std::string_view(value)) : operand(nullptr);
  }
  static json_operand operand(const std::string& value) {
    return operand(std::string_view(value));
  }
  template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
  static json_operand operand(T value) {
    if constexpr (std::is_floating_point_v<T>) {
      return {{0, 0, json_kind::real}, {}, nullptr, true, value};
    }
    else if constexpr (std::is_signed_v<T>) {
      return {json_node::integer(static_cast<int64_t>(value)), {}};
    }
    else {
      return {{static_cast<uint64_t>(value), 0, json_kind::unsigned_integer},
              {}};
    }
  }
};

template <typename L, typename R>
std::optional<int> json_order(const L& left, const R& right) {
  return order_json_operands(json_comparison::operand(left),
                             json_comparison::operand(right));
}

}  // namespace detail

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator==(const L& left, const R& right) {
  return detail::equal_json_operands(detail::json_comparison::operand(left),
                                     detail::json_comparison::operand(right));
}

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator!=(const L& left, const R& right) {
  return !(left == right);
}

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator<(const L& left, const R& right) {
  auto order = detail::json_order(left, right);
  return order && *order < 0;
}

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator>(const L& left, const R& right) {
  return right < left;
}

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator<=(const L& left, const R& right) {
  auto order = detail::json_order(left, right);
  return order && *order <= 0;
}

template <typename L, typename R, detail::json_comparable<L, R> = 0>
bool operator>=(const L& left, const R& right) {
  return right <= left;
}

inline json parse(std::string_view input) { return json::parse(input); }

inline void parse(json& result, std::string_view input) {
  result = json::parse(input);
}

inline void parse(json& result, std::string_view input, std::error_code& ec) {
  auto parsed = json::parse(input, ec);
  if (!ec) {
    result = std::move(parsed);
  }
}

}  // namespace iguana

namespace std {

template <typename Ref>
struct tuple_size<iguana::detail::json_iteration_value<Ref>>
    : integral_constant<size_t, 2> {};

template <size_t I, typename Ref>
struct tuple_element<I, iguana::detail::json_iteration_value<Ref>> {
  static_assert(I < 2, "JSON object entries contain a key and a value");
  using type = conditional_t<I == 0, string_view, Ref>;
};

}  // namespace std
