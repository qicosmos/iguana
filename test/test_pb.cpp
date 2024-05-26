#include <iostream>
#include <memory>

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/pb_reader.hpp"
#include "iguana/pb_writer.hpp"

void print_hex_str(const std::string &str) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : str) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  std::cout << oss.str() << std::endl;
}

#define BASE(T)
#define PUBLIC(T) : public iguana::pb_base_impl<T>

struct point_t BASE(point_t) {
  point_t() = default;
  point_t(int a, double b) : x(a), y(b) {}
  int x;
  double y;
};
REFLECTION(point_t, x, y);

namespace my_space {
struct inner_struct BASE(inner_struct) {
  inner_struct() = default;
  inner_struct(int a, int b, int c) : x(a), y(b), z(c) {}
  int x;
  int y;
  int z;
};

constexpr inline auto get_members_impl(inner_struct *) {
  return std::make_tuple(iguana::field_t{&inner_struct::x, 7, "a"},
                         iguana::field_t{&inner_struct::y, 9, "b"},
                         iguana::field_t{&inner_struct::z, 12, "c"});
}
}  // namespace my_space

struct test_pb_st1 BASE(test_pb_st1) {
  test_pb_st1() = default;
  test_pb_st1(int a, iguana::sint32_t b, iguana::sint64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
};
REFLECTION(test_pb_st1, x, y, z);

struct test_pb_sts PUBLIC(test_pb_sts) {
  test_pb_sts() = default;
  test_pb_sts(std::vector<test_pb_st1> l) : list(std::move(l)) {}
  std::vector<test_pb_st1> list;
};
REFLECTION(test_pb_sts, list);

struct test_pb_st2 BASE(test_pb_st2) {
  test_pb_st2() = default;
  test_pb_st2(int a, iguana::fixed32_t b, iguana::fixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::fixed32_t y;
  iguana::fixed64_t z;
};
REFLECTION(test_pb_st2, x, y, z);

struct test_pb_st3 BASE(test_pb_st3) {
  test_pb_st3() = default;
  test_pb_st3(int a, iguana::sfixed32_t b, iguana::sfixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sfixed32_t y;
  iguana::sfixed64_t z;
};
REFLECTION(test_pb_st3, x, y, z);

struct test_pb_st4 BASE(test_pb_st3) {
  test_pb_st4() = default;
  test_pb_st4(int a, std::string b) : x(a), y(std::move(b)) {}
  int x;
  std::string y;
};
REFLECTION(test_pb_st4, x, y);

struct test_pb_st5 BASE(test_pb_st5) {
  test_pb_st5() = default;
  test_pb_st5(int a, std::string_view b) : x(a), y(b) {}
  int x;
  std::string_view y;
};
REFLECTION(test_pb_st5, x, y);

struct test_pb_st6 BASE(test_pb_st6) {
  test_pb_st6() = default;
  test_pb_st6(std::optional<int> a, std::optional<std::string> b)
      : x(std::move(a)), y(std::move(b)) {}
  std::optional<int> x;
  std::optional<std::string> y;
};
REFLECTION(test_pb_st6, x, y);

struct pair_t BASE(pair_t) {
  pair_t() = default;
  pair_t(int a, int b) : x(a), y(b) {}
  int x;
  int y;
};
REFLECTION(pair_t, x, y);

struct message_t PUBLIC(message_t) {
  message_t() = default;
  message_t(int a, pair_t b) : id(a), t(b) {}
  int id;
  pair_t t;
};
REFLECTION(message_t, id, t);

struct test_pb_st8 BASE(test_pb_st8) {
  test_pb_st8() = default;
  test_pb_st8(int a, pair_t b, message_t c) : x(a), y(b), z(c) {}

  int x;
  pair_t y;
  message_t z;
};
REFLECTION(test_pb_st8, x, y, z);

struct test_pb_st9 BASE(test_pb_st9) {
  test_pb_st9() = default;
  test_pb_st9(int a, std::vector<int> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<int> y;
  std::string z;
};
REFLECTION(test_pb_st9, x, y, z);

struct test_pb_st10 BASE(test_pb_st10) {
  test_pb_st10() = default;
  test_pb_st10(int a, std::vector<message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<message_t> y;
  std::string z;
};
REFLECTION(test_pb_st10, x, y, z);

struct test_pb_st11 PUBLIC(test_pb_st11) {
  test_pb_st11() = default;
  test_pb_st11(int a, std::vector<std::optional<message_t>> b,
               std::vector<std::string> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
REFLECTION(test_pb_st11, x, y, z);

struct test_pb_st12 BASE(test_pb_st12) {
  test_pb_st12() = default;
  test_pb_st12(int a, std::map<int, std::string> b,
               std::map<std::string, int> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
REFLECTION(test_pb_st12, x, y, z);

struct test_pb_st13 BASE(test_pb_st13) {
  test_pb_st13() = default;
  test_pb_st13(int a, std::map<int, message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, message_t> y;
  std::string z;
};
REFLECTION(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 BASE(test_pb_st14) {
  test_pb_st14() = default;
  test_pb_st14(int a, colors_t b, level_t c) : x(a), y(b), z(c) {}
  int x;
  colors_t y;
  level_t z;
};
REFLECTION(test_pb_st14, x, y, z);

namespace client {
struct person BASE(person) {
  person() = default;
  person(std::string s, int d) : name(s), age(d) {}
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
}  // namespace client

struct my_struct BASE(my_struct) {
  my_struct() = default;
  my_struct(int a, bool b, iguana::fixed64_t c) : x(a), y(b), z(c) {}
  int x;
  bool y;
  iguana::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest1 BASE(nest1) {
  nest1() = default;
  nest1(std::string s, my_struct t, int d)
      : name(std::move(s)), value(t), var(d) {}
  std::string name;
  my_struct value;
  int var;
};

REFLECTION(nest1, name, value, var);

struct numer_st BASE(numer_st) {
  numer_st() = default;
  numer_st(bool x, double y, float z) : a(x), b(y), c(z) {}
  bool a;
  double b;
  float c;
};
REFLECTION(numer_st, a, b, c);

TEST_CASE("test reflection") {
  {
    auto t = iguana::create_instance("pair_t");
    t->set_field_value("x", 12);
    t->set_field_value("y", 24);
    auto &r0 = t->get_field_value<int>("x");
    CHECK(r0 == 12);
    auto &r = t->get_field_value<int>("y");
    CHECK(r == 24);

    std::string str;
    t->to_pb(str);

    pair_t t1;
    t1.from_pb(str);

    pair_t *st = dynamic_cast<pair_t *>(t.get());
    CHECK(st->x == t1.x);
    CHECK(st->y == t1.y);
  }
  auto t = iguana::create_instance("numer_st");
  t->set_field_value("a", true);
  t->set_field_value("b", double(25));
  t->set_field_value("c", float(42));
  auto &r0 = t->get_field_value<bool>("a");
  CHECK(r0);
  auto &r = t->get_field_value<double>("b");
  CHECK(r == 25);
  auto &r1 = t->get_field_value<float>("c");
  CHECK(r1 == 42);

  numer_st *st = dynamic_cast<numer_st *>(t.get());
  CHECK(st->a == true);
  CHECK(st->b == 25);
  CHECK(st->c == 42);

  std::string str;
  t->to_pb(str);

  numer_st t1;
  t1.from_pb(str);

  CHECK(st->a == t1.a);
  CHECK(st->b == t1.b);
  CHECK(st->c == t1.c);
}

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{41, 42, 43};

    std::string str;
    iguana::to_pb(inner, str);

    my_space::inner_struct inner1;
    iguana::from_pb(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);
  }

  {
    test_pb_sts p{};
    p.list.push_back(test_pb_st1{41, {42}, {43}});
    std::string str;
    p.to_pb(str);

    test_pb_sts p1{};
    p1.from_pb(str);
    CHECK(p.list[0].z == p1.list[0].z);
  }

  {
    test_pb_st1 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st1 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.x == st2.x);
    CHECK(st1.y.val == st2.y.val);
    CHECK(st1.z.val == st2.z.val);
  }

  {
    test_pb_st2 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st2 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st3 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st4 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st4 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st5 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st6 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // sub nested objects
    nest1 v{"Hi", {1, false, {3}}, 5}, v2{};
    std::string s;
    iguana::to_pb(v, s);
    iguana::from_pb(v2, s);
    CHECK(v.var == v2.var);
    CHECK(v.value.y == v2.value.y);
    CHECK(v.value.z == v2.value.z);

    test_pb_st8 st1{1, {3, 4}, {5, {7, 8}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st8 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z.t.y == st2.z.t.y);
  }

  {
    // repeated messages
    test_pb_st9 st1{1, {2, 4, 6}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st9 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    test_pb_st10 st1{1, {{5, {7, 8}}, {9, {11, 12}}}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st10 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t m{1, {3, 4}};
    test_pb_st11 st1{1, {m}, {}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    auto m = std::make_shared<message_t>();
    m->id = 10;
    m->t.x = 11;
    m->t.y = 12;
    std::string s;
    m->to_pb(s);

    std::shared_ptr<iguana::pb_base> base = m;
    std::string str;
    base->to_pb(str);

    CHECK(s == str);

    base->from_pb(str);
  }
  {
    message_t st1{};
    std::string str;
    iguana::to_pb(st1, str);

    message_t st2{};
    iguana::from_pb(st2, str);
    CHECK(st1.id == st2.id);
  }
  {
    test_pb_st11 st1{
        1, {message_t{5, {7, 8}}, message_t{9, {11, 12}}}, {"test"}};
    std::string str;
    iguana::to_pb(st1, str);

    std::string str1;
    st1.to_pb(str1);

    test_pb_st11 st3;
    st3.from_pb(str1);

    CHECK(str == str1);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(st3.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, "test"}, {2, "ok"}}, {{"test", 4}, {"ok", 6}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st12 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, ""}, {0, "ok"}}, {{"", 4}, {"ok", 0}}};
    std::string str;
    iguana::to_pb(st1, str);  // error
    print_hex_str(str);
    test_pb_st12 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(1, message_t{2, {3, 4}});
    st1.y.emplace(2, message_t{4, {6, 8}});
    st1.z = "test";
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st13 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(2, message_t{});
    st1.y.emplace(3, message_t{});
    st1.z = "test";
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st13 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // enum
    test_pb_st14 st1{1, colors_t::black, level_t::info};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st14 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // bool float double
    numer_st n{true, 10.25, 4.578}, n1;
    std::string str;
    iguana::to_pb(n, str);

    iguana::from_pb(n1, str);
    CHECK(n1.a == n.a);
    CHECK(n1.b == n.b);
    CHECK(n1.c == n.c);
  }
}

TEST_CASE("test members") {
  using namespace iguana;
  using namespace iguana::detail;

  my_space::inner_struct inner{41, 42, 43};
  const auto &map = iguana::get_members<my_space::inner_struct>();
  std::visit(
      [&inner](auto &member) mutable {
        CHECK(member.field_no == 9);
        CHECK(member.field_name == "b");
        CHECK(member.value(inner) == 42);
      },
      map.at(9));

  point_t pt{2, 3};
  const auto &arr1 = iguana::get_members<point_t>();
  auto &val = arr1.at(1);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

struct test_variant BASE(test_variant) {
  test_variant() = default;
  test_variant(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
REFLECTION(test_variant, x, y, z);

TEST_CASE("test variant") {
  {
    constexpr auto tp = iguana::get_members_tuple<test_variant>();
    static_assert(std::get<0>(tp).field_no == 1);
    static_assert(std::get<1>(tp).field_no == 2);
    static_assert(std::get<2>(tp).field_no == 3);
    static_assert(std::get<3>(tp).field_no == 4);
    static_assert(std::get<4>(tp).field_no == 5);
  }
  {
    constexpr static auto map = iguana::get_members<test_variant>();
    static_assert(map.find(1) != map.end());
    static_assert(map.find(2) != map.end());
    static_assert(map.find(3) != map.end());
    static_assert(map.find(4) != map.end());
    auto val1 = map.find(2);
    auto val2 = map.find(3);
    std::visit(
        [](auto &member) mutable {
          CHECK(member.field_no == 2);
          CHECK(member.field_name == "y");
        },
        val1->second);
    std::visit(
        [](auto &member) mutable {
          CHECK(member.field_no == 3);
          CHECK(member.field_name == "y");
        },
        val2->second);
  }
  {
    test_variant st1 = {5, "Hello, variant!", 3.14};
    std::string str;
    iguana::to_pb(st1, str);
    test_variant st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<std::string>(st2.y) == "Hello, variant!");
  }
  {
    test_variant st1 = {5, 3.88, 3.14};
    std::string str;
    iguana::to_pb(st1, str);
    test_variant st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<double>(st2.y) == 3.88);
  }
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
