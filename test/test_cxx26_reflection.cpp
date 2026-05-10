#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "iguana/iguana.hpp"

struct cxx26_person {
  int id;
  std::string name;
  int age;

  bool operator==(const cxx26_person& other) const {
    return id == other.id && name == other.name && age == other.age;
  }
};

struct cxx26_alias_person {
  int id;
  std::string name;

  static constexpr auto get_alias_field_names(cxx26_alias_person*) {
    return std::array{ylt::reflection::field_alias_t{"ID", 0},
                      ylt::reflection::field_alias_t{"Name", 1}};
  }
};

TEST_CASE("std meta basic reflection api") {
  static_assert(ylt::reflection::std_meta::enabled);
  static_assert(ylt::reflection::members_count_v<cxx26_person> == 3);

  constexpr auto names = ylt::reflection::get_member_names<cxx26_person>();
  CHECK(names == std::array<std::string_view, 3>{"id", "name", "age"});

  cxx26_person p{1, "tom", 20};
  int sum = 0;
  ylt::reflection::for_each(p, [&](auto& field, auto name, auto index) {
    if (name == "id") {
      CHECK(index == 0);
      sum += field;
    }
    else if (name == "age") {
      CHECK(index == 2);
      sum += field;
    }
  });
  CHECK(sum == 21);

  auto tp = ylt::reflection::object_to_tuple(p);
  std::get<1>(tp) = "jerry";
  CHECK(p.name == "jerry");
}

TEST_CASE("std meta serialization") {
  cxx26_person p{1, "tom", 20};
  std::string json;
  iguana::to_json(p, json);
  CHECK(json == R"({"id":1,"name":"tom","age":20})");

  cxx26_person decoded{};
  iguana::from_json(decoded, json);
  CHECK(decoded == p);
}

TEST_CASE("std meta field aliases") {
  constexpr auto names =
      ylt::reflection::get_member_names<cxx26_alias_person>();
  CHECK(names == std::array<std::string_view, 2>{"ID", "Name"});

  cxx26_alias_person value{7, "neo"};
  std::string json;
  iguana::to_json(value, json);
  CHECK(json == R"({"ID":7,"Name":"neo"})");
}
