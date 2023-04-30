#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <iguana/xml_reader.hpp>
#include <iguana/xml_writer.hpp>
#include <rapidxml_print.hpp>

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

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP
