#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/pb_reader.hpp"
#include "iguana/pb_writer.hpp"

struct point_t {
  int x;
  double y;
};
REFLECTION(point_t, x, y);

namespace my_space {
struct inner_struct {
  int x;
  int y;
  int z;
};

constexpr inline auto get_members_impl(inner_struct *) {
  return std::make_tuple(iguana::field_t{&inner_struct::x, 7, "a"},
                         iguana::field_t{&inner_struct::y, 9, "b"},
                         iguana::field_t{&inner_struct::z, 12, "c"});
}

inline constexpr size_t iguana_member_count(inner_struct *) { return 3; }

std::integral_constant<size_t, 3> member_count(inner_struct);

}  // namespace my_space

struct test_pb_st1 {
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
};
REFLECTION(test_pb_st1, x, y, z);

struct test_pb_st2 {
  int x;
  iguana::fixed32_t y;
  iguana::fixed64_t z;
};
REFLECTION(test_pb_st2, x, y, z);

struct test_pb_st3 {
  int x;
  iguana::sfixed32_t y;
  iguana::sfixed64_t z;
};
REFLECTION(test_pb_st3, x, y, z);

struct test_pb_st4 {
  int x;
  std::string y;
};
REFLECTION(test_pb_st4, x, y);

struct test_pb_st5 {
  int x;
  std::string_view y;
};
REFLECTION(test_pb_st5, x, y);

struct test_pb_st6 {
  std::optional<int> x;
  std::optional<std::string> y;
};
REFLECTION(test_pb_st6, x, y);

using Variant1 = std::variant<std::string, int>;
struct test_pb_st7 {
  iguana::one_of_t<0, Variant1> x;
  iguana::one_of_t<1, Variant1> y;
};
REFLECTION(test_pb_st7, x, y);

struct pair_t {
  int x;
  int y;
};
REFLECTION(pair_t, x, y);

struct message_t {
  int id;
  pair_t t;
};
REFLECTION(message_t, id, t);

struct test_pb_st8 {
  int x;
  pair_t y;
  message_t z;
};
REFLECTION(test_pb_st8, x, y, z);

struct test_pb_st9 {
  int x;
  std::vector<int> y;
  std::string z;
};
REFLECTION(test_pb_st9, x, y, z);

struct test_pb_st10 {
  int x;
  std::vector<message_t> y;
  std::string z;
};
REFLECTION(test_pb_st10, x, y, z);

struct test_pb_st11 {
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
REFLECTION(test_pb_st11, x, y, z);

struct test_pb_st12 {
  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
REFLECTION(test_pb_st12, x, y, z);

struct test_pb_st13 {
  int x;
  std::map<int, message_t> y;
  std::string z;
};
REFLECTION(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 {
  int x;
  colors_t y;
  level_t z;
};
REFLECTION(test_pb_st14, x, y, z);

namespace client {
struct person {
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
}  // namespace client

struct my_struct {
  int x;
  bool y;
  iguana::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest1 {
  std::string name;
  my_struct value;
  int var;
};

REFLECTION(nest1, name, value, var);

struct numer_st {
  bool a;
  double b;
  float c;
};
REFLECTION(numer_st, a, b, c);

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{41, 42, 43};

    std::string str;
    iguana::to_pb(inner, str);
    CHECK(str.size() == iguana::detail::pb_item_size(inner));

    my_space::inner_struct inner1;
    iguana::from_pb(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);
  }

  {
    test_pb_st1 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

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
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st2 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st3 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st4 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st4 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st5 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st6 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // variant
    test_pb_st7 st1{{"test"}};
    std::string str;
    iguana::to_pb(st1, str);
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

    test_pb_st7 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.x.value == st2.x.value);
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
    CHECK(str.size() == iguana::detail::pb_item_size(st1));

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
    message_t st1{};
    std::string str;
    iguana::to_pb(st1, str);

    message_t st2{};
    iguana::from_pb(st2, str);
    CHECK(st1.id == st2.id);
  }
  {
    test_pb_st11 st1{1, {{{5, {7, 8}}}, {{9, {11, 12}}}}, {"test"}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
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
    iguana::to_pb(st1, str);

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
  auto &val = arr1.at(0);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
