#include "iguana/reflection.hpp"
#include <stdexcept>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json.hpp>
#include <iguana/json_util.hpp>
#include <iostream>

struct person_t {
  std::string name;
  int age;
};
REFLECTION(person_t, name, age);

TEST_CASE("test known fields") {
  std::string str = R"({"dummy": 0, "name":"tom", "age":20})";

  person_t p;
  CHECK_THROWS_WITH(iguana::from_json(p, str.begin(), str.end()),
                    "Unknown key: dummy");
  CHECK_THROWS_WITH_AS(iguana::from_json(p, str.begin(), str.end()),
                       "Unknown key: dummy", std::runtime_error);

  std::string str1 = R"({"name":"tom", "age":20, })";

  CHECK_THROWS_WITH(iguana::from_json(p, str1.begin(), str1.end()),
                    "Expected:\"");
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP