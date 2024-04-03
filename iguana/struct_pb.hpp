#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>

#include "reflection.hpp"
namespace iguana {
namespace detail {}  // namespace detail

template <typename T>
inline void from_pb(T& value, std::string_view pb_str) {}

template <typename T>
inline void to_pb(T& value, std::string& out) {}
}  // namespace iguana