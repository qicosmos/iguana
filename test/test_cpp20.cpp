#define DOCTEST_CONFIG_IMPLEMENT
#define SEQUENTIAL_PARSE
#include "doctest.h"
#include "iguana/iguana.hpp"

using namespace ylt::reflection;

struct point_t {
  int x;
  int y;
};
REQUIRED(point_t, x, y);
#if __cplusplus < 202002L
YLT_REFL(point_t, x, y);
#endif

struct test_variant {
  test_variant() = default;
  test_variant(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
#if __cplusplus < 202002L
YLT_REFL(test_variant, x, y, z);
#endif

struct test_variant1 {
  std::variant<double, std::string, int> x;
  int y;
  double z;
};
YLT_REFL(test_variant1, x, y, z);

struct test_variant2 {
  int x;
  double y;
  std::variant<double, std::string, int> z;
};
YLT_REFL(test_variant2, x, y, z);

struct test_variant3 {
  int x;
  std::variant<double, std::string, int> y;
  int z;
  std::variant<double, std::string, int> a;
  double b;
  double c;
};
YLT_REFL(test_variant3, x, y, z, a, b, c);

struct test_variant4 {
  test_variant4() = default;
  test_variant4(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}

#if __cplusplus < 202002L
  YLT_REFL(test_variant4, x, y, z);
#endif

 private:
  int x;
  std::variant<double, std::string, int> y;
  double z;
};

TEST_CASE("test pb") {
  {
    using Tuple1 = decltype(ylt::reflection::object_to_tuple(
        std::declval<test_variant4>()));
    std::cout << type_string<Tuple1>() << "\n";

    test_variant4 t(1, "test", 3);
    auto tp = iguana::detail::get_pb_members_tuple(t);

    std::string proto;
    iguana::to_proto<test_variant4>(proto);
    std::cout << proto;

    static_assert(std::get<0>(tp).field_no == 1, "err");

    CHECK(std::get<0>(tp).field_no == 1);
    CHECK(std::get<1>(tp).field_no == 2);
    CHECK(std::get<2>(tp).field_no == 3);
    CHECK(std::get<3>(tp).field_no == 4);
    CHECK(std::get<4>(tp).field_no == 5);

    std::string str;
    iguana::to_pb(t, str);

    test_variant4 t1;
    iguana::from_pb(t1, str);
    std::cout << "\n";
  }
  {
    test_variant t(1, "test", 3);
    auto tp = iguana::detail::get_pb_members_tuple(t);

    CHECK(std::get<0>(tp).field_no == 1);
    CHECK(std::get<1>(tp).field_no == 2);
    CHECK(std::get<2>(tp).field_no == 3);
    CHECK(std::get<3>(tp).field_no == 4);
    CHECK(std::get<4>(tp).field_no == 5);
  }

  {
    test_variant1 t{"test", 2, 3};
    auto tp = iguana::detail::get_pb_members_tuple(t);

    CHECK(std::get<0>(tp).field_no == 1);
    CHECK(std::get<1>(tp).field_no == 2);
    CHECK(std::get<2>(tp).field_no == 3);
    CHECK(std::get<3>(tp).field_no == 4);
    CHECK(std::get<4>(tp).field_no == 5);
  }

  {
    test_variant2 t{2, 3, "test"};
    auto tp = iguana::detail::get_pb_members_tuple(t);

    CHECK(std::get<0>(tp).field_no == 1);
    CHECK(std::get<1>(tp).field_no == 2);
    CHECK(std::get<2>(tp).field_no == 3);
    CHECK(std::get<3>(tp).field_no == 4);
    CHECK(std::get<4>(tp).field_no == 5);
  }

  {
    test_variant3 t{2, "test", 3, "ok", 5, 6};
    auto tp = iguana::detail::get_pb_members_tuple(t);

    CHECK(std::get<0>(tp).field_no == 1);
    CHECK(std::get<1>(tp).field_no == 2);
    CHECK(std::get<2>(tp).field_no == 3);
    CHECK(std::get<3>(tp).field_no == 4);
    CHECK(std::get<4>(tp).field_no == 5);
    CHECK(std::get<5>(tp).field_no == 6);
    CHECK(std::get<6>(tp).field_no == 7);
    CHECK(std::get<7>(tp).field_no == 8);
    CHECK(std::get<8>(tp).field_no == 9);
    CHECK(std::get<9>(tp).field_no == 10);

    std::string str;
    iguana::to_pb(t, str);
    std::cout << str.size() << "\n";

    test_variant3 pt1;
    iguana::from_pb(pt1, str);
    std::cout << "\n";
  }

  point_t pt{1, 2};
  std::string str;

  iguana::to_pb(pt, str);
  std::cout << str.size() << "\n";

  point_t pt1;
  iguana::from_pb(pt1, str);

  std::string proto;
  iguana::to_proto<test_variant3>(proto);
  std::cout << proto;
}

TEST_CASE("test simple") {
  point_t pt{1, 2};
  std::string str;
  static_assert(iguana::ylt_refletable_v<point_t>, "e");
  iguana::to_json(pt, str);
  std::cout << str << "\n";
  auto map = ylt::reflection::get_variant_map(pt);
  for (auto& [key, var] : map) {
    std::cout << key.data() << "\n";
    std::visit(
        [](auto ptr) {
          std::cout << *ptr << "\n";
        },
        var);
  }

  point_t pt1;
  iguana::from_json(pt1, str);
  std::cout << pt1.x << "\n";
}

struct description_t {
  iguana::xml_cdata_t<std::string> cdata;
};
YLT_REFL(description_t, cdata);
struct node_t {
  std::string title;
  description_t description;
  iguana::xml_cdata_t<> cdata;
};
YLT_REFL(node_t, title /*, description, cdata*/);
TEST_CASE("test example cdata") {
  auto validator = [](node_t node) {
    CHECK(node.title == "what's the cdata");
    CHECK(node.cdata.value() == "<p>this is a  cdata node</p>");
    CHECK(node.description.cdata.value() ==
          "<p>nest cdata node1 and </p>node2</p>");
  };
  std::string str = R"(
    <?xml version="1.0" encoding="UTF-8"?>
    <node_t>
      <title>what's the cdata</title>
      <description>
        <![CDATA[<p>nest cdata node1 and </p>]]>
        <!-- This is a comment -->
        <![CDATA[ node2</p>]]>
      </description>
      <!DOCTYPE test node>
      <?myapp instruction?>
      <!-- <price>3.25</price> -->
      <!-- <price a="b"/> -->
      <![CDATA[<p>this is a  cdata node</p>]]>
    </node_t>
  )";
  // only parse cdata node
  node_t node;
  iguana::from_xml(node, str);
  validator(node);

  {
    std::string ss;
    iguana::to_xml(node, ss);
    node_t node1;
    iguana::from_xml(node1, ss);
    validator(node1);
  }
  {
    std::string ss;
    iguana::to_xml<true>(node, ss);
    node_t node1;
    iguana::from_xml(node1, ss);
    validator(node1);
  }
}

TEST_CASE("test xml") {
  point_t t{1, 3};
  std::string xml;
  iguana::to_xml(t, xml);
  std::cout << xml << "\n";

  point_t t1;
  iguana::from_xml(t1, xml.begin(), xml.end());
  std::cout << t1.x << "\n";
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP