
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
    iguana::parse_item(p, str.begin(), str.end());
    CHECK(p == 1.4806532964699196e-22);
  }
  {
    std::string str{""};
    double p{};
    const auto ec = iguana::parse_item(p, str.begin(), str.end());
    CHECK(ec == iguana::errc::failed_parse_number);
  }
  {
    std::string str{"1.0"};
    int p{};
    iguana::parse_item(p, str.begin(), str.end());
    CHECK(p == 1);
  }
  {
    std::string str{"3000000"};
    long long p{};
    iguana::parse_item(p, str.begin(), str.end());
    CHECK(p == 3000000);
  }
  {
    std::string str;
    str.append(300, '1');
    int p{};
    const auto ec = iguana::parse_item(p, str.begin(), str.end());
    CHECK(ec == iguana::errc::failed_parse_number);
  }
  {
    std::list<char> arr{'[', '0', '.', '9', ']'};
    std::vector<double> test;
    iguana::from_json(test, arr);
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
    const auto ec = iguana::parse_item(test, arr.begin(), arr.end());
    CHECK(ec == iguana::errc::number_too_long);
  }
}

TEST_CASE("test parse item array_t") {
  {
    std::string str{"[1, -222]"};
    std::array<int, 2> test;
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test[0] == 1);
    CHECK(test[1] == -222);
  }
  {
    std::string str{"[1, -222,"};
    std::array<int, 2> test;
    CHECK_NOTHROW(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"[   "};
    std::array<int, 2> test;
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"[ ]  "};
    std::array<int, 2> test;
    CHECK_NOTHROW(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"[ 1.2345]  "};
    std::array<int, 2> test;
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
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
    iguana::parse_item(test, str.begin(), str.end());

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
    iguana::parse_item(test, str.begin(), str.end());
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
    std::list<char> list{'"', 'u', '8', '0', '0', '1', '"'};
    std::string test{};
    test.resize(20);
    iguana::from_json(test, list);
    CHECK(test == "老");
  }
}

TEST_CASE("test parse item seq container") {
  {
    std::string str{"[0,1,2,3]"};
    std::vector<double> test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test.size() == 4);
    CHECK(test[0] == 0);
    CHECK(test[1] == 1);
    CHECK(test[2] == 2);
    CHECK(test[3] == 3);
  }
  {
    std::string str{"[0,1,2,3,]"};
    std::vector<double> test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"[0,1,2,3,"};
    std::vector<double> test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"[0,1,2"};
    std::array<int, 3> test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }

  {
    std::string str{"[0,1,2"};
    std::list<int> test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
}

TEST_CASE("test parse item map container") {
  {
    std::string str{"{\"key1\":\"value1\", \"key2\":\"value2\"}"};
    std::map<std::string, std::string> test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test.size() == 2);
    CHECK(test.at("key1") == "value1");
    CHECK(test.at("key2") == "value2");
  }
}

TEST_CASE("test parse item char") {
  {
    std::string str{"\"c\""};
    char test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == 'c');
  }
  {
    std::string str{"\""};
    char test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{R"("\)"};
    char test{};
    CHECK_THROWS_WITH(iguana::parse_item(test, str.begin(), str.end()),
                      "Unxpected end of buffer");
  }
  {
    std::string str{""};
    char test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"\"\\a\""};
    char test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == 'a');
  }
}

TEST_CASE("test parse item tuple") {
  {
    std::string str{"[1],\"a\",1.5]"};

    std::tuple<int, std::string, double> tp;

    iguana::parse_item(tp, str.begin(), str.end());
    CHECK(std::get<0>(tp) == 1);
  }
  {
    std::string str{"[1,\"a\",1.5,[1,1.5]]"};

    std::tuple<int, std::string, double, std::tuple<int, double>> tp;

    iguana::parse_item(tp, str.begin(), str.end());
    CHECK(std::get<0>(tp) == 1);
  }
}

TEST_CASE("test parse item bool") {
  {
    std::string str{"true"};
    bool test = false;
    CHECK(iguana::parse_item(test, str.begin(), str.end()) == iguana::errc::ok);
    CHECK(test == true);
  }
  {
    std::string str{"false"};
    bool test = true;
    CHECK(iguana::parse_item(test, str.begin(), str.end()) == iguana::errc::ok);
    CHECK(test == false);
  }
  {
    std::string str{"True"};
    bool test = false;

    CHECK(iguana::parse_item(test, str.begin(), str.end()) ==
          iguana::errc::not_a_bool);
  }
  {
    std::string str{"False"};
    bool test = true;

    CHECK(iguana::parse_item(test, str.begin(), str.end()) ==
          iguana::errc::not_a_bool);
  }
  {
    std::string str{"\"false\""};
    bool test = false;

    CHECK(iguana::parse_item(test, str.begin(), str.end()) ==
          iguana::errc::not_a_bool);
  }
  {
    std::string str{""};
    bool test = false;
    CHECK(iguana::parse_item(test, str.begin(), str.end()) ==
          iguana::errc::not_a_bool);
  }
}

TEST_CASE("test parse item optional") {
  {
    std::string str{"null"};
    std::optional<int> test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(!test.has_value());
  }
  {
    std::string str{""};
    std::optional<int> test{};
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"1"};
    std::optional<int> test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(*test == 1);
  }
}

TEST_CASE("test unknown fields") {
  std::string str = R"({"dummy":0, "name":"tom", "age":20})";
  person p;
  CHECK_THROWS_WITH(iguana::from_json(p, str), "Unknown key: dummy");

  std::string str1 = R"({"name":"tom", "age":20})";
  person p1;
  iguana::from_json(p1, str1);

  std::string str2 = R"({"\name":"\tom", "age":20})";
  person p2;
  iguana::from_json(p2, str2);
  std::cout << p2.name << "\n";
  CHECK(p2.name == "tom");
}

TEST_CASE("test unicode") {
  {
    std::string str2 = R"({"\name":"\u8001", "age":20})";
    person p2;
    iguana::from_json(p2, str2);

    CHECK(p2.name == "老");
  }
  {
    std::string str = R"("\u8001")";

    std::string t;
    iguana::from_json(t, str);
    CHECK(t == "老");
  }
  {
    std::list<char> str = {'[', '"', 'u', '8', '0', '0', '1', '"', ']'};

    std::list<std::string> list;
    iguana::from_json(list, str);
    CHECK(*list.begin() == "老");
  }
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

  std::filesystem::remove(filename);

  person p;
  CHECK_THROWS_AS(iguana::from_json_file(p, "not_exist.json"),
                  std::runtime_error);

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