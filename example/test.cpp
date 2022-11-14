#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include <iguana/json.hpp>
#include <iguana/json_util.hpp>
#include <iostream>
#include <optional>

struct point_t {
  int x;
  double y;
};
REFLECTION(point_t, x, y);

struct person {
  std::string name;
  bool ok;
};
REFLECTION(person, name, ok);

struct bool_t {
  bool ok;
};
REFLECTION(bool_t, ok);

struct optional_t {
  std::optional<bool> p;
};
REFLECTION(optional_t, p);

struct char_t {
  char ch;
};
REFLECTION(char_t, ch);

// nested object
struct simple_nested_t {
  int id;
  person p;
};
REFLECTION(simple_nested_t, id, p);

// c array
struct arr_t {
  int arr[2];
};
REFLECTION(arr_t, arr);

// std array
struct std_array_t {
  std::array<int, 2> arr;
};
REFLECTION(std_array_t, arr);

// vector
struct vector_t {
  std::vector<int> arr;
};
REFLECTION(vector_t, arr);

struct two_fields_t {
  std::array<int, 2> a;
  std::vector<std::string> v;
};
REFLECTION(two_fields_t, a, v);

// map TODO
struct map_t {
  std::map<int, std::string> map1;
  std::unordered_map<int, std::string> map2;
};
REFLECTION(map_t, map1, map2);

// list, set, unordered map... TODO

TEST_CASE("test simple object") {
  std::string_view str = R"({"name": "tom", "ok":true})";

  person p{};
  iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(p.name == "tom");
  CHECK(p.ok == true);

  SUBCASE("random order of fields") {
    person p1{};
    std::string_view str1 = R"({"ok":true, "name": "tom"})";
    iguana::from_json(p1, std::begin(str1), std::end(str1));
    CHECK(p1.name == "tom");
    CHECK(p1.ok == true);
  }
}

TEST_CASE("test two_fields object") {
  two_fields_t obj{{1, 2}, {"aa", "bb"}};
  iguana::string_stream ss;
  iguana::json::to_json(ss, obj);

  std::string str = ss.str();
  two_fields_t p{};
  iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(p.v == obj.v);
}

TEST_CASE("test simple nested object") {
  person o{.name = "tom", .ok = false};
  simple_nested_t t{1, o};
  iguana::string_stream ss;
  iguana::json::to_json(ss, t);

  std::string str = ss.str();
  simple_nested_t p{};
  iguana::from_json(p, std::begin(str), std::end(str));

  CHECK(t.id == p.id);
  CHECK(t.p.name == p.p.name);
  CHECK(t.p.ok == p.p.ok);
}

TEST_CASE("test c array and std::array") {
  arr_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::json::to_json(ss, arr);
  arr_t arr1{};
  std::string str = ss.str();

  iguana::from_json(arr1, std::begin(str), std::end(str));
  CHECK(arr.arr[0] == arr1.arr[0]);
  CHECK(arr.arr[1] == arr1.arr[1]);

  std_array_t arr2{};
  iguana::from_json(arr2, std::begin(str), std::end(str));
  CHECK(arr.arr[0] == arr2.arr[0]);
  CHECK(arr.arr[1] == arr2.arr[1]);

  vector_t vec;
  iguana::from_json(vec, std::begin(str), std::end(str));
  CHECK(vec.arr.size() == arr2.arr.size());
  CHECK(arr2.arr[0] == vec.arr[0]);
  CHECK(arr2.arr[1] == vec.arr[1]);
}

TEST_CASE("test bool, null, char, int, float") {
  {
    optional_t p{};
    std::string str = R"({"p": false})";
    iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.p.has_value());
    CHECK(*p.p == false);

    std::string str1 = R"({"p": null})";
    optional_t p1{};
    iguana::from_json(p1, std::begin(str1), std::end(str1));
    CHECK(!p1.p.has_value());
  }
  {
    char_t p{};
    std::string str = R"({"ch": "t"})";
    iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.ch == 't');
  }

  {
    bool_t p{};
    std::string str = R"({"ok": true})";
    iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.ok == true);
  }

  {
    point_t p{};
    std::string str = R"({"x" : 1, "y" : 2})";
    iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.x == 1);
    CHECK(p.y == double(2));
  }
}

TEST_CASE("test vector") {
  vector_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::json::to_json(ss, arr);

  std::string str = ss.str();
  vector_t p{};
  iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(arr.arr == p.arr);
}

TEST_CASE("test map") {
  map_t map{};
  map.map1 = {{1, "hello"}, {2, "iguana"}};
  map.map2 = {{3, "this"}, {4, "hashmap"}};
  iguana::string_stream ss;
  iguana::json::to_json(ss, map);

  std::string str = ss.str();
  map_t p{};
  iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(map.map1 == p.map1);
  CHECK(map.map2 == p.map2);
}

TEST_CASE("check some types") {
  using value_type = std::variant<int point_t::*, double point_t::*>;
  constexpr auto map = iguana::get_iguana_struct_map<point_t>();
  static_assert(map.size() == 2);
  static_assert(map.at("x") ==
                value_type{std::in_place_index_t<0>{}, &point_t::x});
  static_assert(map.at("y") ==
                value_type{std::in_place_index_t<1>{}, &point_t::y});
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP