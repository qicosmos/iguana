#include <deque>
#include <iterator>
#include <list>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/xml_reader.hpp"
#include "iguana/xml_writer.hpp"
#include "rapidxml_print.hpp"
#include <iostream>
#include <optional>

struct simple_t {
  std::vector<int> a;
  char b;
  bool c;
  bool d;
  std::optional<std::string> e;
  bool operator==(const simple_t &other) const {
    if ((b == other.b) && (c == other.c) && (d == other.d) && (e == other.e)) {
      auto t1 = a;
      auto t2 = other.a;
      sort(t1.begin(), t1.end());
      sort(t2.begin(), t2.end());
      return t1 == t2;
    }
    return false;
  }
};
REFLECTION(simple_t, a, b, c, d, e);
TEST_CASE("test simple xml") {
  simple_t simple{{1, 2, 3}, '|', 0, 1};
  std::string str;
  iguana::xml::to_xml_pretty(str, simple);

  simple_t sfrom;
  iguana::xml::from_xml(sfrom, str.data());
  CHECK(sfrom == simple);

  std::string xmlstr = R"(
    <simple_t><a>1</a><a>2</a><a>3</a><b>|</b><c>False</c><d>True</d><e>optional?</e></simple_t>
  )";
  simple_t sfrom2;
  iguana::xml::from_xml(sfrom2, xmlstr.data());
  auto t = sfrom2.a;
  sort(t.begin(), t.end());
  CHECK(t == std::vector{1, 2, 3});
  CHECK(sfrom2.b == '|');
  CHECK(!sfrom2.c);
  CHECK(sfrom2.d);
  CHECK(*sfrom2.e == "optional?");
}

struct nested_t {
  simple_t simple;
  int code;
};
REFLECTION(nested_t, simple, code);
TEST_CASE("test simple nested") {
  std::string str = R"(
    <nested_t>
    <simple><a>1</a><a>2</a><a>3</a><b>|</b><c>False</c><d>True</d><e></e></simple>
    <code>10086</code>
    </nested_t>
  )";
  nested_t nest;
  iguana::xml::from_xml(nest, str.data());
  simple_t res{{1, 2, 3}, '|', 0, 1};
  CHECK(res == nest.simple);
  CHECK(nest.code == 10086);

  std::string toxmlstr;
  iguana::xml::to_xml_pretty(toxmlstr, nest);
  nested_t nest2;
  iguana::xml::from_xml(nest2, toxmlstr.data());
  CHECK(nest2.simple == nest.simple);

  nest2.simple.a = std::vector<int>();
  std::string ss;
  iguana::xml::to_xml(ss, nest2);
}

struct book_t {
  std::string title;
  int edition;
  std::vector<std::string> author;
  std::optional<std::string> description;
  bool operator==(const book_t &other) const {
    if ((title == other.title) && (edition == other.edition) &&
        (description == other.description)) {
      auto t1 = author;
      auto t2 = other.author;
      sort(t1.begin(), t1.end());
      sort(t2.begin(), t2.end());
      return t1 == t2;
    }
    return false;
  }
};

REFLECTION(book_t, title, edition, author, description);
TEST_CASE("test optinal and vector") {
  std::string str = R"(
    <book_t>
      <title>C++ templates</title>
      <edition>2</edition>
      <author>David Vandevoorde</author>
      <author>Nicolai M. Josuttis</author>
      <author>Douglas Gregor</author>
      <description>talking about how to use template</description>
    </book_t> 
  )";
  std::string origin_str = str;
  book_t book;
  iguana::xml::from_xml(book, str.data());
  CHECK(book.title == "C++ templates");
  CHECK(book.author.size() == 3);

  std::vector<std::string> author = {"David Vandevoorde", "Nicolai M. Josuttis",
                                     "Douglas Gregor"};
  sort(author.begin(), author.end());
  auto t = book.author;
  sort(t.begin(), t.end());
  CHECK(t == author);
  CHECK(book.edition == 2);
  CHECK(book.description);
  CHECK(*book.description == "talking about how to use template");

  std::string xml_str;
  iguana::xml::to_xml(xml_str, book);
  book_t newbook;
  iguana::xml::from_xml(newbook, xml_str.data());
  CHECK_MESSAGE(newbook == book, "the newer must be same as the older");
}

struct library_t {
  std::vector<book_t> book;
  int sum;
};
REFLECTION(library_t, book, sum);
TEST_CASE("test nested vector") {
  std::string str = R"(
    <library>
    <book>
      <title>C++ templates</title>
      <edition>2</edition>
      <author>David Vandevoorde</author>
      <author>Nicolai M. Josuttis</author>
      <author>Douglas Gregor</author>
      <description>talking about how to use template</description>
    </book>
    <book>
      <title>C++ primer</title>
      <edition>6</edition>
      <author>Stanley B. Lippman</author>
      <author>Josée Lajoie</author>
      <author>Barbara E. Moo</author>
      <description></description>
    </book>
    <sum>2</sum>
    </library>
  )";
  library_t library;
  iguana::xml::from_xml(library, str.data());
  CHECK(library.sum == 2);
  CHECK(library.book[0].title == "C++ templates");
  CHECK(library.book[1].title == "C++ primer");
  if (library.book[1].title == "C++ primer") {
    CHECK(!library.book[1].description);
    std::vector<std::string> author = {"Stanley B. Lippman", "Josée Lajoie",
                                       "Barbara E. Moo"};
    auto t1 = library.book[1].author;
    CHECK(t1.size() == 3);
    sort(t1.begin(), t1.end());
    sort(author.begin(), author.end());
    CHECK(t1 == author);
  }

  std::string xml_str;
  iguana::xml::to_xml(xml_str, library);
  library_t newlibrary;
  iguana::xml::from_xml(newlibrary, xml_str.data());
  if (newlibrary.book[0].title == library.book[0].title) {
    CHECK(newlibrary.book[0] == library.book[0]);
    CHECK(newlibrary.book[1] == library.book[1]);
  } else {
    CHECK(newlibrary.book[1] == library.book[0]);
    CHECK(newlibrary.book[0] == library.book[1]);
  }
}

struct book_attr_t {
  std::map<std::string, float> __attr;
  std::string title;
};
REFLECTION(book_attr_t, __attr, title);
TEST_CASE("test attribute with map") {
  std::string str = R"(
    <book_attr_t id="5" pages="392" price="79.9">
      <title>C++ templates</title>
    </book_with_attr_t>
  )";
  book_attr_t b;
  iguana::xml::from_xml(b, str.data());
  CHECK(b.__attr["id"] == 5);
  CHECK(b.__attr["pages"] == 392.0f);
  CHECK(b.__attr["price"] == 79.9f);

  std::string ss;
  iguana::xml::to_xml_pretty(ss, b);
  book_attr_t b2;
  iguana::xml::from_xml(b2, ss.data());
  CHECK(b2.__attr["id"] == 5);
  CHECK(b2.__attr["pages"] == 392.0f);
  CHECK(b2.__attr["price"] == 79.9f);
}

struct book_attr_any_t {
  std::unordered_map<std::string, iguana::xml::any_t> __attr;
  std::string title;
};
REFLECTION(book_attr_any_t, __attr, title);
TEST_CASE("test attribute with any") {
  std::string str = R"(
    <book_attr_any_t id="5" language="en" price="79.9">
      <title>C++ templates</title>
    </book_attr_any_t>
  )";
  book_attr_any_t b;
  iguana::xml::from_xml(b, str.data());
  auto &map = b.__attr;
  CHECK(map["id"].get<int>().first);
  CHECK(map["id"].get<int>().second == 5);
  CHECK(map["language"].get<std::string_view>().first);
  CHECK(map["language"].get<std::string_view>().second == "en");
  CHECK(map["price"].get<float>().first);
  CHECK(map["price"].get<float>().second == 79.9f);

  std::string ss;
  iguana::xml::to_xml(ss, b);
  book_attr_any_t b1;
  iguana::xml::from_xml(b1, ss.data());
  map = b1.__attr;
  CHECK(map["id"].get<int>().first);
  CHECK(map["id"].get<int>().second == 5);
  CHECK(map["language"].get<std::string_view>().first);
  CHECK(map["language"].get<std::string_view>().second == "en");
  CHECK(map["price"].get<float>().first);
  CHECK(map["price"].get<float>().second == 79.9f);
}

struct library_attr_t {
  book_attr_any_t book;
  std::unordered_map<std::string, iguana::xml::any_t> __attr;
};
REFLECTION(library_attr_t, book, __attr);
TEST_CASE("Test nested attribute with any") {
  std::string str = R"(
    <library_attr_t code="102" name="UESTC" time="3.2">
      <book id="5" language="en" price="79.9">
        <title>C++ templates</title>
      </book>
    </library_attr_t>
  )";
  library_attr_t library;
  iguana::xml::from_xml(library, str.data());

  auto map = library.__attr;
  CHECK(map["code"].get<int>().first);
  CHECK(map["code"].get<int>().second == 102);
  CHECK(map["name"].get<std::string>().second == "UESTC");
  CHECK(map["name"].get<std::string_view>().second == "UESTC");
  CHECK(map["time"].get<float>().first);
  CHECK(map["time"].get<float>().second == 3.2f);

  map = library.book.__attr;
  CHECK(map["id"].get<int>().first);
  CHECK(map["id"].get<int>().second == 5);
  CHECK(map["language"].get<std::string_view>().first);
  CHECK(map["language"].get<std::string_view>().second == "en");
  CHECK(map["price"].get<float>().first);
  CHECK(map["price"].get<float>().second == 79.9f);

  std::string ss;
  iguana::xml::to_xml_pretty(ss, library);
  std::cout << ss << std::endl;
  library_attr_t library1;
  iguana::xml::from_xml(library1, ss.data());
  map = library1.__attr;
  CHECK(map["code"].get<int>().first);
  CHECK(map["code"].get<int>().second == 102);
  CHECK(map["name"].get<std::string>().second == "UESTC");
  CHECK(map["name"].get<std::string_view>().second == "UESTC");
  CHECK(map["time"].get<float>().first);
  CHECK(map["time"].get<float>().second == 3.2f);

  map = library1.book.__attr;
  CHECK(map["id"].get<int>().first);
  CHECK(map["id"].get<int>().second == 5);
  CHECK(map["language"].get<std::string_view>().first);
  CHECK(map["language"].get<std::string_view>().second == "en");
  CHECK(map["price"].get<float>().first);
  CHECK(map["price"].get<float>().second == 79.9f);
  CHECK_FALSE(map["language"].get<int>().first); // parse num failed
}

TEST_CASE("test exception") {
  simple_t simple;
  std::string str = R"(
    <simple_t><a>1</a><a>2</a><a>3</a><b>|</b><c>False</c><d>True</d><e></e></
  )";
  CHECK_FALSE(iguana::xml::from_xml(simple, str.data())); // expected >
  std::string str2 = R"(
    <simple_t><a>1</a><a>2</a><a>3</a><b>|</b><c>Flase</c><d>tru</d><e></e></simple_t>
  )";
  CHECK_NOTHROW(
      iguana::xml::from_xml(simple, str2.data())); // Failed to parse bool
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
