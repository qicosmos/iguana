#pragma once

#include <optional>

#include "json_dom_storage.hpp"

namespace iguana::detail {

struct json_operand {
  json_node value;
  std::string_view text;
  const json_storage* storage = nullptr;
  bool present = true;
  long double real = 0;

  static json_operand from_node(const json_storage& storage, json_index index) {
    if (index == no_json_index) {
      return {{}, {}, nullptr, false};
    }
    auto node = storage.nodes[index];
    return {node,
            node.kind == json_kind::string ? storage.string(node)
                                           : std::string_view{},
            &storage, true, node.kind == json_kind::real ? node.as_real() : 0};
  }
};

template <typename T>
int compare_json_values(T left, T right) {
  return left < right ? -1 : right < left ? 1 : 0;
}

inline bool is_json_number(json_kind kind) {
  return kind == json_kind::integer || kind == json_kind::unsigned_integer ||
         kind == json_kind::real;
}

template <typename Integer>
int compare_json_integer_to_real(Integer integer, long double real) {
  constexpr bool signed_integer = std::is_signed_v<Integer>;
  constexpr long double limit = signed_integer ? 0x1p63L : 0x1p64L;
  constexpr long double minimum = signed_integer ? -limit : 0;
  if (real >= limit) {
    return -1;
  }
  if (real < minimum) {
    return 1;
  }
  // Compare the exact integer part before checking the fraction. Converting a
  // 64-bit integer to double first would make adjacent large integers equal.
  auto relation = compare_json_values(integer, static_cast<Integer>(real));
  return relation != 0 ? relation : compare_json_values(std::trunc(real), real);
}

inline int compare_json_numbers(const json_operand& lhs,
                                const json_operand& rhs) {
  auto left = lhs.value;
  auto right = rhs.value;
  if (left.kind == json_kind::real) {
    if (right.kind == json_kind::real) {
      return compare_json_values(lhs.real, rhs.real);
    }
    return -compare_json_numbers(rhs, lhs);
  }
  if (right.kind == json_kind::real) {
    return left.kind == json_kind::integer
               ? compare_json_integer_to_real(left.as_integer(), rhs.real)
               : compare_json_integer_to_real(left.data, rhs.real);
  }
  if (left.kind == right.kind) {
    return left.kind == json_kind::integer
               ? compare_json_values(left.as_integer(), right.as_integer())
               : compare_json_values(left.data, right.data);
  }
  if (left.kind == json_kind::unsigned_integer) {
    return -compare_json_numbers(rhs, lhs);
  }
  return left.as_integer() < 0
             ? -1
             : compare_json_values(static_cast<uint64_t>(left.as_integer()),
                                   right.data);
}

inline std::optional<int> order_json_operands(const json_operand& left,
                                              const json_operand& right) {
  if (!left.present || !right.present) {
    return std::nullopt;
  }
  const auto left_kind = left.value.kind;
  const auto right_kind = right.value.kind;
  if (is_json_number(left_kind) && is_json_number(right_kind)) {
    if ((left_kind == json_kind::real && std::isnan(left.real)) ||
        (right_kind == json_kind::real && std::isnan(right.real))) {
      return std::nullopt;
    }
    return compare_json_numbers(left, right);
  }
  if (left_kind == right_kind) {
    switch (left_kind) {
      case json_kind::null:
        return 0;
      case json_kind::boolean:
        return compare_json_values(left.value.data, right.value.data);
      case json_kind::string:
        return compare_json_values(left.text, right.text);
      default:
        break;
    }
  }
  return std::nullopt;
}

inline bool equal_json_operands(const json_operand& left,
                                const json_operand& right, unsigned depth = 0) {
  if (auto relation = order_json_operands(left, right)) {
    return *relation == 0;
  }
  const auto left_kind = left.value.kind;
  if (!left.present || !right.present || left_kind != right.value.kind ||
      (left_kind != json_kind::object && left_kind != json_kind::array) ||
      left.value.size != right.value.size) {
    return false;
  }
  if (depth == json_max_depth) {
    throw std::length_error("JSON nesting exceeds 512 containers");
  }
  for (json_index i = 0; i < left.value.size; ++i) {
    auto child = left.storage->child(left.value, i);
    auto right_index = left_kind == json_kind::object
                           ? right.storage->find_position(
                                 right.value, left.storage->key(child))
                           : i;
    if (right_index == no_json_index ||
        !equal_json_operands(
            json_operand::from_node(*left.storage, child.value),
            json_operand::from_node(
                *right.storage,
                right.storage->child(right.value, right_index).value),
            depth + 1)) {
      return false;
    }
  }
  return true;
}

}  // namespace iguana::detail
