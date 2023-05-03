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

struct Owner_t {
  std::string ID;
  std::string DisplayName;
  auto operator==(const Owner_t &rhs) const {
    return ID == rhs.ID && DisplayName == rhs.DisplayName;
  }
};
REFLECTION(Owner_t, ID, DisplayName);

struct Contents {
  std::string Key;
  std::string LastModified;
  std::string ETag;
  std::string Type;
  uint32_t Size;
  std::string StorageClass;
  Owner_t Owner;

  auto operator==(const Contents &rhs) const {
    return Key == rhs.Key && LastModified == rhs.LastModified &&
           ETag == rhs.ETag && Type == rhs.Type && Size == rhs.Size &&
           StorageClass == rhs.StorageClass && Owner == rhs.Owner;
  }
};
REFLECTION(Contents, Key, LastModified, ETag, Type, Size, StorageClass, Owner);

TEST_CASE("test to xml") {
  Contents contents{"key", "ddd", "ccc", "aaa", 123, "aaa", {"bbb", "sss"}};

  // pretty xml
  std::string ss;
  iguana::xml::to_xml_pretty(ss, contents);
  std::string expected_xml_str = "<Contents>\n"
                                 "\t<Key>key</Key>\n"
                                 "\t<LastModified>ddd</LastModified>\n"
                                 "\t<ETag>ccc</ETag>\n"
                                 "\t<Type>aaa</Type>\n"
                                 "\t<Size>123</Size>\n"
                                 "\t<StorageClass>aaa</StorageClass>\n"
                                 "\t<Owner>\n"
                                 "\t\t<ID>bbb</ID>\n"
                                 "\t\t<DisplayName>sss</DisplayName>\n"
                                 "\t</Owner>\n"
                                 "</Contents>\n\n";
  CHECK(ss == expected_xml_str);

  // non pretty xml
  std::string s;
  iguana::xml::to_xml(s, contents);
  CHECK(
      s ==
      R"(<Contents><Key>key</Key><LastModified>ddd</LastModified><ETag>ccc</ETag><Type>aaa</Type><Size>123</Size><StorageClass>aaa</StorageClass><Owner><ID>bbb</ID><DisplayName>sss</DisplayName></Owner></Contents>)");
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

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP