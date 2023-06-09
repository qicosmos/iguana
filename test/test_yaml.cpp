#include <deque>
#include <iterator>
#include <list>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <iostream>
#include <optional>

enum class stat {
  start,
  stop,
};
struct plain_type_t {
  bool isok;
  stat status;
  char c;
  std::optional<bool> hasprice;
  std::optional<int> price;
};
REFLECTION(plain_type_t, isok, status, c, hasprice, price);
TEST_CASE("test plain_type") {
  plain_type_t p{false, stat::stop, 'a', true};
  std::string ss;
  iguana::to_yaml(p, ss);
  std::cout << ss << "\n";
  plain_type_t p1;
  iguana::from_yaml(p1, ss);
  CHECK(p1.isok == p.isok);
  CHECK(p1.status == p.status);
  CHECK(p1.c == p.c);
  CHECK(p1.hasprice == p.hasprice);
  CHECK(!p1.price);
}

struct arr_t {
  std::vector<std::string_view> arr;
};
REFLECTION(arr_t, arr);
TEST_CASE("test array") {
  std::string str = R"(
  arr : [
      a,
      
      b] 
  )";
  auto validator = [](const arr_t &a) {
    CHECK(a.arr[0] == std::string_view("a"));
    CHECK(a.arr[1] == std::string_view("b"));
  };
  arr_t a;
  iguana::from_yaml(a, str);
  validator(a);

  std::string str1 = R"(
  arr: 
    - a
    - b
  )";
  arr_t a1;
  iguana::from_yaml(a1, str1);
  validator(a1);

  std::string ss;
  iguana::to_yaml(a1, ss);
  arr_t a2;
  iguana::from_yaml(a2, ss);
  validator(a2);
}

struct nest_arr_t {
  std::vector<std::vector<std::string_view>> arr;
};
REFLECTION(nest_arr_t, arr);
TEST_CASE("test nest arr ") {
  std::string str = R"(
  arr : [[
      a,
      b],
   [c, d]]
  )";
  auto validator = [](const nest_arr_t &a) {
    CHECK(a.arr[0][0] == std::string_view("a"));
    CHECK(a.arr[0][1] == std::string_view("b"));
    CHECK(a.arr[1][0] == std::string_view("c"));
    CHECK(a.arr[1][1] == std::string_view("d"));
  };
  nest_arr_t a;
  iguana::from_yaml(a, str);
  validator(a);
  std::string str1 = R"(
  arr :
    - - a
       - b
    - - c
      - d
  )";
  nest_arr_t a1;
  iguana::from_yaml(a1, str1);
  validator(a1);
  std::string str2 = R"(
  arr :
    - [a, b]
    - [c, d]
  )";
  nest_arr_t a2;
  iguana::from_yaml(a2, str2);
  validator(a2);
  std::string ss;
  iguana::to_yaml(a2, ss);
  nest_arr_t a3;
  iguana::from_yaml(a3, ss);
  validator(a3);
}

struct nest_float_arr_t {
  std::vector<std::vector<float>> arr;
};
REFLECTION(nest_float_arr_t, arr);
TEST_CASE("test arr with float") {
  std::string str = R"(
  arr :
    - - 28.5
       - 56.7
    - - 123.4
      - -324.9)";
  nest_float_arr_t a;
  iguana::from_yaml(a, str);
  CHECK(a.arr[0][0] == 28.5f);
  CHECK(a.arr[0][1] == 56.7f);
  CHECK(a.arr[1][0] == 123.4f);
  CHECK(a.arr[1][1] == -324.9f);
  std::cout << a.arr[1][1] << std::endl;
}

struct map_t {
  std::unordered_map<std::string_view, std::string_view> map;
};
REFLECTION(map_t, map);
TEST_CASE("test map") {
  std::string str = R"(
  map : {
      k1 : v1,
   k2 : v2}
  )";
  map_t m;
  iguana::from_yaml(m, str);
  CHECK(m.map["k1"] == "v1");
  CHECK(m.map["k2"] == "v2");
  std::string str1 = R"(
  map:
    k1: v1
    k2: v2
)";
  map_t m1;
  iguana::from_yaml(m1, str1);
  CHECK(m1.map["k1"] == "v1");
  CHECK(m1.map["k2"] == "v2");
}

struct map_arr_t {
  std::unordered_map<std::string_view, std::vector<std::string_view>> map;
};
REFLECTION(map_arr_t, map);
TEST_CASE("test map") {
  std::string str = R"(
  map : {
      k1 : [a , b],
   k2 : [c, d], }
  )";
  auto validator = [](map_arr_t &m) {
    CHECK(m.map["k1"][0] == "a");
    CHECK(m.map["k1"][1] == "b");
    CHECK(m.map["k2"][0] == "c");
    CHECK(m.map["k2"][1] == "d");
  };
  map_arr_t m;
  iguana::from_yaml(m, str);
  validator(m);
  std::string str1 = R"(
  map:
    k1:
      - a
      - b
    k2:
      - c
      - d)";
  map_arr_t m1;
  iguana::from_yaml(m1, str1);
  validator(m1);
  std::string ss;
  iguana::to_yaml(m1, ss);
  map_arr_t m2;
  iguana::from_yaml(m2, ss);
  validator(m2);
}

// | > >- ""  ''
struct test_str_t {
  std::string_view str;
};
REFLECTION(test_str_t, str);
TEST_CASE("test str type") {
  std::string str = R"(
  str :
      hello world
  )";
  test_str_t s;
  iguana::from_yaml(s, str);
  CHECK(s.str == "hello world");
  std::string str1 = R"(
  str :
      'hello world')";
  test_str_t s1;
  iguana::from_yaml(s1, str1);
  CHECK(s1.str == "hello world");
  std::string ss;
  iguana::to_yaml(s1, ss);
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
