#define DOCTEST_CONFIG_IMPLEMENT
// #define SEQUENTIAL_PARSE
#include "doctest.h"
#include "iguana/iguana.hpp"

using namespace ylt::reflection;

struct point_t {
  int x;
  int y;
};

struct test_variant {
  test_variant() = default;
  test_variant(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
#if __cplusplus < 202002L
YLT_REFL(test_variant, x, y, z);
#endif

#if __cplusplus < 202002L
YLT_REFL(point_t, x, y);
#endif

TEST_CASE("test pb") {
  {
    test_variant t(1, "test", 3);
    auto tp = iguana::detail::get_pb_members_tuple(t);
    std::cout << "\n";
    // auto tp = iguana::detail::get_pb_members_tuple(t);
    assert(std::get<0>(tp).field_no == 1);
    // assert(std::get<1>(tp).field_no == 2);
    // assert(std::get<2>(tp).field_no == 3);
    // assert(std::get<3>(tp).field_no == 4);
    // assert(std::get<4>(tp).field_no == 5);
  }

  point_t pt{1, 2};
  std::string str;

  iguana::to_pb(pt, str);
  std::cout << str.size() << "\n";
}

TEST_CASE("test simple") {
  point_t pt{1, 2};
  std::string str;
  static_assert(iguana::ylt_refletable_v<point_t>, "e");
  iguana::to_json(pt, str);
  std::cout << str << "\n";
  auto map = ylt::reflection::get_variant_map(pt);
  for (auto& [key, var] : map) {
    std::cout << key.data() << "\n";
    std::visit(
        [](auto ptr) {
          std::cout << *ptr << "\n";
        },
        var);
  }

  point_t pt1;
  iguana::from_json(pt1, str);
  std::cout << pt1.x << "\n";
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP