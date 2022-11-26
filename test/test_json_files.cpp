#include "iguana/reflection.hpp"
#include <stdexcept>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/json_reader.hpp"
#include "test_headers.h"
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
#include <iostream>

TEST_CASE("test canada.json") {

  {
    std::string test_str = R"({
    "type": "FeatureCollection",
    "features": [{
      "type": "Feature",
      "properties": {
        "name": "Canada"
      },
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [
            [-65.613616999999977,
              43.420273000000009
            ]
          ]
        ]
      }
      }]
    })";

    FeatureCollection p;
    iguana::from_json(p, test_str);
    std::cout << p.type << "\n";
  }

  FeatureCollection t;
  iguana::from_json_file(t, "../data/canada.json");
  std::cout << t.type << "\n";
}

TEST_CASE("test apache_builds.json") {
  apache_builds t;
  iguana::from_json_file(t, "../data/apache_builds.json");
  std::cout << t.description << "\n";
  for (auto &v : t.views) {
    std::cout << v.name << ", " << v.url << "\n";
  }
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP