#include "iguana/reflection.hpp"
#include <stdexcept>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
#include <iostream>

struct person_t {
  std::string name;
  int age;
};
REFLECTION(person_t, name, age);

TEST_CASE("test known fields") {
  std::string str = R"({"dummy": 0, "name":"tom", "age":20})";

  person_t p;
  CHECK(iguana::from_json(p, str.begin(), str.end()) ==
        iguana::errc::unknown_key);

  std::string str1 = R"({"name":"tom", "age":20, })";
  CHECK(iguana::from_json(p, str1.begin(), str1.end()) ==
        iguana::errc::not_match_specific_chars);
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP