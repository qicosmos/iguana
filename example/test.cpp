#include "iguana/json_reader.hpp"
#include <iguana/json.hpp>
#include <iguana/json_util.hpp>
#include <iostream>
#include <optional>

struct point_t {
  int x;
  double y;
};
REFLECTION(point_t, x, y);

struct person {
  std::string name;
  bool ok;
};
REFLECTION(person, name, ok);

struct bool_t {
  bool ok;
};
REFLECTION(bool_t, ok);

struct optional_t {
  std::optional<bool> p;
};
REFLECTION(optional_t, p);

struct char_t {
  char ch;
};
REFLECTION(char_t, ch);

// nested object
struct simple_t {
  int id;
  person p;
};
REFLECTION(simple_t, id, p);

// c array
struct arr_t {
  int a[2];
  //    std::vector<std::string> v;
};
REFLECTION(arr_t, a);
// std array TODO

// vector
struct vector_t {
  std::vector<int> v;
};
REFLECTION(vector_t, v);

// map TODO

int main() {
  {
    try {
      vector_t arr{{1, 2}};
      iguana::string_stream ss;
      iguana::json::to_json(ss, arr);
      vector_t p{};
      std::string str = ss.str();
      iguana::from_json(p, std::begin(str),
                                               std::end(str));
      std::cout << "test ok\n";
    } catch (std::exception &e) {
      std::cout << "error: " << e.what() << "\n";
    }
  }
  {
    arr_t arr{{1, 2}};
    iguana::string_stream ss;
    iguana::json::to_json(ss, arr);
    arr_t p{};
    std::string str = ss.str();
    iguana::from_json(p, std::begin(str), std::end(str));
    std::cout << "\n";
  }
  {
    simple_t t{1, {.name = "tom", .ok = false}};
    iguana::string_stream ss;
    iguana::json::to_json(ss, t);
    simple_t p{};
    std::string str = ss.str();
    iguana::from_json(p, std::begin(str), std::end(str));
    std::cout << p.id << "\n";
  }

  {
    optional_t p{};
    std::string str = R"({"p": false})";
    iguana::from_json(p, std::begin(str),
                                               std::end(str));
  }
  {
    char_t p{};
    std::string str = R"({"ch": "t"})";
    iguana::from_json(p, std::begin(str), std::end(str));
  }

  {
    bool_t p{};
    std::string str = R"({"ok": true})";
    iguana::from_json(p, std::begin(str), std::end(str));
  }
  {
    person p{};
    std::string str = R"({"name" : "tom", "ok": true})";
    iguana::from_json(p, std::begin(str), std::end(str));

    person p1{};
    std::string str1 = R"({"ok": true, "name" : "tom"})";
    iguana::from_json(p1, std::begin(str1),
                                           std::end(str1));
    std::cout << p1.name << " " << p1.ok << "\n";
  }

  {
    point_t p{};
    std::string str = R"({"x" : 1, "y" : 2})";
    iguana::from_json(p, std::begin(str), std::end(str));
  }

  //  using value_type = std::variant<int point_t::*, int point_t::*>;
  //  constexpr auto map = iguana::get_iguana_struct_map<point_t>();
  //  static_assert(map.size() == 2);
  //  static_assert(map.at("x") ==
  //                value_type{std::in_place_index_t<0>{}, &point_t::x});
  //  static_assert(map.at("y") ==
  //                value_type{std::in_place_index_t<1>{}, &point_t::y});
}