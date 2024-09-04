#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#undef THROW_UNKNOWN_KEY
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <optional>
#include <vector>

#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"

struct product_t {
  std::string_view name;
  float price;
  std::optional<std::string> description;
};
YLT_REFL(product_t, name, price, description);
struct store_t {
  std::string name;
  std::string_view location;
  std::vector<product_t> products;
};
YLT_REFL(store_t, name, location, products);
struct store_example_t {
  store_t store;
};
YLT_REFL(store_example_t, store);
TEST_CASE("test nothrow unknown key") {
  std::string str = R"(
unkonwnkey: aabb
store:
  name: "\u6c38\u8f89\u8d85\u5e02\t"
  location: Chengdu
  unkonwnkey: aabb
  products:
    - name: iPad
      price: 
        899.4
      unkonwnkey: aabb
      description: >
        nice
        ipad
    - name: watch
      price: 488.8
      description: |
        cheap watch
    - name: iPhone
      price: 999.99
      description: >-   
        expensive
        iphone
  )";
  store_example_t store_1;
  iguana::from_yaml(store_1, str);
  auto store = store_1.store;
  CHECK(store.name == "永辉超市\t");
  CHECK(store.location == "Chengdu");
  CHECK(store.products[0].name == "iPad");
  CHECK(store.products[0].price == 899.4f);
  CHECK(store.products[0].description == "nice ipad\n");
  CHECK(store.products[1].name == "watch");
  CHECK(store.products[1].price == 488.8f);
  CHECK(store.products[1].description == "cheap watch\n");
  CHECK(store.products[2].name == "iPhone");
  CHECK(store.products[2].price == 999.99f);
  CHECK(store.products[2].description == "expensive iphone");
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
