
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json.hpp>
#include <iguana/json_util.hpp>
#include <iostream>

TEST_CASE("test cooke book number") {
  MyClass1 cls1;
  iguana::from_json(cls1, "../test_data/cookbook_numbers1.json");
  CHECK(cls1.member0 == 1.23);
  CHECK(cls1.member1 == 1);
  CHECK(cls1.member2 == 1200);
  CHECK(cls1.member3 == 3000);

  MyClass2 cls2;
  iguana::from_json(cls2, "../test_data/cookbook_numbers2.json");
  CHECK(cls2.member_signed == -12345);
  CHECK(cls2.member_unsigned0 == 12345);
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP