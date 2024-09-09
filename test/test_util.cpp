#include <iguana/ylt/reflection/template_string.hpp>
#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
class message {};

namespace ns {
class message {};
}  // namespace ns

namespace ns::ns2 {
class message {};
}  // namespace ns::ns2

TEST_CASE("test type string") {
  static_assert(iguana::type_string<message>() == "message");

  auto s = iguana::type_string<message>();
  std::cout << s << "\n";
  CHECK(s == "message");

  static_assert(iguana::type_string<ns::message>() == "ns::message");
  auto s1 = iguana::type_string<ns::message>();
  std::cout << s1 << "\n";
  CHECK(s1 == "ns::message");

  static_assert(iguana::type_string<ns::ns2::message>() == "ns::ns2::message");
  auto s2 = iguana::type_string<ns::ns2::message>();
  std::cout << s2 << "\n";
  CHECK(s2 == "ns::ns2::message");
}

enum class Color { red, black };

enum Size { small, large };

namespace ns {
enum class Color { red, black };
enum Size { small, large };
}  // namespace ns

namespace ns::ns2 {
enum class Color { red, black };
enum Size { small, large };
}  // namespace ns::ns2

TEST_CASE("test enum string") {
  static_assert(iguana::enum_string<Color::red>() == "Color::red");
  std::cout << iguana::get_raw_name<int>() << "\n";
  std::cout << iguana::get_raw_name<Color::red>() << "\n";
  auto s = iguana::enum_string<Color::red>();
  std::cout << s << "\n";
  CHECK(s == "Color::red");

  static_assert(iguana::enum_string<ns::Color::red>() == "ns::Color::red");
  auto s1 = iguana::enum_string<ns::Color::red>();
  std::cout << s1 << "\n";
  CHECK(s1 == "ns::Color::red");

  static_assert(iguana::enum_string<ns::ns2::Color::red>() ==
                "ns::ns2::Color::red");
  auto s2 = iguana::enum_string<ns::ns2::Color::red>();
  std::cout << s2 << "\n";
  CHECK(s2 == "ns::ns2::Color::red");

  static_assert(iguana::enum_string<Size::small>() == "small");
  auto s3 = iguana::enum_string<Size::small>();
  std::cout << s3 << "\n";
  CHECK(s3 == "small");

  static_assert(iguana::enum_string<ns::Size::small>() == "ns::small");
  auto s4 = iguana::enum_string<ns::Size::small>();
  std::cout << s4 << "\n";
  CHECK(s4 == "ns::small");

  static_assert(iguana::enum_string<ns::ns2::Size::small>() ==
                "ns::ns2::small");
  auto s5 = iguana::enum_string<ns::ns2::Size::large>();
  std::cout << s5 << "\n";
  CHECK(s5 == "ns::ns2::large");
}

struct sub {
  int id;
};

struct person {
  ns::ns2::Color color;
  int id;
  sub s;
  std::string str;
};

TEST_CASE("test field string") {
  constexpr auto field_name1 = iguana::field_string<&person::color>();
  constexpr auto field_name2 = iguana::field_string<&person::id>();
  constexpr auto field_name3 = iguana::field_string<&person::s>();
  constexpr auto field_name4 = iguana::field_string<&person::str>();
  CHECK(field_name1 == "color");
  CHECK(field_name2 == "id");
  CHECK(field_name3 == "s");
  CHECK(field_name4 == "str");
}
#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP