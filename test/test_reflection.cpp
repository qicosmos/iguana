#include "iguana/ylt/reflection/field_names.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)

using namespace ylt::reflection;

struct sub {
  int id;
};

enum class Color { red, black };

struct person {
  Color color;
  int id;
  sub s;
  std::string str;
  int arr[2];
};

TEST_CASE("test field names") {
  constexpr size_t size = members_count_v<person>;
  CHECK(size == 5);
  constexpr auto tp = internal::bind_fake_object_to_tuple<person>();
  constexpr size_t tp_size = std::tuple_size_v<decltype(tp)>;
  CHECK(tp_size == 5);

  constexpr auto arr = get_field_names<person>();
  for (auto name : arr) {
    std::cout << name << ", ";
  }
  std::cout << "\n";
  CHECK(arr ==
        std::array<std::string_view, size>{"color", "id", "s", "str", "arr"});
}

#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
