#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#undef THROW_UNKNOWN_KEY
#include "iguana/xml_reader.hpp"
#include "iguana/xml_writer.hpp"
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <optional>
#include <vector>

enum class enum_status {
  paid,
  unpaid,
};
struct order_t {
  enum_status status;
  float total;
};
REFLECTION(order_t, status, total);

TEST_CASE("test unkonwn key") {
  auto validator = [] (order_t od) {
    CHECK(od.status == enum_status::unpaid);
    CHECK(od.total == 65.0f);
  };
  std::string str = R"(
<order_t>
  <orderID>12345</orderID>
  <status>1</status>
  <phone />
  <customer>
    <firstName>John</firstName>
    <![CDATA[ node2</p>]]>
    <lastName>Doe</lastName>
    <address>
      <street>123 Main St</street>
      <city>Anytown</city>
      <state>CA</state>
      <zip>12345</zip>
    </address>
  </customer>
  <items>
    <item>
      <productID>67890</productID>
      <name>Widget A</name>
      <quantity>2</quantity>
      <price>10.00</price>
    </item>
    <item>
      <productID>98765</productID>
      <name>Widget B</name>
      <quantity>3</quantity>
      <price>15.00</price>
    </item>
  </items>
  <![CDATA[ node2</p>]]>
  <total>65.00</total>
</order_t>
  )";
  order_t od;
  iguana::from_xml(od, str);
  validator(od);

  std::string ss;
  iguana::to_xml(od, ss);
  order_t od1;
  iguana::from_xml(od1, ss);
  validator(od1);
}



// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP

