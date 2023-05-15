
#include <deque>
#include <iterator>
#include <list>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
#include <iostream>
#include <optional>

TEST_CASE("test parse item num_t") {
  {
    std::string str{"1.4806532964699196e-22"};
    double p{};
    iguana::from_json(p, str.begin(), str.end());
    CHECK(p == 1.4806532964699196e-22);

    std::error_code ec;
    iguana::from_json(p, str.begin(), str.end(), ec);
    CHECK(p == 1.4806532964699196e-22);
    CHECK(!ec);
  }
  {
    std::string str{""};
    double p{};

    CHECK_THROWS(iguana::from_json(p, str.begin(), str.end()));
    std::error_code ec;
    iguana::from_json(p, str.begin(), str.end(), ec);
    CHECK(ec);
  }
  {
    std::string str{"1.0"};
    int p{};
    iguana::from_json(p, str.begin(), str.end());
    CHECK(p == 1);
  }
  {
    std::string str{"3000000"};
    long long p{};
    iguana::from_json(p, str.begin(), str.end());
    CHECK(p == 3000000);

    iguana::from_json(p, str.data(), str.size());
    CHECK(p == 3000000);

    std::error_code ec;
    iguana::from_json(p, str.data(), str.size(), ec);
    CHECK(!ec);
    CHECK(p == 3000000);
  }
  {
    std::string str;
    str.append(300, '1');
    int p{};
    CHECK_THROWS(iguana::from_json(p, str.begin(), str.end()));
  }
  {
    std::list<char> arr{'[', '0', '.', '9', ']'};
    std::vector<double> test;
    iguana::from_json(test, arr);
    CHECK(test[0] == 0.9);

    std::error_code ec;
    iguana::from_json(test, arr, ec);
    CHECK(!ec);
    CHECK(test[0] == 0.9);

    std::deque<char> arr1{'[', '0', '.', '9', ']'};
    iguana::from_json(test, arr1);
    CHECK(test[0] == 0.9);
  }
  {
    std::list<char> arr{'0', '.', '9'};
    for (int i = 0; i < 999; i++) {
      arr.push_back('1');
    }

    double test = 0;
    CHECK_THROWS(iguana::from_json(test, arr.begin(), arr.end()));
  }
}

TEST_CASE("test parse item array_t") {
  {
    std::string str{"[1, -222]"};
    std::array<int, 2> test;
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test[0] == 1);
    CHECK(test[1] == -222);
  }
  {
    std::string str{"[1, -222,"};
    std::array<int, 2> test;
    CHECK_NOTHROW(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"[   "};
    std::array<int, 2> test;
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"[ ]  "};
    std::array<int, 2> test;
    CHECK_NOTHROW(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"[ 1.2345]  "};
    std::array<int, 2> test;
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
}

TEST_CASE("test parse item str_t") {
  {
    std::string str{"[\"aaaaaaaaaa1\"]"};
    std::vector<std::string> test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test[0] == "aaaaaaaaaa1");
  }
  {
    // this case throw at json_util@line 132
    std::string str{"\"aaa1\""};
    std::string test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == "aaa1");
  }
  {
    std::list<char> str;
    str.push_back('[');
    str.push_back('\"');
    str.push_back('\\');
    str.push_back('a');
    str.push_back('\"');
    str.push_back(']');
    str.push_back('a');
    str.push_back('a');
    str.push_back('1');
    std::vector<std::string> test{};
    iguana::from_json(test, str.begin(), str.end());

    CHECK(test[0] == "a");
  }

  {
    std::list<char> str;
    str.push_back('\"');
    str.push_back('\\');
    str.push_back('a');
    str.push_back('\"');
    std::string test{};
    test.resize(1);
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == "a");
  }

  {
    std::list<char> list{'"', 'a', '"'};
    std::string test{};
    test.resize(2);
    iguana::from_json(test, list);
    CHECK(test == "a");
  }

  {
    std::list<char> list{'"', '\\', 'u', '8', '0', '0', '1', '"'};
    std::string test{};
    test.resize(20);
    iguana::from_json(test, list);

#ifdef __GNUC__
    CHECK(test == "老");
#endif // __GNUC__
  }
}

TEST_CASE("test parse item seq container") {
  {
    std::string str{"[0,1,2,3]"};
    std::vector<double> test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test.size() == 4);
    CHECK(test[0] == 0);
    CHECK(test[1] == 1);
    CHECK(test[2] == 2);
    CHECK(test[3] == 3);
  }
  {
    std::string str{"[0,1,2,3,]"};
    std::vector<double> test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"[0,1,2,3,"};
    std::vector<double> test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"[0,1,2"};
    std::array<int, 3> test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }

  {
    std::string str{"[0,1,2"};
    std::list<int> test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
}

enum class ee {
  aa,
  bb,
};

struct ee_t {
  ee e;
};
REFLECTION(ee_t, e);

TEST_CASE("test enum") {
  ee_t t{.e = ee::bb};
  std::string str;
  iguana::to_json(t, str);

  ee_t t1;
  iguana::from_json(t1, str);
  CHECK(t1.e == ee::bb);
}

TEST_CASE("test parse item map container") {
  {
    std::string str{"{\"key1\":\"value1\", \"key2\":\"value2\"}"};
    std::map<std::string, std::string> test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test.size() == 2);
    CHECK(test.at("key1") == "value1");
    CHECK(test.at("key2") == "value2");
  }
}

TEST_CASE("test parse item char") {
  {
    std::string str{"\"c\""};
    char test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == 'c');
  }
  {
    std::string str{"\""};
    char test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{R"("\)"};
    char test{};
    CHECK_THROWS_WITH(iguana::from_json(test, str.begin(), str.end()),
                      "Unxpected end of buffer");
  }
  {
    std::string str{""};
    char test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"\"\\a\""};
    char test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == 'a');
  }
}

TEST_CASE("test parse item tuple") {
  {
    std::string str{"[1],\"a\",1.5]"};

    std::tuple<int, std::string, double> tp;

    iguana::from_json(tp, str.begin(), str.end());
    CHECK(std::get<0>(tp) == 1);
  }
  {
    std::string str{"[1,\"a\",1.5,[1,1.5]]"};

    std::tuple<int, std::string, double, std::tuple<int, double>> tp;

    iguana::from_json(tp, str.begin(), str.end());
    CHECK(std::get<0>(tp) == 1);
  }
}

TEST_CASE("test parse item bool") {
  {
    std::string str{"true"};
    bool test = false;
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == true);
  }
  {
    std::string str{"false"};
    bool test = true;
    iguana::from_json(test, str.begin(), str.end());
    CHECK(test == false);
  }
  {
    std::string str{"True"};
    bool test = false;

    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"False"};
    bool test = true;

    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"\"false\""};
    bool test = false;

    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{""};
    bool test = false;
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
}

TEST_CASE("test parse item optional") {
  {
    std::string str{"null"};
    std::optional<int> test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(!test.has_value());
  }
  {
    std::string str{""};
    std::optional<int> test{};
    CHECK_THROWS(iguana::from_json(test, str.begin(), str.end()));
  }
  {
    std::string str{"1"};
    std::optional<int> test{};
    iguana::from_json(test, str.begin(), str.end());
    CHECK(*test == 1);
  }
}

struct optional_t {
  std::optional<bool> p;
};
REFLECTION(optional_t, p);

struct struct_test_t {
  int32_t value;
};
REFLECTION(struct_test_t, value);

struct struct_container_t {
  std::vector<struct_test_t> values;
};
REFLECTION(struct_container_t, values);

struct struct_container_1_t {
  std::optional<struct_container_t> val;
}; // entities_t
REFLECTION(struct_container_1_t, val);

TEST_CASE("test optional") {
  {
    struct_container_t container{};
    container.values = {{1}, {2}, {3}};

    struct_container_1_t t1{};
    t1.val = container;

    std::string str;
    iguana::to_json(t1, str);
    std::cout << str << "\n";

    struct_container_1_t t2{};
    iguana::from_json(t2, str);
    std::cout << t2.val.has_value() << "\n";

    CHECK(t1.val->values[0].value == t2.val->values[0].value);
    CHECK(t1.val->values[1].value == t2.val->values[1].value);
    CHECK(t1.val->values[2].value == t2.val->values[2].value);
  }

  {
    optional_t p;
    std::string str;
    iguana::to_json(p, str);

    optional_t p1;
    iguana::from_json(p1, str);
    CHECK(!p1.p.has_value());

    p.p = false;

    str.clear();
    iguana::to_json(p, str);
    std::cout << str << "\n";

    iguana::from_json(p1, str);
    CHECK(*p1.p == false);

    p.p = true;
    str.clear();
    iguana::to_json(p, str);
    std::cout << str << "\n";

    iguana::from_json(p1, str);
    CHECK(*p1.p == true);
  }
}

struct empty_t {};
REFLECTION_EMPTY(empty_t);

TEST_CASE("test empty struct") {
  empty_t t;
  std::string str;
  iguana::to_json(t, str);
  std::cout << str << "\n";

  iguana::from_json(t, str);
}

struct keyword_t {
  std::string ___private;
  std::string ___public;
  std::string ___protected;
  std::string ___class;
};
REFLECTION(keyword_t, ___private, ___protected, ___public, ___class);

TEST_CASE("test keyword") {
  std::string ss =
      R"({"private":"private","protected":"protected","public":"public","class":"class"})";
  keyword_t t2;
  iguana::from_json(t2, ss);
  CHECK(t2.___private == "private");
  CHECK(t2.___protected == "protected");
  CHECK(t2.___public == "public");
  CHECK(t2.___class == "class");
}

struct config_actor_type {
  long id = 0;
  std::string make;
  std::string config;
};
REFLECTION(config_actor_type, id, make, config);

struct config_app_json_type {
  long id = 0;
  int threads;
  std::string loglevel;
  std::vector<config_actor_type> actors;
};
REFLECTION(config_app_json_type, id, threads, loglevel, actors);

TEST_CASE("test long") {
  config_app_json_type app{1234};
  std::string str;
  iguana::to_json(app, str);

  config_app_json_type app1;
  iguana::from_json(app1, str);
  CHECK(app1.id == 1234);
}

TEST_CASE("test unknown fields") {
  std::string str = R"({"dummy":0, "name":"tom", "age":20})";
  person p;
  CHECK_THROWS_WITH(iguana::from_json(p, str), "Unknown key: dummy");

  std::string str1 = R"({"name":"tom", "age":20})";
  person p1;
  iguana::from_json(p1, str1);

  std::string str2 = R"({"name":"tom", "age":20})";
  person p2;
  iguana::from_json(p2, str2);
  std::cout << p2.name << "\n";
  CHECK(p2.name == "tom");
}

TEST_CASE("test unicode") {
  {
    std::string str2 = R"({"name":"\u8001", "age":20})";
    person p2;
    iguana::from_json(p2, str2);
#ifdef __GNUC__
    CHECK(p2.name == "老");
#endif
  }
  {
    std::string str = R"("\u8001")";

    std::string t;
    iguana::from_json(t, str);
#ifdef __GNUC__
    CHECK(t == "老");
#endif
  }
  {
    std::list<char> str = {'[', '"', '\\', 'u', '8', '0', '0', '1', '"', ']'};
    std::list<std::string> list;
    iguana::from_json(list, str);
#ifdef __GNUC__
    CHECK(*list.begin() == "老");
#endif
  }
}

TEST_CASE("test escape in string") {
  std::string str = R"({"name":"A\nB\tC\rD\bEF\n\f\n", "age": 20})";
  {
    person p;
    iguana::from_json(p, str);
    CHECK(p.name == "A\nB\tC\rD\bEF\n\f\n");
    CHECK(p.age == 20);
  }
  {
    std::string slist = R"({"name":"\u8001", "age":20})";
    std::list<char> strlist(slist.begin(), slist.end());
    person p;
    iguana::from_json(p, strlist);
    CHECK(p.name == "老");
    CHECK(p.age == 20);
  }
  {
    std::list<char> strlist(str.begin(), str.end());
    person p;
    iguana::from_json(p, strlist);
    CHECK(p.name == "A\nB\tC\rD\bEF\n\f\n");
    CHECK(p.age == 20);
  }
}

TEST_CASE("test pmr") {
#ifdef IGUANA_ENABLE_PMR
#if __has_include(<memory_resource>)
  iguana::string_stream str{&iguana::iguana_resource};
#endif
#else
  iguana::string_stream str;
#endif
  person obj{.name = "tom", .age = 20};
  iguana::to_json(obj, str);
}

TEST_CASE("test from_json_file") {
  std::string str = R"({"name":"tom", "age":20})";
  std::string filename = "test.json";
  std::ofstream out(filename, std::ios::binary);
  out.write(str.data(), str.size());
  out.close();

  person obj;
  iguana::from_json_file(obj, filename);
  CHECK(obj.name == "tom");
  CHECK(obj.age == 20);

  std::error_code ec;
  iguana::from_json_file(obj, filename, ec);
  CHECK(!ec);
  CHECK(obj.name == "tom");
  CHECK(obj.age == 20);

  std::filesystem::remove(filename);

  person p;
  CHECK_THROWS_AS(iguana::from_json_file(p, "not_exist.json"),
                  std::runtime_error);

  iguana::from_json_file(p, "not_exist.json", ec);
  CHECK(ec);

  std::string cur_path = std::filesystem::current_path().string();
  std::filesystem::create_directories("dummy_test_dir");
  CHECK_THROWS_AS(iguana::from_json_file(p, "dummy_test_dir"),
                  std::runtime_error);
  std::filesystem::remove("dummy_test_dir");

  std::ofstream out1("dummy_test_dir.json", std::ios::binary);
  CHECK_THROWS_AS(iguana::from_json_file(p, "dummy_test_dir.json"),
                  std::runtime_error);
  out1.close();
  std::filesystem::remove("dummy_test_dir.json");
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP
