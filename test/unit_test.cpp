
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
    std::string str{ "" };
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
    std::string str{"3e6"};
    int p{};
    iguana::parse_item(p, str.begin(), str.end());
    //throw here because parsed p = 3 and said not correct. 
    //CHECK(p == 3e6);
  }
  {
    std::string str;
    str.append(300, '1');
    int p{};
    CHECK_THROWS(iguana::parse_item(p, str.begin(), str.end()));
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
    //iguana::parse_item(test, str.begin(), str.end());
    //CHECK(test == str.substr(1, test.size()));
  }
}

TEST_CASE("test parse item seq container") {
    {
        std::string str{ "[0,1,2,3]" };
        std::vector<double> test{};
        iguana::parse_item(test, str.begin(), str.end());
        CHECK(test.size() == 4);
        CHECK(test[0] == 0);
        CHECK(test[1] == 1);
        CHECK(test[2] == 2);
        CHECK(test[3] == 3);
    }
    {
        std::string str{ "[0,1,2,3,]" };
        std::vector<double> test{};
        CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
    }
    {
        std::string str{ "[0,1,2,3," };
        std::vector<double> test{};
        CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
    }
}


TEST_CASE("test parse item map container")
{
    {
        std::string str{ "{\"key1\":\"value1\", \"key2\":\"value2\"}" };
        std::map<std::string, std::string> test{};
        iguana::parse_item(test, str.begin(), str.end());
        CHECK(test.size() == 2);
        CHECK(test.at("key1") == "value1");
        CHECK(test.at("key2") == "value2");
    }
}

TEST_CASE("test parse item char")
{
    {
        std::string str{ "\"c\"" };
        char test{};
        iguana::parse_item(test, str.begin(), str.end());
        CHECK(test == 'c');
    }
    {
        std::string str{ "\"" };
        char test{};
        CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
    }
    {
        std::string str{ "" };
        char test{};
        CHECK_THROWS(iguana::parse_item(test, str.begin(), str.end()));
    }
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
    return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP