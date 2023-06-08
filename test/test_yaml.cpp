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
  arr_t a;
  iguana::from_yaml(a, str);
  CHECK(a.arr[0] == std::string_view("a"));
  CHECK(a.arr[1] == std::string_view("b"));

  std::string str1 = R"(
  arr: 
    - a
    - b
  )";
  arr_t a1;
  iguana::from_yaml(a1, str1);
  CHECK(a1.arr[0] == std::string_view("a"));
  CHECK(a1.arr[1] == std::string_view("b"));

  std::string ss;
  iguana::to_yaml(a1, ss);
  arr_t a2;
  iguana::from_yaml(a2, ss);
  CHECK(a2.arr[0] == std::string_view("a"));
  CHECK(a2.arr[1] == std::string_view("b"));
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
  nest_arr_t a;
  iguana::from_yaml(a, str);
  CHECK(a.arr[0][0] == std::string_view("a"));
  CHECK(a.arr[0][1] == std::string_view("b"));
  CHECK(a.arr[1][0] == std::string_view("c"));
  std::string str1 = R"(
  arr :
    - - a
       - b
    - - c
      - d
  )";
  nest_arr_t a1;
  iguana::from_yaml(a1, str1);
  CHECK(a1.arr[0][0] == std::string_view("a"));
  CHECK(a1.arr[0][1] == std::string_view("b"));
  CHECK(a1.arr[1][0] == std::string_view("c"));
  CHECK(a1.arr[1][1] == std::string_view("d"));
  std::string str2 = R"(
  arr :
    - [a, b]
    - [c, d]
  )";
  nest_arr_t a2;
  iguana::from_yaml(a2, str2);
  CHECK(a2.arr[0][0] == std::string_view("a"));
  CHECK(a2.arr[0][1] == std::string_view("b"));
  CHECK(a2.arr[1][0] == std::string_view("c"));
  CHECK(a2.arr[1][1] == std::string_view("d"));

  std::string ss;
  iguana::to_yaml(a2, ss);
  std::cout << ss << std::endl;
  nest_arr_t a3;
  iguana::from_yaml(a3, ss);
  CHECK(a3.arr[0][0] == std::string_view("a"));
  CHECK(a3.arr[0][1] == std::string_view("b"));
  CHECK(a3.arr[1][0] == std::string_view("c"));
  CHECK(a3.arr[1][1] == std::string_view("d"));
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
  map_arr_t m;
  iguana::from_yaml(m, str);
  CHECK(m.map["k1"][0] == "a");
  CHECK(m.map["k1"][1] == "b");
  CHECK(m.map["k2"][0] == "c");
  CHECK(m.map["k2"][1] == "d");
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
  CHECK(m1.map["k1"][0] == "a");
  CHECK(m1.map["k1"][1] == "b");
  CHECK(m1.map["k2"][0] == "c");
  CHECK(m1.map["k2"][1] == "d");
  std::string ss;
  iguana::to_yaml(m1, ss);
  std::cout << ss << std::endl;
  map_arr_t m2;
  iguana::from_yaml(m2, ss);
  CHECK(m2.map["k1"][0] == "a");
  CHECK(m2.map["k1"][1] == "b");
  CHECK(m2.map["k2"][0] == "c");
  CHECK(m2.map["k2"][1] == "d");
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
  std::cout << ss << std::endl;
  
}


// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
