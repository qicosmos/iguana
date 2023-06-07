#pragma once

#include "define.h"
#include "reflection.hpp"
#include "value.hpp"
#include <bit>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <math.h>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace iguana {

template <typename T> constexpr inline bool is_basic_string_view = false;

template <typename T>
constexpr inline bool is_basic_string_view<std::basic_string_view<T>> = true;

template <typename T>
concept str_view_t = is_basic_string_view<std::remove_reference_t<T>>;

template <class T>
concept str_t =
    std::convertible_to<std::decay_t<T>, std::string_view> && !str_view_t<T>;

template <class T>
concept string_t = std::convertible_to<std::decay_t<T>, std::string_view>;

template <class T>
concept sequence_container_t =
    is_sequence_container<std::remove_cvref_t<T>>::value;

template <typename Type>
concept container = requires(Type container) {
  typename std::remove_cvref_t<Type>::value_type;
  container.size();
  container.begin();
  container.end();
};
template <typename Type>
concept map_container = container<Type> && requires(Type container) {
  typename std::remove_cvref_t<Type>::mapped_type;
};

// 数组 + 字符串 + bool之类的
template <class T>
concept plain_t = string_t<T>;

IGUANA_INLINE void skip_yaml_comment(auto &&it, auto &&end) {
  while (++it != end && *it != '\n')
    ;
}

IGUANA_INLINE void skip_yaml_space(auto &&it, auto &&end) {
  while (it != end && *it == ' ')
    ++it;
}

// 当前肯定是c并且跳过
template <char c> IGUANA_INLINE void match(auto &&it, auto &&end) {
  if (it == end || *it != c) [[unlikely]] {
    static constexpr char b[] = {c, '\0'};
    //         static constexpr auto error = concat_arrays("Expected:", b);
    std::string error = std::string("Expected:").append(b);
    throw std::runtime_error(error);
  } else [[likely]] {
    ++it;
  }
}

// 跳过空格,到达第一个不是空格的
IGUANA_INLINE void skip_space(auto &&it, auto &&end) {
  while (it != end && *it == ' ')
    ++it;
}

// 第一行不需要考虑缩进，到了第二行才考虑
template <bool Throw = true>
IGUANA_INLINE size_t skip_space_and_lines(auto &&it, auto &&end,
                                          size_t minspaces) {
  size_t res = minspaces;
  while (it != end) {
    if (*it == '\n') {
      ++it;
      res = 0;
    } else if (*it == ' ') {
      ++it;
      ++res;
    } else {
      if constexpr (Throw) {
        if (res < minspaces) [[unlikely]] {
          throw std::runtime_error("Indentation problem");
        }
      }
      return res;
    }
  }
  return res; // throw ?
}

// 跳过空格和缩进，考虑第一行
IGUANA_INLINE size_t skip_space_and_lines_for_key(auto &&it, auto &&end) {
  size_t res = 0;
  while (it != end) {
    if (*it == '\n') {
      ++it;
      res = 0;
    } else if (*it == ' ') {
      ++it;
      ++res;
    } else {
      return res;
    }
  }
  return res; // throw ？
}

// 刚好跳过字符 c ,不能有换行符, 返回最后一个字符的下一个位置
template <char... C, bool Throw = true> IGUANA_INLINE auto skip_till(auto &&it, auto &&end) {
  std::decay_t<decltype(it)> res;
  while ((it != end) && (!((... || (*it == C))))) {
    if (*it == '\n') [[unlikely]] {
      throw std::runtime_error("\\n is not expected");
    } else if (*it == ' ') [[unlikely]] {
      res = it;
      while (it != end && *it == ' ')
        ++it;
    } else [[likely]] {
      ++it;
    }
  }
  if (it == end) [[unlikely]] {
    if constexpr (Throw) {
      return (*(it - 1) == ' ') ? res : end;
    } else {
      static constexpr char b[] = {C..., '\0'};
      std::string error = std::string("Expected one of these: ").append(b);
      throw std::runtime_error(error);
    }
  }
  ++it; //  skip c
  return (*(it - 2) == ' ') ? res : it - 1;
}
} // namespace iguana
