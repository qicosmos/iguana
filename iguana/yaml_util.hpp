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

template <class T>
concept char_t = std::same_as < std::decay_t<T>,
char > || std::same_as<std::decay_t<T>, char16_t> ||
    std::same_as<std::decay_t<T>, char32_t> ||
    std::same_as<std::decay_t<T>, wchar_t>;

template <class T>
concept bool_t = std::same_as < std::decay_t<T>,
bool > || std::same_as<std::decay_t<T>, std::vector<bool>::reference>;

template <class T>
concept int_t =
    std::integral<std::decay_t<T>> && !char_t<std::decay_t<T>> && !bool_t<T>;

template <class T>
concept float_t = std::floating_point<std::decay_t<T>>;

template <class T>
concept num_t = std::floating_point<std::decay_t<T>> || int_t<T>;

template <class T>
concept enum_t = std::is_enum_v<std::decay_t<T>>;

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

// TODO: add others
template <class T>
concept plain_t = string_t<T> || num_t<T>;

IGUANA_INLINE void skip_yaml_comment(auto &&it, auto &&end) {
  while (++it != end && *it != '\n')
    ;
}

IGUANA_INLINE void skip_yaml_space(auto &&it, auto &&end) {
  while (it != end && *it == ' ')
    ++it;
}

// match c and skip
template <char c> IGUANA_INLINE void match(auto &&it, auto &&end) {
  if (it == end || *it != c) [[unlikely]] {
    static constexpr char b[] = {c, '\0'};
    std::string error = std::string("Expected:").append(b);
    throw std::runtime_error(error);
  } else [[likely]] {
    ++it;
  }
}

IGUANA_INLINE void skip_space(auto &&it, auto &&end) {
  while (it != end && *it == ' ')
    ++it;
}

// If there are '\n' ,return indentation
// If not, return minspaces + space 
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
  return res; // throw in certain situations ?
}

// (Throw == false) means allow (it == end)
// when C == '\n' ,we should never skip '\n'  except it == end
template <bool Throw, char... C> IGUANA_INLINE auto skip_till(auto &&it, auto &&end) {
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
      static constexpr char b[] = {C..., '\0'};
      std::string error = std::string("Expected one of these: ").append(b);
      throw std::runtime_error(error);   
    } else { 
      return (*(it - 1) == ' ') ? res : end;
    }
  }
  // true : when C is not '\n'
  if (!((... || ('\n' == C)) && (sizeof...(C) == 1))) {
    ++it;
    return (*(it - 2) == ' ') ? res : it - 1;
  }    
  return (*(it - 1) == ' ') ? res : it;
}
} // namespace iguana
