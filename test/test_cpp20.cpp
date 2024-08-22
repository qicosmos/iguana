#define DOCTEST_CONFIG_IMPLEMENT
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
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP