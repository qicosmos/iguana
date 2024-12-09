#include <filesystem>
#include <stdexcept>
#include <vector>

#include "iguana/value.hpp"
#define DOCTEST_CONFIG_IMPLEMENT
#include <filesystem>
#include <iguana/json_reader.hpp>
#include <iguana/json_util.hpp>
#include <iguana/json_writer.hpp>
#include <iostream>
#include <limits>

#include "doctest.h"
#include "test_headers.h"

TEST_CASE("test canada.json") {
  std::cout << std::filesystem::current_path().string() << "\n";
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

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, test_str.begin(), test_str.end()));

    auto &&features = val.at<iguana::jarray>("features");
    CHECK(features.size() == 1);
    CHECK(features[0].is_object());
    auto &&feature = features[0].get<iguana::jobject>();
    auto &&geometry = feature["geometry"].to_object();
    CHECK(geometry["coordinates"].is_array());
    auto &&coors = geometry["coordinates"].to_array();
    CHECK(coors.size() == 1);
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
  {
    auto &&content = iguana::json_file_content("../data/apache_builds.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    CHECK(val.at<int>("numExecutors") == 0);
    auto &&arr = val.at<iguana::jarray>(("jobs"));
    CHECK(arr.size() == 875);
  }
}

TEST_CASE("test numbers.json") {
  {
    std::string test_str = R"(
          [
           0.655561997649,
           0.54153630768
          ]
      )";

    std::vector<double> numbers;
    iguana::from_json(numbers, test_str);
    for (const auto &num : numbers) {
      std::cout << std::fixed << std::setprecision(12) << num << "\n";
    }
  }

  std::vector<double> numbers;
  iguana::from_json_file(numbers, "../data/numbers.json");

  iguana::string_stream ss;
  const std::string jsonName = "temp.json";
  iguana::to_json(numbers, ss);
  std::ofstream writeSteam(jsonName, std::ios::out);
  writeSteam << ss;
  writeSteam.flush();
  writeSteam.close();

  std::vector<double> dummy;
  iguana::from_json_file(dummy, jsonName);

  CHECK(dummy.size() == numbers.size());
  for (size_t i = 0; i < numbers.size(); i++) {
    CHECK(numbers[i] - dummy[i] < std::numeric_limits<double>::epsilon());
  }
  if (std::filesystem::exists(jsonName))
    std::filesystem::remove(jsonName);

  {
    auto &&content = iguana::json_file_content("../data/numbers.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    CHECK(val.to_array().size() == numbers.size());
  }
}

TEST_CASE("test citm_catalog.json") {
  citm_object_t t;
  iguana::from_json_file(t, "../data/citm_catalog.json");
  CHECK(t.venueNames.value().PLEYEL_PLEYEL == "Salle Pleyel");

  {
    std::string test_str = R"({
      "events": {
        "138586341": {
          "description": null,
          "id": 138586341,
          "logo": null,
          "name": "30th Anniversary Tour",
          "subTopicIds": [
            337184269,
            337184283
          ],
          "subjectCode": null,
          "subtitle": null,
          "topicIds": [
            324846099,
            107888604
          ]
        }
      }
    })";

    auto &&content = iguana::json_file_content("../data/citm_catalog.json");

    iguana::jvalue val;
    iguana::parse(val, content);
    CHECK(val.at<iguana::jarray>("performances").size() == 243);
  }
}

TEST_CASE("test gsoc-2018.json") {
  gsoc_object_t t;
  iguana::from_json_file(t, "../data/gsoc-2018.json");
  auto last = std::rbegin(t);
  CHECK(last->second.author.type == "Person");
  CHECK(last->second.author.name == "Oleg Serikov");

  {
    auto &&content = iguana::json_file_content("../data/gsoc-2018.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto &&map = val.to_object();
    auto &&last = map.rbegin();
    CHECK(last->second.at<std::string>("@type") == "SoftwareSourceCode");
  }
}

TEST_CASE("test mesh.pretty.json") {
  mesh_t t;
  iguana::from_json_file(t, "../data/mesh.pretty.json");
  CHECK(t.tex0.back() == 0);

  {
    auto &&content = iguana::json_file_content("../data/mesh.pretty.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto arr = val.at<iguana::jarray>("positions");
    CHECK(arr.back().is_number());
  }
}

TEST_CASE("test random.json") {
  random_t t;
  iguana::from_json_file(t, "../data/random.json");
  CHECK(t.result.back().id == 1000);
  CHECK(t.result.back().age == 32);

  {
    auto &&content = iguana::json_file_content("../data/random.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto &&arr = val.at<iguana::jarray>("result");
    CHECK(arr.back().is_object());
    auto &&res = arr.back().to_object();
    CHECK(res.at("admin").is_bool());
  }
}

TEST_CASE("test github_events.json") {
  {
    std::string one_event = R"([
    {
      "type": "PushEvent",
      "created_at": "2013-01-10T07:58:30Z",
      "actor": {
        "gravatar_id": "a7cec1f75a06a5f8ab53139515da5d99",
        "login": "jathanism",
        "avatar_url": "https://secure.gravatar.com/avatar/a7cec1f75a06a5f8ab53139515da5d99?d=https://a248.e.akamai.net/assets.github.com%2Fimages%2Fgravatars%2Fgravatar-user-420.png",
        "url": "https://api.github.com/users/jathanism",
        "id": 138052
      },
      "repo": {
        "url": "https://api.github.com/repos/jathanism/trigger",
        "id": 6357414,
        "name": "jathanism/trigger"
      },
      "public": true,
      "payload": {
        "commits": [
          {
            "url": "https://api.github.com/repos/jathanism/trigger/commits/05570a3080693f6e55244e012b3b1ec59516c01b",
            "message": "- SSH Channel data now initialized in base class (TriggerSSHChannelBase)\n- New doc w/ checklist for adding new vendor support to Trigger.",
            "distinct": true,
            "sha": "05570a3080693f6e55244e012b3b1ec59516c01b",
            "author": {
              "email": "jathanism@aol.com",
              "name": "jathanism"
            }
          }
        ],
        "distinct_size": 1,
        "ref": "refs/heads/issue-22",
        "push_id": 134107894,
        "head": "05570a3080693f6e55244e012b3b1ec59516c01b",
        "before": "7460e1588817b3f885fb4ec76ec2f08c7caf6385",
        "size": 1
      },
      "id": "1652857722"
    }])";
    std::vector<githubEvents::event_t> events;
    iguana::from_json(events, one_event);
    CHECK(events.size() == 1);

    auto &event = events.front();
    CHECK(event.type == "PushEvent");
    CHECK(event.created_at == "2013-01-10T07:58:30Z");
    CHECK(event.actor);
    CHECK(event.actor.value().gravatar_id ==
          "a7cec1f75a06a5f8ab53139515da5d99");
    CHECK(event.repo.url == "https://api.github.com/repos/jathanism/trigger");
    CHECK(event.___public == true);
    CHECK(event.id == "1652857722");
  }
  std::vector<githubEvents::event_t> events;
  iguana::from_json_file(events, "../data/github_events.json");

  {
    auto &&content = iguana::json_file_content("../data/github_events.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto &&arr = val.to_array();
    auto &&last = arr.rbegin();
    auto &&actor = last->at<iguana::jobject>("actor");
    CHECK(!actor["id"].is_double());
  }
}

TEST_CASE("test marine_ik.json") {
  marine_ik::marine_ik_t t;
  iguana::from_json_file(t, "../data/marine_ik.json");

  CHECK(t.animations.size() == 1);
  CHECK(t.animations[0].tracks.empty());

  CHECK(t.object.children[0].matrix[0] == 1);
  CHECK(t.geometries[0].uuid == "C5CA037C-30C8-3A8C-9678-8A4BF32D5D85");

  {
    auto &&content = iguana::json_file_content("../data/marine_ik.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto &&meta = val.at<iguana::jobject>("metadata");
    CHECK(meta["version"].to_double() == 4.4);
  }
}

TEST_CASE("test instruments.json") {
  instruments_t t;
  iguana::from_json_file(t, "../data/instruments.json");
  CHECK(t.name == "epanos");

  {
    auto &&content = iguana::json_file_content("../data/instruments.json");

    iguana::jvalue val;
    CHECK_NOTHROW(iguana::parse(val, content));
    auto &&patterns = val.at<iguana::jarray>("patterns");
    CHECK(patterns[0].to_object().at("data").is_null());
  }
}

struct test_optstr_reader_null {
  std::optional<std::string> name;
};
YLT_REFL(test_optstr_reader_null, name);
TEST_CASE("test_optstr_reader") {
  test_optstr_reader_null v;
  v.name = "name";  // optional<string> begin with 'n'
  std::string json;
  iguana::to_json(v, json);

  test_optstr_reader_null v1;
  iguana::from_json(v1, json);
  CHECK(v.name == v1.name);
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP
