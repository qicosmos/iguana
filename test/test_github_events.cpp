#include <string>
#include <vector>

#include "iguana/value.hpp"
#define DOCTEST_CONFIG_IMPLEMENT
#include <iguana/json_reader.hpp>
#include <iguana/json_util.hpp>

#include "doctest.h"
#include "github_events_schema.h"

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

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007) int main(int argc, char **argv) {
  return doctest::Context(argc, argv).run();
}
DOCTEST_MSVC_SUPPRESS_WARNING_POP
