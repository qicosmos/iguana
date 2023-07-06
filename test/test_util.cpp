#include <iguana/util.hpp>
#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

class message {};

namespace ns {
class message {};
} // namespace ns

namespace ns::ns2 {
class message {};
} // namespace ns::ns2

TEST_CASE("test type string") {
  static_assert(iguana::type_string<message>() == "message");
  auto s = iguana::type_string<message>();
  assert(s == "message");

  static_assert(iguana::type_string<ns::message>() == "ns::message");
  auto s1 = iguana::type_string<ns::message>();
  assert(s1 == "ns::message");

  static_assert(iguana::type_string<ns::ns2::message>() == "ns::ns2::message");
  auto s2 = iguana::type_string<ns::ns2::message>();
  assert(s2 == "ns::ns2::message");
}

enum class Color { red, black };

enum Size { small, large };

namespace ns {
enum class Color { red, black };
enum Size { small, large };
} // namespace ns

namespace ns::ns2 {
enum class Color { red, black };
enum Size { small, large };
} // namespace ns::ns2

TEST_CASE("test enum string") {
  static_assert(iguana::enum_string<Color::red>() == "Color::red");
  auto s = iguana::enum_string<Color::red>();
  assert(s == "Color::red");

  static_assert(iguana::enum_string<ns::Color::red>() == "ns::Color::red");
  auto s1 = iguana::enum_string<ns::Color::red>();
  assert(s1 == "ns::Color::red");

  static_assert(iguana::enum_string<ns::ns2::Color::red>() ==
                "ns::ns2::Color::red");
  auto s2 = iguana::enum_string<ns::ns2::Color::red>();
  assert(s2 == "ns::ns2::Color::red");

  static_assert(iguana::enum_string<Size::small>() == "small");
  auto s3 = iguana::enum_string<Size::small>();
  assert(s3 == "small");

  static_assert(iguana::enum_string<ns::Size::small>() == "ns::small");
  auto s4 = iguana::enum_string<ns::Size::small>();
  assert(s4 == "ns::small");

  static_assert(iguana::enum_string<ns::ns2::Size::small>() ==
                "ns::ns2::small");
  auto s5 = iguana::enum_string<ns::ns2::Size::large>();
  assert(s5 == "ns::ns2::large");
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP