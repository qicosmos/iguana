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

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP