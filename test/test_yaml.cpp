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

TEST_CASE("test without struct") {
  using MapType = std::unordered_map<int, std::vector<int>>;
  {
    MapType mp;
    mp[0] = {1, 2};
    mp[1] = {3, 4};
    std::string ss;
    iguana::to_yaml(mp, ss);

    MapType mp1;
    iguana::from_yaml(mp1, ss);
    CHECK(mp[0] == mp1[0]);
    CHECK(mp1[1] == mp1[1]);

    std::string ss1 = R"({0 : [1, 2], 1 : [3, 4]})";
    MapType mp2;
    iguana::from_yaml(mp2, ss1);
    CHECK(mp[0] == mp2[0]);
    CHECK(mp1[1] == mp2[1]);
  }
  {
    using TupleType = std::tuple<int, std::string, double>;
    TupleType t_v(10, "Hello", 3.14);
    std::string ss;
    iguana::to_yaml(t_v, ss);
    TupleType t;
    iguana::from_yaml(t, ss);
    CHECK(t == t_v);

    std::string ss1 = R"([10, Hello, 3.14])";
    TupleType t1;
    iguana::from_yaml(t1, ss1);
    CHECK(t1 == t_v);
  }
  {
    using TupleType = std::tuple<std::vector<int>, std::string, double>;
    TupleType t_v({1, 2, 3}, "Hello", 3.14);
    std::string ss;
    iguana::to_yaml(t_v, ss);
    std::cout << ss << std::endl;
    TupleType t;
    iguana::from_yaml(t, ss);
    CHECK(t == t_v);

    std::string ss1 = R"([ [1, 2 ,3], Hello, 3.14])";
    TupleType t1;
    iguana::from_yaml(t1, ss1);
    CHECK(t1 == t_v);
  }
}

enum class enum_status {
  start,
  stop,
};
struct plain_type_t {
  bool isok;
  enum_status status;
  char c;
  std::optional<bool> hasprice;
  std::optional<float> num;
  std::optional<int> price;
};
REFLECTION(plain_type_t, isok, status, c, hasprice, price);
TEST_CASE("test plain_type") {
  plain_type_t p{false, enum_status::stop, 'a', true};
  p.price = 20;
  std::string ss;
  iguana::to_yaml(p, ss);
  plain_type_t p1;
  iguana::from_yaml(p1, ss);
  CHECK(p1.isok == p.isok);
  CHECK(p1.status == p.status);
  CHECK(p1.c == p.c);
  CHECK(*p1.hasprice == *p.hasprice);
  CHECK(!p1.num);
  CHECK(*p1.price == 20);

  // Unknown key: pri
  std::string str = R"(pri: 10)";
  CHECK_THROWS(iguana::from_yaml(p, str));
}

TEST_CASE("test optional") {
  std::string str = R"()";
  std::optional<int> opt;
  iguana::from_yaml(opt, str.begin(), str.end());
  CHECK(!opt);
}

TEST_CASE("test exception") {
  {
    // Expected 4 hexadecimal digits
    std::string str = R"("\u28")";
    std::string str_v;
    std::error_code ec;
    iguana::from_yaml(str_v, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    // Failed to parse number
    std::string str = R"(c1wd)";
    std::string str1 = R"()";
    float f = 1.0f;
    std::error_code ec;
    iguana::from_yaml(f, str1.begin(), str1.end());
    CHECK(f == 1.0f);
    iguana::from_yaml(f, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    // Failed to parse number
    std::string str = R"(c1wd)";
    int d;
    std::error_code ec;
    iguana::from_yaml(d, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    // Expected "
    std::string str = R"("Hello)";
    std::string str_v;
    std::error_code ec;
    iguana::from_yaml(str_v, str.begin(), str.end(), ec);
    CHECK(ec);
    // Expected '
    std::string str1 = R"('Hello)";
    std::error_code ec1;
    iguana::from_yaml(str_v, str1.begin(), str1.end(), ec1);
    CHECK(ec1);
  }
  {
    // Expected one character
    std::string str = R"(He)";
    char c;
    std::error_code ec;
    iguana::from_yaml(c, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    // Expected true or false
    std::string str = R"(Flsae)";
    bool isok;
    std::error_code ec;
    iguana::from_yaml(isok, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    // Expected ] or '-'
    std::string str = R"(1, 3)";
    std::vector<int> arr;
    std::error_code ec;
    iguana::from_yaml(arr, str.begin(), str.end(), ec);
    CHECK(ec);
  }
}

struct test_string_t {
  std::vector<std::string> txt;
};
REFLECTION(test_string_t, txt);
TEST_CASE("test block string") {
  std::string str = R"(
txt:
 - |
  Hello
  World
 - >
  Hello
  World
 - >-
  Hello
  World
 - "Hello\nWorld\n"
 - "\u8001A\nB\tC\rD\bEF\n\f\n123"
 - >
 -)";
  test_string_t s;
  std::error_code ec;
  iguana::from_yaml(s, str.begin(), str.end(), ec);
  CHECK(s.txt[0] == "Hello\nWorld\n");
  CHECK(s.txt[1] == "Hello World\n");
  CHECK(s.txt[2] == "Hello World");
  CHECK(s.txt[3] == "Hello\nWorld\n"); // escape
  CHECK(s.txt[4] == "老A\nB\tC\rD\bEF\n\f\n123");
  CHECK(s.txt[5] == "\n");
  CHECK(s.txt[6].empty());
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

  std::string ss1;
  iguana::to_yaml(a1.arr, ss1);
  std::vector<std::string_view> arr;
  iguana::from_yaml(arr, ss1);
  validator(arr_t{arr});
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
  std::string str0 = R"(
  arr : [
    [a, b],
    [c, d]]
  )";
  nest_arr_t a0;
  iguana::from_yaml(a0, str0);
  validator(a0);
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
   k2 : [c, d], 
   }
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

struct some_type_t {
  std::vector<float> price;
  std::optional<std::string> description;
  std::map<std::string, int> child;
  bool hasdescription;
  char c;
  std::optional<double> d_v;
  std::string name;
  std::string_view addr;
  enum_status status;
};
REFLECTION(some_type_t, price, description, child, hasdescription, c, d_v, name,
           addr, status);
TEST_CASE("test some_type") {
  auto validator_some_type = [](some_type_t s) {
    CHECK(s.price[0] == 1.23f);
    CHECK(s.price[1] == 3.25f);
    CHECK(s.price[2] == 9.57f);
    CHECK(*s.description == "Some description");
    CHECK(s.child["key1"] == 10);
    CHECK(s.child["key2"] == 20);
    CHECK(s.hasdescription == true);
    CHECK(s.c == 'X');
    CHECK(*s.d_v == 3.14159);
    CHECK(s.name == "John Doe");
    CHECK(s.addr == "123 Main St");
    CHECK(s.status == enum_status::stop);
  };
  std::string str = R"(
price: [1.23, 3.25, 9.57]
description: >-
    Some 
    description
child:
  key1: 10
  key2: 20
hasdescription: true
c: X
d_v: 3.14159
name: John Doe
addr: '123 Main St'
status : 1
)";
  some_type_t s;
  iguana::from_yaml(s, str);
  validator_some_type(s);
  std::string ss;
  iguana::to_yaml(s, ss);
  some_type_t s1;
  iguana::from_yaml(s1, ss);
  validator_some_type(s1);
}

struct address_t {
  std::string_view street;
  std::string_view city;
  std::string_view state;
  std::string_view country;
};
REFLECTION(address_t, street, city, state, country);
struct contact_t {
  std::string_view type;
  std::string_view value;
};
REFLECTION(contact_t, type, value);
struct person_t {
  std::string_view name;
  int age;
  address_t address;
  std::vector<contact_t> contacts;
};
REFLECTION(person_t, name, age, address, contacts);
TEST_CASE("test person_t") {
  auto validator_person = [](const person_t &p) {
    CHECK(p.name == "John Doe");
    CHECK(p.age == 30);
    CHECK(p.address.street == "123 Main St");
    CHECK(p.address.city == "Anytown");
    CHECK(p.address.state == "Example State");
    CHECK(p.address.country == "Example Country");
    CHECK(p.contacts[0].type == "email");
    CHECK(p.contacts[0].value == "john@example.com");
    CHECK(p.contacts[1].type == "phone");
    CHECK(p.contacts[1].value == "123456789");
    CHECK(p.contacts[2].type == "social");
    CHECK(p.contacts[2].value == "johndoe");
  };
  std::string str = R"(
name: John Doe
age: 30
address:
  street: 123 Main St
  city: Anytown
  state: Example State
  country: Example Country
contacts:
  - type: email
    value: john@example.com
  - type: phone
    value: 123456789
  - type: social
    value: "johndoe"
  )";
  person_t p;
  iguana::from_yaml(p, str);
  validator_person(p);
  std::string ss;
  iguana::to_yaml(p, ss);
  person_t p2;
  iguana::from_yaml(p2, ss);
  validator_person(p2);
}

struct product_t {
  std::string_view name;
  float price;
  std::optional<std::string> description;
};
REFLECTION(product_t, name, price, description);
struct store_t {
  std::string name;
  std::string_view location;
  std::vector<product_t> products;
};
REFLECTION(store_t, name, location, products);
struct store_example_t {
  store_t store;
};
REFLECTION(store_example_t, store);
void store_example() {
  std::string str = R"(
store:
  name: "\u6c38\u8f89\u8d85\u5e02\t"
  location: Chengdu
  products:
    - name: iPad
      price: 
        899.4
      description: >
        nice
        ipad
    - name: watch
      price: 488.8
      description: |
        cheap watch
    - name: iPhone
      price: 999.99
      description: >-   
        expensive
        iphone
  )";
  store_example_t store_1;
  iguana::from_yaml(store_1, str);
  auto store = store_1.store;
  CHECK(store.name == "永辉超市\t");
  CHECK(store.location == "Chengdu");
  CHECK(store.products[0].name == "iPad");
  CHECK(store.products[0].price == 899.4);
  CHECK(store.products[0].description == "nice ipad\n");
  CHECK(store.products[1].name == "watch");
  CHECK(store.products[1].price == 488.8);
  CHECK(store.products[1].description == "cheap watch\n");
  CHECK(store.products[2].name == "iPhone");
  CHECK(store.products[2].price == 999.99);
  CHECK(store.products[2].description == "expensive iphone");
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
