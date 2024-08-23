#define DOCTEST_CONFIG_IMPLEMENT
// #define SEQUENTIAL_PARSE
#include "doctest.h"
#include "iguana/iguana.hpp"

using namespace ylt::reflection;

struct point_t {
  int x;
  int y;
};

#if __cplusplus < 202002L
YLT_REFL(point_t, x, y);
#endif

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