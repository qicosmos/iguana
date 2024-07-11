#include <sstream>
#include <utility>

#include "iguana/ylt/reflection/member_names.hpp"
#include "iguana/ylt/reflection/member_value.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)

using namespace ylt::reflection;

struct sub {
  int id;
};

enum class Color { red, black };

struct person {
  Color color;
  int id;
  sub s;
  std::string str;
  int arr[2];
};

TEST_CASE("test member names") {
  constexpr size_t size = members_count_v<person>;
  CHECK(size == 5);
  constexpr auto tp = struct_to_tuple<person>();
  constexpr size_t tp_size = std::tuple_size_v<decltype(tp)>;
  CHECK(tp_size == 5);

  constexpr auto arr = get_member_names<person>();
  for (auto name : arr) {
    std::cout << name << ", ";
  }
  std::cout << "\n";
  CHECK(arr ==
        std::array<std::string_view, size>{"color", "id", "s", "str", "arr"});
}

struct simple {
  int color;
  int id;
  std::string str;
  int age;
};

TEST_CASE("test member value") {
  simple p{.color = 2, .id = 10, .str = "hello reflection", .age = 6};
  auto ptr_tp = object_to_tuple(p);
  constexpr auto arr = get_member_names<simple>();
  std::stringstream out;
  [&]<size_t... Is>(std::index_sequence<Is...>) {
    ((out << "name: " << arr[Is] << ", value: " << *std::get<Is>(ptr_tp)
          << "\n"),
     ...);
  }
  (std::make_index_sequence<arr.size()>{});

  std::string result = out.str();
  std::cout << out.str();

  std::string expected_str =
      "name: color, value: 2\nname: id, value: 10\nname: str, value: hello "
      "reflection\nname: age, value: 6\n";
  CHECK(result == expected_str);

  constexpr auto map = get_member_names_map<simple>();
  constexpr size_t index = map.at("age");
  CHECK(index == 3);
  auto age = *std::get<index>(ptr_tp);
  CHECK(age == 6);

  auto& age1 = get_member_by_name<int, "age"_ylts>(p);
  CHECK(age1 == 6);

  auto& str = get_member_by_name<std::string, "str"_ylts>(p);
  CHECK(str == "hello reflection");

  auto age2 = get_member_by_index<int>(p, 3);
  CHECK(age2 == 6);

  auto str2 = get_member_by_index<std::string, 2>(p);
  CHECK(str2 == "hello reflection");
}

#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
