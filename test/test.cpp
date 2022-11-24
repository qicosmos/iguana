#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
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
  bool operator==(person const &rhs) const {
    return name == rhs.name and ok == rhs.ok;
  }
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

struct map_t {
  std::map<int, std::string> map1;
  std::unordered_map<int, std::string> map2;
};
REFLECTION(map_t, map1, map2);

struct list_t {
  std::list<int> lst;
};
REFLECTION(list_t, lst);

struct forward_list_t {
  std::forward_list<int> lst;
};
REFLECTION(forward_list_t, lst);

struct deque_t {
  std::deque<int> lst;
};
REFLECTION(deque_t, lst);

struct fixed_name_object_t {
  std::string name0{};
  std::string name1{};
  std::string name2{};
  std::string name3{};
  std::string name4{};
};
REFLECTION(fixed_name_object_t, name0, name1, name2, name3, name4);

struct nested_object_t {
  std::vector<std::array<double, 3>> v3s{};
  std::string id{};
};
REFLECTION(nested_object_t, v3s, id);

struct another_object_t {
  std::string string{};
  std::string another_string{};
  bool boolean{};
  nested_object_t nested_object{};
};
REFLECTION(another_object_t, string, another_string, boolean, nested_object);

struct obj_t {
  //   fixed_object_t fixed_object{};
  fixed_name_object_t fixed_name_object{};
  another_object_t another_object{};
  std::vector<std::string> string_array{};
  std::string string{};
  double number{};
  bool boolean{};
  bool another_bool{};
};
REFLECTION(obj_t, fixed_name_object, another_object, string_array, string,
           number, boolean, another_bool);

struct tuple_t {
  std::tuple<int, double, std::string> tp;
};
REFLECTION(tuple_t, tp);

struct test_double_t {
  double val;
};
REFLECTION(test_double_t, val);

TEST_CASE("test double") {
  //  test_double_t d{.val = 1.4806532964699196e-22};
  //  iguana::string_stream ss;
  //  iguana::to_json(ss, d);
  //
  //  test_double_t p{};
  //  iguana::from_json(p, std::begin(ss), std::end(ss));
  //  CHECK(d.val == p.val);
}

TEST_CASE("test simple object") {
  {
    //    test_double_t d{.val = 1.4806532964699196e-22};
    //    iguana::string_stream ss;
    //    iguana::to_json(ss, d);
    //
    //    test_double_t p{};
    //    iguana::from_json(p, std::begin(ss), std::end(ss));
    //    std::cout << p.val << "\n";
  }

  std::string_view str = R"({"name": "tom", "ok":true})";

  person p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(p.name == "tom");
  CHECK(p.ok == true);

  SUBCASE("random order of fields") {
    person p1{};
    std::string_view str1 = R"({"ok":false, "name": "tom"})";
    [[maybe_unused]] auto ec =
        iguana::from_json(p1, std::begin(str1), std::end(str1));
    CHECK(p1.name == "tom");
    CHECK(p1.ok == false);
  }
}

TEST_CASE("test two_fields object") {
  two_fields_t obj{{1, 2}, {"aa", "bb"}};
  iguana::string_stream ss;
  iguana::to_json(ss, obj);

  std::string str = ss;
  two_fields_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(p.v == obj.v);
}

TEST_CASE("test simple nested object") {
  person o{.name = "tom", .ok = false};
  simple_nested_t t{1, o};
  iguana::string_stream ss;
  iguana::to_json(ss, t);

  std::string str = ss;
  simple_nested_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));

  CHECK(t.id == p.id);
  CHECK(t.p.name == p.p.name);
  CHECK(t.p.ok == p.p.ok);
}

TEST_CASE("test c array and std::array") {
  arr_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::to_json(ss, arr);
  arr_t arr1{};
  std::string str = ss;

  [[maybe_unused]] auto ec =
      iguana::from_json(arr1, std::begin(str), std::end(str));
  CHECK(arr.arr[0] == arr1.arr[0]);
  CHECK(arr.arr[1] == arr1.arr[1]);

  std_array_t arr2{};
  [[maybe_unused]] auto ec2 =
      iguana::from_json(arr2, std::begin(str), std::end(str));
  CHECK(arr.arr[0] == arr2.arr[0]);
  CHECK(arr.arr[1] == arr2.arr[1]);

  vector_t vec;
  [[maybe_unused]] auto ec3 =
      iguana::from_json(vec, std::begin(str), std::end(str));
  CHECK(vec.arr.size() == arr2.arr.size());
  CHECK(arr2.arr[0] == vec.arr[0]);
  CHECK(arr2.arr[1] == vec.arr[1]);
}

TEST_CASE("test bool, null, char, int, float") {
  {
    optional_t p{};
    std::string str = R"({"p": false})";
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.p.has_value());
    CHECK(*p.p == false);

    std::string str1 = R"({"p": null})";
    optional_t p1{};
    [[maybe_unused]] auto ec2 =
        iguana::from_json(p1, std::begin(str1), std::end(str1));
    CHECK(!p1.p.has_value());
  }
  {
    char_t p{};
    std::string str = R"({"ch": "t"})";
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.ch == 't');
  }

  {
    bool_t p{};
    std::string str = R"({"ok": true})";
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.ok == true);
  }

  {
    point_t p{};
    std::string str = R"({"x" : 1, "y" : 2})";
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p.x == 1);
    CHECK(p.y == double(2));
  }

  {
    std::string str = R"([1.0, 2.0])";
    std::vector<float> v;
    [[maybe_unused]] auto ec = iguana::from_json(v, str);
    CHECK(v[0] == 1.0);
    CHECK(v[1] == 2.0);
  }
}

TEST_CASE("test vector") {
  vector_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::to_json(ss, arr);

  std::string str = ss;
  vector_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(arr.arr == p.arr);
}

TEST_CASE("test map") {
  map_t map{};
  map.map1 = {{1, "hello"}, {2, "iguana"}};
  map.map2 = {{3, "this"}, {4, "hashmap"}};
  iguana::string_stream ss;
  iguana::to_json(ss, map);

  std::string str = ss;
  map_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(map.map1 == p.map1);
  CHECK(map.map2 == p.map2);
}

TEST_CASE("test nested object") {
  std::string str = R"({
         "v3s": [[0.12345, 0.23456, 0.001345],
                  [0.3894675, 97.39827, 297.92387],
                  [18.18, 87.289, 2988.298]],
         "id": "298728949872"
      })";

  nested_object_t obj{};
  [[maybe_unused]] auto ec =
      iguana::from_json(obj, std::begin(str), std::end(str));
  CHECK(obj.id == "298728949872");
}

TEST_CASE("test tuple") {
  tuple_t t;
  t.tp = std::make_tuple(2, 3.14, "hello iguana");
  iguana::string_stream ss;
  iguana::to_json(ss, t);

  std::string str = ss;
  tuple_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));

  CHECK(std::get<0>(t.tp) == std::get<0>(p.tp));
  CHECK(std::get<1>(t.tp) == std::get<1>(p.tp));
  CHECK(std::get<2>(t.tp) == std::get<2>(p.tp));
}

TEST_CASE("test list") {
  list_t list{{1, 2, 3}};
  iguana::string_stream ss;
  iguana::to_json(ss, list);

  std::string str = ss;
  list_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(list.lst == p.lst);
}

TEST_CASE("test deque_t") {
  deque_t list{{1, 2, 3}};
  iguana::string_stream ss;
  iguana::to_json(ss, list);

  std::string str = ss;
  deque_t p{};
  [[maybe_unused]] auto ec =
      iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(list.lst == p.lst);
}

inline constexpr std::string_view json0 = R"(
{
   "fixed_name_object": {
      "name0": "James",
      "name1": "Abraham",
      "name2": "Susan",
      "name3": "Frank",
      "name4": "Alicia"
   },
   "another_object": {
      "string": "here is some text",
      "another_string": "Hello World",
      "boolean": false,
      "nested_object": {
         "v3s": [[0.12345, 0.23456, 0.001345],
                  [0.3894675, 97.39827, 297.92387],
                  [18.18, 87.289, 2988.298]],
         "id": "298728949872"
      }
   },
   "string_array": ["Cat", "Dog", "Elephant", "Tiger"],
   "string": "Hello world",
   "number": 3.14,
   "boolean": true,
   "another_bool": false
}
)";

TEST_CASE("test complicated object") {
  obj_t obj;
  [[maybe_unused]] auto ec =
      iguana::from_json(obj, std::begin(json0), std::end(json0));
  CHECK(obj.number == 3.14);
  CHECK(obj.string == "Hello world");
}

TEST_CASE("test non-reflectable object") {
  {
    std::tuple<int, double, std::string> t{1, 3.14, std::string("iguana")};

    iguana::string_stream ss;
    iguana::to_json(ss, t);

    std::string str = ss;
    std::tuple<int, double, std::string> p{};
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));

    CHECK(std::get<0>(t) == std::get<0>(p));
    CHECK(std::get<1>(t) == std::get<1>(p));
    CHECK(std::get<2>(t) == std::get<2>(p));
  }

  {
    std::string str = "[1, 2, 3]";
    std::vector<int> p{};
    [[maybe_unused]] auto ec =
        iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p == std::vector<int>{1, 2, 3});

    std::array<int, 3> arr;
    [[maybe_unused]] auto ec2 =
        iguana::from_json(arr, std::begin(str), std::end(str));
    CHECK(arr == std::array<int, 3>{1, 2, 3});

    int c_arr[3];
    [[maybe_unused]] auto ec3 =
        iguana::from_json(c_arr, std::begin(str), std::end(str));
    CHECK(c_arr[0] == 1);
    CHECK(c_arr[1] == 2);
    CHECK(c_arr[2] == 3);
  }

  {
    std::string str = R"({"1":"tom"})";
    std::map<int, std::string> map;
    [[maybe_unused]] auto ec =
        iguana::from_json(map, std::begin(str), std::end(str));
    CHECK(map.size() == 1);
    CHECK(map.at(1) == "tom");
  }
}

TEST_CASE("test file interface") {
  std::string filename = "test.json";
  std::ofstream out(filename, std::ios::binary);
  out.write(json0.data(), json0.size());
  out.close();

  obj_t obj;
  [[maybe_unused]] auto ec = iguana::from_json_file(obj, filename);
  CHECK(obj.number == 3.14);
  CHECK(obj.string == "Hello world");

  std::filesystem::remove(filename);
}

TEST_CASE("test view and byte interface") {
  std::string_view str = R"({"name": "tom", "ok":true})";

  person p;
  [[maybe_unused]] auto ec1 = iguana::from_json(p, str);

  std::string str1 = {str.data(), str.size()};
  person p1;
  [[maybe_unused]] auto ec2 = iguana::from_json(p1, str1);

  CHECK(p == p1);

  std::vector<char> v;
  v.resize(str.size());
  std::memcpy(v.data(), str.data(), str.size());
  person p2;
  [[maybe_unused]] auto ec3 = iguana::from_json(p2, v);
  CHECK(p == p2);

  person p3;
  [[maybe_unused]] auto ec4 = iguana::from_json(p3, v.data(), v.size());
  CHECK(p2 == p3);
}

TEST_CASE("parse num") {
  std::string str = R"(["x"])";
  std::vector<float> v;
  CHECK(iguana::from_json(v, str) == iguana::errc::failed_parse_number);

  std::vector<int> v1;
  CHECK(iguana::from_json(v1, str) == iguana::errc::failed_parse_number);
}

TEST_CASE("parse invalid array") {
  {
    std::string str = R"([1)";
    std::vector<int> v;
    CHECK(iguana::from_json(v, str) == iguana::errc::lack_of_bracket);

    std::array<int, 1> arr;
    CHECK(iguana::from_json(arr, str) == iguana::errc::unexpected_end);
  }
  {
    std::string str = R"([ )";
    std::array<int, 1> v;
    CHECK(iguana::from_json(v, str) == iguana::errc::unexpected_end);
  }
  {
    std::string str = R"([1})";
    std::vector<float> v;
    CHECK(iguana::from_json(v, str) == iguana::errc::not_match_specific_chars);

    std::array<int, 1> arr;
    CHECK(iguana::from_json(arr, str) == iguana::errc::lack_of_bracket);
  }

  {
    std::string str = R"([])";
    std::array<int, 1> arr;
    [[maybe_unused]] auto ec = iguana::from_json(arr, str);
  }
}

TEST_CASE("parse some other char") {
  std::string str = R"({"\name":"\tom", "ok":false})";
  person p;
  [[maybe_unused]] auto ec = iguana::from_json(p, str);
  CHECK(p.name == "tom");
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