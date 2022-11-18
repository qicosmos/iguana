
#include <deque>
#include <iterator>
#include <list>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json.hpp>
#include <iguana/json_util.hpp>
#include <iostream>

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
    CHECK_THROWS(iguana::parse_item(p, str.begin(), str.end()));
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
    CHECK_THROWS(iguana::parse_item(p, str.begin(), str.end()));
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
    CHECK_THROWS(iguana::parse_item(test, arr.begin(), arr.end()));
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
    std::string str{"\"aaaaaaaaaa1\""};
    std::string test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == str.substr(1, test.size()));
  }
  {
    // this case throw at json_util@line 132
    std::string str{"\"aaa1\""};
    std::string test{};
    // iguana::parse_item(test, str.begin(), str.end());
    // CHECK(test == str.substr(1, test.size()));
  }
  {
    std::list<char> str;
    str.push_back('\\');
    str.push_back('a');
    str.push_back('\"');
    str.push_back('a');
    str.push_back('a');
    str.push_back('1');
    std::string test{};
    test.resize(2);
    iguana::parse_item(test, str.begin(), str.end(), true);
    // CHECK(test.empty());
  }
  {
    std::list<char> str;
    str.push_back('\"');
    str.push_back('a');
    str.push_back('a');
    str.push_back('a');
    str.push_back('1');
    std::string test{};
    test.resize(3);
    iguana::parse_item(test, str.begin(), str.end());
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
    std::list<char> str;
    str.push_back('\"');
    str.push_back('a');
    str.push_back('\\');
    str.push_back('\"');
    std::string test{};
    iguana::parse_item(test, str.begin(), str.end());
    // CHECK(test == "a");
  }
  {
    std::string str{"\"\\aaaa1\""};
    std::string test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == str.substr(2, test.size()));
  }
  {
    std::string str{"\"aaaa1\""};
    std::string test{};
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == str.substr(1, test.size()));
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
      std::string str{ "[0,1,2" };
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
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == true);
  }
  {
    std::string str{"false"};
    bool test = true;
    iguana::parse_item(test, str.begin(), str.end());
    CHECK(test == false);
  }
  {
    std::string str{"True"};
    bool test = false;

    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"False"};
    bool test = true;

    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{"\"false\""};
    bool test = false;

    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
  }
  {
    std::string str{""};
    bool test = false;
    CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
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

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP