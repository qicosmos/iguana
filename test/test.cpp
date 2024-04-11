#include <cstddef>
#include <string>
#include <vector>

#include "iguana/reflection.hpp"
#define DOCTEST_CONFIG_IMPLEMENT
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
#include <iostream>
#include <optional>

#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "iguana/prettify.hpp"
#include "iguana/struct_pb.hpp"
#include "iguana/value.hpp"

struct point_t {
  int x;
  double y;
};
REFLECTION(point_t, x, y);

struct person {
  std::string name;
  bool ok;
  bool operator==(person const &rhs) const {
    return name == rhs.name && ok == rhs.ok;
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

struct json0_obj_t {
  //   fixed_object_t fixed_object{};
  fixed_name_object_t fixed_name_object{};
  another_object_t another_object{};
  std::vector<std::string> string_array{};
  std::string string{};
  double number{};
  bool boolean{};
  bool another_bool{};
};
REFLECTION(json0_obj_t, fixed_name_object, another_object, string_array, string,
           number, boolean, another_bool);

struct tuple_t {
  std::tuple<int, double, std::string> tp;
};
REFLECTION(tuple_t, tp);

struct test_double_t {
  double val;
};
REFLECTION(test_double_t, val);

struct test_empty_t {};
REFLECTION_EMPTY(test_empty_t);

struct test {
  std::string username;
  std::string password;
  long long id;
  bool error;
};
REFLECTION(test, username, password, id, error);

template <typename T>
void get_value_test_helper(const std::string &json_str, const T &expect) {
  iguana::jvalue jv;
  CHECK_NOTHROW(iguana::parse(jv, json_str.begin(), json_str.end()));
  CHECK_NOTHROW(jv.get<T>());
  T actual{};
  CHECK_NOTHROW(jv.get_to(actual));
  CHECK(actual == expect);
}

namespace my_space {
struct inner_struct {
  int x;
  int y;
  int z;
};
REFLECTION(inner_struct, x, y, z);

template <typename T>
inline auto get_fileds_impl(inner_struct &&) {
  using namespace iguana;
  return std::make_tuple(field_t{&inner_struct::x, 1, "x"},
                         field_t{&inner_struct::x, 2, "y"},
                         field_t{&inner_struct::x, 3, "z"});
}
}  // namespace my_space

struct nest_t {
  std::string name;
  my_space::inner_struct value;
  std::variant<int, std::string> var;
};
REFLECTION(nest_t, name, value, var);

struct test_pb_st1 {
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
};
REFLECTION(test_pb_st1, x, y, z);

struct test_pb_st2 {
  int x;
  iguana::fixed32_t y;
  iguana::fixed64_t z;
};
REFLECTION(test_pb_st2, x, y, z);

struct test_pb_st3 {
  int x;
  iguana::sfixed32_t y;
  iguana::sfixed64_t z;
};
REFLECTION(test_pb_st3, x, y, z);

struct test_pb_st4 {
  int x;
  std::string y;
};
REFLECTION(test_pb_st4, x, y);

struct test_pb_st5 {
  int x;
  std::string_view y;
};
REFLECTION(test_pb_st5, x, y);

struct test_pb_st6 {
  std::optional<int> x;
  std::optional<std::string> y;
};
REFLECTION(test_pb_st6, x, y);

using Variant1 = std::variant<std::string, int>;
struct test_pb_st7 {
  iguana::one_of_t<0, Variant1> x;
  iguana::one_of_t<1, Variant1> y;
};
REFLECTION(test_pb_st7, x, y);

struct pair_t {
  int x;
  int y;
};
REFLECTION(pair_t, x, y);

struct message_t {
  int id;
  pair_t t;
};
REFLECTION(message_t, id, t);

struct test_pb_st8 {
  int x;
  pair_t y;
  message_t z;
};
REFLECTION(test_pb_st8, x, y, z);

struct test_pb_st9 {
  int x;
  std::vector<int> y;
  std::string z;
};
REFLECTION(test_pb_st9, x, y, z);

struct test_pb_st10 {
  int x;
  std::vector<message_t> y;
  std::string z;
};
REFLECTION(test_pb_st10, x, y, z);

struct test_pb_st11 {
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
REFLECTION(test_pb_st11, x, y, z);

struct test_pb_st12 {
  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
REFLECTION(test_pb_st12, x, y, z);

struct test_pb_st13 {
  int x;
  std::map<int, message_t> y;
  std::string z;
};
REFLECTION(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 {
  int x;
  colors_t y;
  level_t z;
};
REFLECTION(test_pb_st14, x, y, z);

namespace client {
struct person {
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
}  // namespace client

struct my_struct {
  int x;
  int y;
  int z;
};
REFLECTION(my_struct, x, y, z);

struct nest1 {
  std::string name;
  my_struct value;
  int var;
};

REFLECTION(nest1, name, value, var);

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{41, 42, 43};

    std::string str;
    iguana::to_pb(inner, str);

    my_space::inner_struct inner1;
    iguana::from_pb(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);
  }

  {
    test_pb_st1 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st1 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }

  {
    test_pb_st2 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st2 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st3 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st4 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st4 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st5 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st5 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st6 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // variant
    test_pb_st7 st1{{"test"}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st7 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.x.value == st2.x.value);
  }
  {
    // sub nested objects
    nest1 v{"Hi", {1, 2, 3}, 5}, v2;
    std::string s;
    iguana::to_pb(v, s);
    iguana::from_pb(v2, s);
    CHECK(v.var == v2.var);

    test_pb_st8 st1{1, {3, 4}, {5, {7, 8}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st8 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z.t.y == st2.z.t.y);
  }
  {
    // repeated messages
    test_pb_st9 st1{1, {2, 4, 6}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st9 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    test_pb_st10 st1{1, {{5, {7, 8}}, {9, {11, 12}}}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st10 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    test_pb_st11 st1{1, {{{5, {7, 8}}}, {{9, {11, 12}}}}, {"test"}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, "test"}, {2, "ok"}}, {{"test", 4}, {"ok", 6}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st12 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(1, message_t{2, {3, 4}});
    st1.y.emplace(2, message_t{4, {6, 8}});
    st1.z = "test";
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st13 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // enum
    test_pb_st14 st1{1, colors_t::black, level_t::info};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st14 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
}

TEST_CASE("test members") {
  using namespace iguana;
  using namespace iguana::detail;

  my_space::inner_struct inner{41, 42, 43};
  const auto &arr = iguana::get_members(inner);
  std::visit(
      [&inner](auto &member) mutable {
        CHECK(member.field_no == 2);
        CHECK(member.field_name == "y");
        CHECK(member.value(inner) == 42);
      },
      arr.at(1));

  point_t pt{2, 3};
  iguana::get_members(pt);
  const auto &arr1 = iguana::get_members(pt);
  auto &val = arr1.at(0);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

TEST_CASE("test variant") {
  std::variant<int, std::string> var;
  var = 1;
  nest_t v{"Hi", {1, 2, 3}, var}, v2;
  std::string s;
  iguana::to_json(v, s);
  std::cout << s << std::endl;
  iguana::from_json(v2, s);
  CHECK(v.name == v2.name);
}

TEST_CASE("test from issues") {
  test test1{};
  std::string str1 =
      R"({"username1": "test", "password":test, "id": 10.1, "error": false})";

  CHECK_THROWS(iguana::from_json(test1, str1));
  std::cout << test1.username << std::endl;
  std::cout << test1.password << std::endl;
  std::cout << test1.id << std::endl;
  std::cout << std::boolalpha << test1.error << std::endl;
}

TEST_CASE("test dom parse") {
  {
    std::string_view str = R"(null)";
    iguana::jvalue val;
    iguana::parse(val, str.begin(), str.end());
    std::error_code ec;
    [[maybe_unused]] int i = val.get<int>(ec);
    if (ec) {
      CHECK(ec.message() == "wrong type, real type is null type");
    }
    CHECK(val.get<std::nullptr_t>() == std::nullptr_t{});
  }
  {
    std::string_view str = R"(false)";
    iguana::jvalue val;
    iguana::parse(val, str.begin(), str.end());

    std::error_code ec;
    auto b = val.get<bool>(ec);
    CHECK(!ec);
    CHECK(!b);
  }
  {
    std::string_view str = R"({"name": "tom", "ok":true, "t": {"val":2.5}})";
    iguana::jvalue val;
    iguana::parse(val, str.begin(), str.end());

    CHECK(val.at<std::string>("name") == "tom");
    CHECK(val.at<bool>("ok") == true);

    std::error_code ec;
    CHECK(val.at<bool>("no such", ec) == false);
    CHECK(ec);

    CHECK_THROWS_WITH(val.at<int>("no ec"), "the key is unknown");

    auto sub_map = val.at<iguana::jobject>("t");
    CHECK(sub_map.at("val").get<double>() == 2.5);
    CHECK(val.is_object());
  }
  std::cout << "test dom parse part 1 ok\n";
  {
    std::string json_str = R"({"a": [1, 2, 3]})";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());

    auto &map = std::get<iguana::jobject>(val1);
    auto &arr = std::get<iguana::jarray>(map.at("a"));

    CHECK(std::get<int>(arr[0]) == 1);
    CHECK(std::get<int>(arr[1]) == 2);
    CHECK(std::get<int>(arr[2]) == 3);
    CHECK(val1.is_object());
    CHECK(val1.to_object().size() == 1);
  }
  {
    std::string json_str = R"({ "b": [{}, {"c":1}] })";
    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, json_str.begin(), json_str.end()));
  }

  std::cout << "test dom parse part 2 ok\n";

  {
    std::string json_str = R"([0.5, 2.2, 3.3, 4, "6.7"])";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());
    auto &arr = std::get<iguana::jarray>(val1);

    CHECK(val1.at<double>(1) == 2.2);

    std::error_code ec1;
    val1.at<int>(1, ec1);
    CHECK(ec1);
    std::cout << ec1.message() << "\n";

    {
      CHECK_THROWS_WITH(val1.at<double>(9), "idx is out of range");
      std::error_code ec;
      CHECK_NOTHROW(val1.at<double>(-1, ec));
      CHECK(ec);
    }

    CHECK(std::get<double>(arr[0]) == 0.5);
    CHECK(std::get<double>(arr[1]) == 2.2);
    CHECK(std::get<double>(arr[2]) == 3.3);
    // could arr elems be different type?
    CHECK(std::get<std::string>(arr[4]) == "6.7");

    CHECK(val1.is_array());
    const iguana::jarray &arr1 = val1.to_array();
    CHECK(arr1.size() == 5);
    CHECK(arr1[0].to_double() == 0.5);
    CHECK(arr1[3].is_int());

    std::error_code ec;
    CHECK_NOTHROW(val1.to_object(ec));
    CHECK_THROWS(val1.to_object());
  }
  {
    std::string json_str = R"(709)";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());
    auto &num = std::get<int>(val1);
    CHECK(num == 709);
    CHECK_THROWS(std::get<double>(val1));

    get_value_test_helper(json_str, 709);
  }

  std::cout << "test get ok\n";
  {
    std::string json_str = R"(-0.111)";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());

    CHECK(val1.is_double());
    CHECK(val1.is_number());
    CHECK(!val1.is_array());
  }
  {
    std::string json_str = R"(true)";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());
    CHECK(val1.is_bool());

    bool expect = true;
    get_value_test_helper(json_str, expect);
  }
  {
    std::string json_str = R"("true")";
    iguana::jvalue val1;
    iguana::parse(val1, json_str.begin(), json_str.end());
    CHECK(val1.is_string());

    std::string expect("true");
    get_value_test_helper(json_str, expect);
  }
  {
    std::string json_str = R"(null)";
    iguana::jvalue val1;
    CHECK(val1.is_undefined());

    iguana::parse(val1, json_str.begin(), json_str.end());
    CHECK(val1.is_null());
    // throw
    CHECK_THROWS(val1.to_array());
    CHECK_THROWS(val1.to_object());
    CHECK_THROWS(val1.to_double());
    CHECK_THROWS(val1.to_int());
    CHECK_THROWS(val1.to_bool());
    CHECK_THROWS(val1.to_string());
    // no throw
    std::error_code ec;
    CHECK_NOTHROW(val1.to_array(ec));
    CHECK_NOTHROW(val1.to_object(ec));
    CHECK_NOTHROW(val1.to_double(ec));
    CHECK_NOTHROW(val1.to_int(ec));
    CHECK_NOTHROW(val1.to_bool(ec));
    CHECK_NOTHROW(val1.to_string(ec));
  }
  {
    // what should be filled back?
    std::string json_str = R"("tr)";
    iguana::jvalue val1;
    std::error_code ec{};
    CHECK_NOTHROW(iguana::parse(val1, json_str.begin(), json_str.end(), ec));
    CHECK(!val1.is_string());
    CHECK(val1.is_null());
  }
  {
    std::string_view str =
        R"({"name": "tom", "skill": ["cpp", "go"], "parent": {"name": "jone"}})";
    iguana::jvalue val;
    iguana::parse<true>(val, str.begin(), str.end());
    CHECK(val.at<std::string_view>("name") == "tom");
    auto arr = val.at<iguana::jarray>("skill");
    CHECK(arr[0].to_string_view() == "cpp");
    CHECK(arr[1].to_string_view() == "go");
    auto submap = val.at<iguana::jobject>("parent");
    CHECK(submap["name"].to_string_view() == "jone");
  }
  {
    std::string str = R"(["cpp", "go", "java"])";
    iguana::jarray arr;
    iguana::parse(arr, str);
    CHECK(arr[0].to_string() == "cpp");
    CHECK(arr[1].to_string() == "go");
    CHECK(arr[2].to_string() == "java");
  }
  {
    std::string str = R"({"name": "tom", "age": 5})";
    iguana::jvalue val;
    iguana::parse(val, str.begin(), str.end(), true);
    CHECK(5.0f == (val.to_object())["age"].to_double());
  }

  std::cout << "test dom parse ok\n";
}

TEST_CASE("test simple object") {
  {
    //    test_double_t d{.val = 1.4806532964699196e-22};
    //    iguana::string_stream ss;
    //    iguana::to_json(d, ss);
    //
    //    test_double_t p{};
    //    iguana::from_json(p, std::begin(ss), std::end(ss));
    //    std::cout << p.val << "\n";
  }

  std::string_view str = R"({"name": "tom", "ok":true})";

  person p{};
  iguana::from_json(p, std::begin(str), std::end(str));
  CHECK(p.name == "tom");
  CHECK(p.ok == true);

  auto pretty_str = iguana::prettify(str);
  std::cout << pretty_str << "\n";

  SUBCASE("random order of fields") {
    person p1{};
    std::string_view str1 = R"({"ok":false, "name": "tom"})";
    iguana::from_json(p1, std::begin(str1), std::end(str1));
    CHECK(p1.name == "tom");
    CHECK(p1.ok == false);
  }
}

TEST_CASE("test two_fields object") {
  two_fields_t obj{{1, 2}, {"aa", "bb"}};
  iguana::string_stream ss;
  iguana::to_json(obj, ss);

  two_fields_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));
  CHECK(p.v == obj.v);
}

TEST_CASE("test simple nested object") {
  person o{"tom", false};
  simple_nested_t t{1, o};
  iguana::string_stream ss;
  iguana::to_json(t, ss);

  simple_nested_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));

  CHECK(t.id == p.id);
  CHECK(t.p.name == p.p.name);
  CHECK(t.p.ok == p.p.ok);
}

TEST_CASE("test c array and std::array") {
  arr_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::to_json(arr, ss);
  arr_t arr1{};

  iguana::from_json(arr1, std::begin(ss), std::end(ss));
  CHECK(arr.arr[0] == arr1.arr[0]);
  CHECK(arr.arr[1] == arr1.arr[1]);

  std_array_t arr2{};
  iguana::from_json(arr2, std::begin(ss), std::end(ss));
  CHECK(arr.arr[0] == arr2.arr[0]);
  CHECK(arr.arr[1] == arr2.arr[1]);

  vector_t vec;
  iguana::from_json(vec, std::begin(ss), std::end(ss));
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

  {
    std::string str = R"([1.0, 2.0])";
    std::vector<float> v;
    iguana::from_json(v, str);
    CHECK(v[0] == 1.0);
    CHECK(v[1] == 2.0);
  }
}

TEST_CASE("test vector") {
  vector_t arr{{1, 2}};
  iguana::string_stream ss;
  iguana::to_json(arr, ss);

  vector_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));
  CHECK(arr.arr == p.arr);
}

TEST_CASE("test map") {
  map_t map{};
  map.map1 = {{1, "hello"}, {2, "iguana"}};
  map.map2 = {{3, "this"}, {4, "hashmap"}};
  iguana::string_stream ss;
  iguana::to_json(map, ss);

  map_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));
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
  iguana::from_json(obj, std::begin(str), std::end(str));
  CHECK(obj.id == "298728949872");
}

TEST_CASE("test tuple") {
  tuple_t t;
  t.tp = std::make_tuple(2, 3.14, "hello iguana");
  iguana::string_stream ss;
  iguana::to_json(t, ss);

  tuple_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));

  CHECK(std::get<0>(t.tp) == std::get<0>(p.tp));
  CHECK(std::get<1>(t.tp) == std::get<1>(p.tp));
  CHECK(std::get<2>(t.tp) == std::get<2>(p.tp));
}

TEST_CASE("test list") {
  list_t list{{1, 2, 3}};
  iguana::string_stream ss;
  iguana::to_json(list, ss);

  list_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));
  CHECK(list.lst == p.lst);
}

TEST_CASE("test deque_t") {
  deque_t list{{1, 2, 3}};
  iguana::string_stream ss;
  iguana::to_json(list, ss);

  deque_t p{};
  iguana::from_json(p, std::begin(ss), std::end(ss));
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
  json0_obj_t obj;
  iguana::from_json(obj, std::begin(json0), std::end(json0));
  CHECK(obj.number == 3.14);
  CHECK(obj.string == "Hello world");
}

TEST_CASE("test unique_ptr object") {
  std::unique_ptr<json0_obj_t> obj_ptr;
  iguana::from_json(obj_ptr, std::begin(json0), std::end(json0));
  CHECK(obj_ptr->number == 3.14);
  CHECK(obj_ptr->string == "Hello world");
  std::string ss;
  iguana::to_json(obj_ptr, ss);

  std::unique_ptr<json0_obj_t> obj_ptr2;
  iguana::from_json(obj_ptr2, ss);
  CHECK(obj_ptr2->number == 3.14);
  CHECK(obj_ptr2->string == "Hello world");

  std::string_view ss1 = R"(null)";
  std::unique_ptr<json0_obj_t> obj_ptr3;
  iguana::from_json(obj_ptr3, ss1);
  CHECK(!obj_ptr3);
  std::string ss2;
  iguana::to_json(obj_ptr3, ss2);
  CHECK(ss2 == "null");
}

TEST_CASE("test empty object") {
  test_empty_t empty_obj;

  iguana::string_stream ss;
  iguana::to_json(empty_obj, ss);
  CHECK(ss == "{}");
}

TEST_CASE("test non-reflectable object") {
  {
    std::tuple<int, double, std::string> t{1, 3.14, std::string("iguana")};

    iguana::string_stream ss;
    iguana::to_json(t, ss);

    std::tuple<int, double, std::string> p{};
    iguana::from_json(p, std::begin(ss), std::end(ss));

    CHECK(std::get<0>(t) == std::get<0>(p));
    CHECK(std::get<1>(t) == std::get<1>(p));
    CHECK(std::get<2>(t) == std::get<2>(p));
  }

  {
    std::string str = "[1, 2, 3]";
    std::vector<int> p{};
    iguana::from_json(p, std::begin(str), std::end(str));
    CHECK(p == std::vector<int>{1, 2, 3});

    std::array<int, 3> arr;
    iguana::from_json(arr, std::begin(str), std::end(str));
    CHECK(arr == std::array<int, 3>{1, 2, 3});

    int c_arr[3];
    iguana::from_json(c_arr, std::begin(str), std::end(str));
    CHECK(c_arr[0] == 1);
    CHECK(c_arr[1] == 2);
    CHECK(c_arr[2] == 3);
  }

  {
    std::string str = R"({"1":"tom"})";
    std::map<int, std::string> map;
    iguana::from_json(map, std::begin(str), std::end(str));
    CHECK(map.size() == 1);
    CHECK(map.at(1) == "tom");
  }
}

TEST_CASE("test file interface") {
  namespace fs = std::filesystem;
  {
    std::string filename = "test.json";
    std::ofstream out(filename, std::ios::binary);
    out.write(json0.data(), json0.size());
    out.close();

    json0_obj_t obj;
    iguana::from_json_file(obj, filename);
    CHECK(obj.number == 3.14);
    CHECK(obj.string == "Hello world");

    fs::remove(filename);
  }  // namespace std::filesystem;
  {
    fs::path p = "empty_file.bin";
    std::ofstream{p};
    test_empty_t empty_obj;
    CHECK_THROWS_WITH(iguana::from_json_file(empty_obj, p.string()),
                      "empty file");

    fs::remove(p);
  }
  {
    test_empty_t empty_obj;
    CHECK_THROWS(iguana::from_json_file(empty_obj, "/null"));
  }
}

TEST_CASE("test view and byte interface") {
  std::string_view str = R"({"name": "tom", "ok":true})";

  person p;
  iguana::from_json(p, str);

  std::string str1 = {str.data(), str.size()};
  person p1;
  iguana::from_json(p1, str1);

  CHECK(p == p1);

  std::vector<char> v;
  v.resize(str.size());
  std::memcpy(v.data(), str.data(), str.size());
  person p2;
  iguana::from_json(p2, v);
  CHECK(p == p2);

  person p3;
  iguana::from_json(p3, v.data(), v.size());
  CHECK(p2 == p3);
}

TEST_CASE("parse num") {
  std::string str = R"(["x"])";
  std::vector<float> v;
  CHECK_THROWS_WITH(iguana::from_json(v, str), "Failed to parse number");

  std::vector<int> v1;
  CHECK_THROWS_WITH(iguana::from_json(v1, str), "Failed to parse number");
}

TEST_CASE("parse invalid array") {
  {
    std::string str = R"([1)";
    std::vector<int> v;
    CHECK_THROWS_WITH(iguana::from_json(v, str), "Expected ]");

    std::array<int, 1> arr;
    CHECK_THROWS_WITH(iguana::from_json(arr, str), "Unexpected end");
  }
  {
    std::string str = R"([ )";
    std::array<int, 1> v;
    CHECK_THROWS_WITH(iguana::from_json(v, str), "Unexpected end");
  }
  {
    std::string str = R"([1})";
    std::vector<float> v;
    CHECK_THROWS_AS(iguana::from_json(v, str), std::runtime_error);
  }
  {
    std::string str = R"([1}])";
    std::array<int, 1> arr;
    CHECK_THROWS_WITH(iguana::from_json(arr, str), "Expected ]");
  }

  {
    std::string str = R"([])";
    std::array<int, 1> arr;
    iguana::from_json(arr, str);
  }
}

TEST_CASE("parse some other char") {
  std::string str = R"({"name":"tom", "ok":false})";
  person p;
  iguana::from_json(p, str);
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

enum class Status { STOP = 10, START };
namespace iguana {
template <>
struct enum_value<Status> {
  constexpr static std::array<int, 1> value = {10};
};
}  // namespace iguana
TEST_CASE("test exception") {
  {
    std::string str = R"({"\u8001": "name"})";
    std::unordered_map<std::string_view, std::string> mp;
    iguana::from_json(mp, str);
    CHECK(mp["老"] == "name");
  }
  {
    std::string str = R"({"a": "\)";
    std::unordered_map<std::string, std::string> mp;
    CHECK_THROWS(iguana::from_json(mp, str));
  }
  {
    std::string str = R"({"a": "\u8")";
    std::unordered_map<std::string, std::string> mp;
    CHECK_THROWS(iguana::from_json(mp, str));
  }
  {
    std::string str = R"([10, d5])";
    std::deque<char> char_q(str.begin(), str.end());
    std::vector<int> arr;
    CHECK_THROWS(iguana::from_json(arr, char_q));
  }
  {
    std::string str = R"({"a": "\)";
    std::unordered_map<std::string, char> mp;
    CHECK_THROWS(iguana::from_json(mp, str));
  }
  {
    std::string str = R"({"a: "\")";
    std::unordered_map<std::string, std::string_view> mp;
    CHECK_THROWS(iguana::from_json(mp, str));
  }
  {
    std::string str = R"(bb)";
    iguana::jvalue val;
    CHECK_THROWS(iguana::parse(val, str.begin(), str.end()));
  }
  {
    // THREW exception: duplicated key a
    std::string str = R"({"a": "b", "a": "c"})";
    iguana::jvalue val;
    CHECK_THROWS(iguana::parse(val, str.begin(), str.end()));
  }
  {
    std::string str = R"({"a": "c",)";
    iguana::jvalue val;
    CHECK_THROWS(iguana::parse(val, str.begin(), str.end()));
  }
  {
    std::string str = R"(
{
  "a": "START",
  "b": "STOP",
}
  )";
    std::unordered_map<std::string, Status> mp;
    CHECK_THROWS(iguana::from_json(mp, str));
  }
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
  {
    std::unordered_map<std::string, Status> mp;
    mp["a"] = Status::START;
    mp["b"] = Status::STOP;
    std::string ss;
    CHECK_THROWS(iguana::to_json(mp, ss));
  }
#endif
}

namespace my_space {
struct my_struct {
  int x, y, z;
  bool operator==(const my_struct &o) const {
    return x == o.x && y == o.y && z == o.z;
  }
};

template <bool Is_writing_escape, typename Stream>
inline void to_json_impl(Stream &s, const my_struct &t) {
  iguana::to_json(*(int(*)[3]) & t, s);
}

template <typename It>
IGUANA_INLINE void from_json_impl(my_struct &value, It &&it, It &&end) {
  iguana::from_json(*(int(*)[3]) & value, it, end);
}

}  // namespace my_space

struct nest {
  std::string name;
  my_space::my_struct value;
  bool operator==(const nest &o) const {
    return name == o.name && value == o.value;
  }
};

REFLECTION(nest, name, value);

void example1() {
  my_space::my_struct v{1, 2, 3}, v2;
  std::string s;
  iguana::to_json(v, s);
  std::cout << s << std::endl;
  iguana::from_json(v2, s);
  CHECK(v == v2);
};

void example2() {
  nest v{"Hi", {1, 2, 3}}, v2;
  std::string s;
  iguana::to_json(v, s);
  std::cout << s << std::endl;
  iguana::from_json(v2, s);
  CHECK(v == v2);
};

void test_user_defined_struct() {
  example1();
  example2();
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
