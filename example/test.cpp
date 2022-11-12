#include <iguana/json.hpp>
#include <iostream>

struct point_t {
  int x;
  int y;
};
REFLECTION(point_t, x, y);

int main() {
  using value_type = std::variant<int point_t::*, int point_t::*>;
  constexpr auto map = iguana::get_iguana_struct_map<point_t>();
  static_assert(map.size() == 2);
  static_assert(map.at("x") ==
                value_type{std::in_place_index_t<0>{}, &point_t::x});
  static_assert(map.at("y") ==
                value_type{std::in_place_index_t<1>{}, &point_t::y});
}