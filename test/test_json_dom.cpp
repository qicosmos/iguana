#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <sstream>

#include "doctest.h"
#include "iguana/json_dom.hpp"
#include "iguana/json_reader.hpp"

TEST_CASE("DOM parser reuses buffers across documents and parse errors") {
  iguana::json_parser parser;
  auto first = parser.parse(R"({"name":"Ada","roles":["reader"]})");
  iguana::json saved = first;
  CHECK(first["name"] == "Ada");

  auto second = parser.parse(R"({"name":"Lin","roles":[1,2,3]})");
  CHECK(second["name"] == "Lin");
  CHECK(second["roles"][2] == 3);
  CHECK(saved["name"] == "Ada");

  std::error_code ec;
  parser.parse(R"({"name":[1,{"roles":"old"},)", ec);
  CHECK(ec);
  CHECK_FALSE(parser.try_parse(R"({"name":1,"name":2})"));
  auto recovered = parser.parse(R"({"name":{"roles":"new"}})", ec);
  CHECK_FALSE(ec);
  CHECK(recovered["name"]["roles"] == "new");
  CHECK_THROWS_AS(parser.parse("[1] trailing"), iguana::json_parse_error);
  auto null = parser.try_parse("null");
  REQUIRE(null.has_value());
  CHECK(null->is_null());
}

TEST_CASE("DOM parser accepts input borrowed from its current document") {
  iguana::json_parser parser;
  auto text = parser.parse(R"("[1,2,3]")").get<std::string_view>();
  auto decoded = parser.parse(text);
  REQUIRE(decoded.is_array());
  CHECK(decoded[2] == 3);

  auto current = parser.parse(R"({"name":"Ada"})");
  auto moved = std::move(parser);
  CHECK(current["name"] == "Ada");
  CHECK(parser.parse("123") == 123);
  CHECK(current["name"] == "Ada");
  CHECK(moved.parse("false") == false);
}

TEST_CASE("DOM compares and prints nodes directly") {
  auto doc = iguana::json::parse(
      R"({"age":37,"name":"Ada","enabled":true,"large":18446744073709551615,"precise":9007199254740993,"min":-9223372036854775808})");
  CHECK(doc["name"] == "Ada");
  CHECK("Ada" == doc["name"]);
  CHECK(doc["name"] < std::string_view("Zoe"));
  CHECK(doc["name"] != 37);
  CHECK_FALSE(doc["name"] >= 37);
  CHECK(doc["age"] >= 18);
  CHECK(18 < doc["age"]);
  CHECK(doc["age"] == 37.0);
  CHECK(doc["age"] < 37.1);
  CHECK(doc["enabled"] == true);
  CHECK(doc["enabled"] != 1);
  CHECK(doc["large"] == std::numeric_limits<uint64_t>::max());
  CHECK(doc["large"] < 0x1p64);
  CHECK(doc["precise"] > 0x1p53);
  CHECK(doc["min"] == -0x1p63);
  CHECK(iguana::json::parse("null") == nullptr);
  CHECK(iguana::json::parse(R"({"a":[1,2],"b":true})") ==
        iguana::json::parse(R"({"b":true,"a":[1,2]})"));
  CHECK(iguana::json::parse("[1,2]") != iguana::json::parse("[2,1]"));

  std::ostringstream out;
  out << doc["name"] << ' ' << std::as_const(doc)["age"];
  CHECK(out.str() == "\"Ada\" 37");
}

TEST_CASE("DOM converts containers and accepts list assignments") {
  auto doc = iguana::json::parse(R"({"scores":[1,2],"stats":{"a":[3,4]}})");
  std::vector<int> scores = doc["scores"];
  CHECK(scores == std::vector<int>{1, 2});
  std::map<std::string, std::vector<int>> stats = doc["stats"];
  CHECK(stats.at("a") == std::vector<int>{3, 4});

  doc["scores"] = std::vector<int>{5, 6};
  doc["stats"] = std::map<std::string, std::vector<int>>{{"b", {7, 8}}};
  doc["tags"] = {"dev", "reviewer"};
  doc["entries"].push_back({{"value", 3}});
  doc["flags"] = std::vector<bool>{true, false};
  CHECK(doc["scores"].get<std::vector<int>>() == std::vector<int>{5, 6});
  CHECK(doc["stats"]["b"][0] == 7);
  CHECK(doc["tags"][1] == "reviewer");
  CHECK(doc["entries"][0]["value"] == 3);
  CHECK(doc["flags"].get<std::vector<bool>>() ==
        std::vector<bool>{true, false});
  CHECK_FALSE(doc["tags"].try_get<std::vector<int>>());
  CHECK_THROWS_AS(scores = doc["tags"], std::invalid_argument);
  scores = doc["scores"];
  CHECK(scores == std::vector<int>{5, 6});

  iguana::json array = std::vector<int>{1, 2};
  array.push_back({3, 4});
  CHECK(array[2][0] == 3);
  CHECK(array.contains(0));
  CHECK_FALSE(array.find(3));
  auto first = array.find(0);
  REQUIRE(first.has_value());
  *first = 10;
  CHECK(array[0] == 10);
  std::vector<int> owned = iguana::json::parse("[5,6]");
  CHECK(owned == std::vector<int>{5, 6});
}

TEST_CASE("DOM creates missing members on writes") {
  auto doc = iguana::json::parse(R"({"person":{},"nullable":null})");
  auto original = doc.dump();
  auto missing = doc["other"];
  CHECK(missing.is_missing());
  CHECK_FALSE(missing.is_null());
  CHECK_FALSE(missing.try_get<int>());
  CHECK_FALSE(missing.try_get<std::nullptr_t>());
  CHECK_THROWS_AS((void)static_cast<int>(missing), std::out_of_range);
  CHECK_THROWS_AS(doc.at("other"), std::out_of_range);
  CHECK_FALSE(doc["person"]["age"].try_get<int>());
  CHECK_FALSE(doc["nullable"]["child"].try_get<int>());
  CHECK_FALSE(doc["other"].erase("x"));
  CHECK(doc.dump() == original);

  auto later = doc[std::string("dynamic")]["child"];
  doc["dynamic"]["child"] = 42;
  CHECK(later == 42);
  later = 43;
  CHECK(doc["dynamic"]["child"] == 43);
  doc["nullable"]["child"] = nullptr;
  CHECK(doc["nullable"]["child"].is_null());
  doc["other"]["entries"].push_back(7);
  CHECK(doc["other"]["entries"][0] == 7);

  CHECK_THROWS_AS(doc["absent"] = std::numeric_limits<double>::infinity(),
                  std::invalid_argument);
  CHECK_THROWS_AS(doc["absent"] = doc["also_absent"], std::out_of_range);
  CHECK_FALSE(doc.contains("absent"));
  CHECK_FALSE(doc.contains("also_absent"));

  auto shadowed = doc["shadowed"]["child"];
  doc["shadowed"] = false;
  CHECK_THROWS_AS(shadowed = 1, std::invalid_argument);
  CHECK(doc["shadowed"] == false);

  std::string text(200, 'x');
  doc["text"] = text;
  doc["copy"] = doc["text"].get<std::string_view>();
  CHECK(doc["copy"] == text);
}

TEST_CASE("DOM iterates object entries and array values directly") {
  auto doc =
      iguana::json::parse(R"({"user":{"age":37,"name":"Ada"},"scores":[1,2]})");
  for (auto [key, value] : doc["user"]) {
    if (key == "age") {
      value = 38;
    }
  }
  auto& read = std::as_const(doc);
  size_t fields = 0;
  for (auto [key, value] : read.at("user")) {
    static_assert(std::is_same_v<decltype(value), iguana::json_view>);
    if (key == "age") {
      auto age = value.get<int>();
      CHECK(age == 38);
    }
    else if (key == "name") {
      auto name = value.get<std::string_view>();
      CHECK(name == "Ada");
    }
    ++fields;
  }
  CHECK(fields == 2);

  int sum = 0;
  for (int score : doc["scores"]) {
    sum += score;
  }
  CHECK(sum == 3);
  for (auto score : doc["scores"]) {
    score = 5;
  }
  CHECK(read["scores"][0].get<int>() == 5);
  CHECK(read["scores"][1].get<int>() == 5);

  size_t root_fields = 0;
  for (auto [key, value] : read) {
    auto has_key = read.contains(key);
    auto size = value.size();
    CHECK(has_key);
    CHECK(size == 2);
    ++root_fields;
  }
  CHECK(root_fields == 2);
}

TEST_CASE("DOM infers scalar conversion from the destination type") {
  auto doc = iguana::json::parse(
      R"({"name":"Ada","age":37,"enabled":true,"scores":[1,2.5],"large":18446744073709551615})");
  int age = doc["age"];
  std::string name = doc["name"];
  bool enabled = doc["enabled"];
  double score = doc["scores"][1];
  const auto& read = doc;
  std::string_view name_view = read["name"];
  int first_score = read["scores"][0];
  uint64_t large = read["large"];

  CHECK(age == 37);
  CHECK(name == "Ada");
  CHECK(enabled);
  CHECK(score == 2.5);
  CHECK(name_view == "Ada");
  CHECK(first_score == 1);
  CHECK(large == std::numeric_limits<uint64_t>::max());

  name.clear();
  age = 0;
  name = read["name"];
  age = doc["age"];
  CHECK(name == "Ada");
  CHECK(age == 37);
  const char* key = "age";
  int from_pointer = read[key];
  CHECK(from_pointer == age);

  std::string from_temporary = iguana::json::parse(R"({"name":"Ada"})")["name"];
  int scalar = iguana::json::parse("42");
  CHECK(from_temporary == "Ada");
  CHECK(scalar == 42);
  CHECK_THROWS_AS((void)static_cast<int>(read["name"]), std::invalid_argument);
  CHECK_THROWS_AS((void)static_cast<int>(read["large"]), std::invalid_argument);

  static_assert(!std::is_convertible_v<iguana::json&&, std::string_view>);
  static_assert(!std::is_convertible_v<iguana::json_view, const char*>);
}

TEST_CASE("DOM reads, updates and copies independently") {
  auto doc = iguana::json::parse(std::string(
      R"({"user":{"name":"Ada","roles":["reader"]},"enabled":true})"));
  const auto& read = doc;
  CHECK(read["user"]["name"].get<std::string_view>() == "Ada");
  CHECK(read.at("user").value("locale", "en") == "en");
  CHECK(read["enabled"].get<bool>());
  CHECK_FALSE(read.contains("missing"));
  CHECK_FALSE(read.find("missing"));
  CHECK(read.try_get<std::string>("user", "roles", 0).value_or("") == "reader");
  CHECK_FALSE(read.try_get<std::string>("user", "roles", 1));
  CHECK_FALSE(read.try_get<int>("user", "roles", 0));
  CHECK_FALSE(read["enabled"].try_get<int>());
  CHECK_THROWS_AS(read["missing"], std::out_of_range);
  CHECK_THROWS_AS(read["user"]["roles"][4], std::out_of_range);

  auto name = doc["user"]["name"];
  doc["user"]["roles"].push_back("writer");
  doc["profile"]["display"] = name;
  for (auto [key, value] : doc["user"].items()) {
    if (key == "name") {
      value = "Grace";
    }
  }
  CHECK(name.get<std::string>() == "Grace");
  CHECK(doc["profile"]["display"].get<std::string>() == "Ada");
  CHECK(doc["user"]["roles"][1].get<std::string>() == "writer");

  auto copy = doc;
  copy["user"]["name"] = "Lin";
  CHECK(doc["user"]["name"].get<std::string>() == "Grace");
  CHECK(copy["user"]["name"].get<std::string>() == "Lin");
  CHECK(iguana::json::parse(doc.dump(2)).dump() == doc.dump());

  auto owned = iguana::json::parse(R"({"p":{"q":42}})")["p"];
  CHECK(owned.at("q").get<int>() == 42);
}

TEST_CASE("DOM construction and arena mutation") {
  iguana::json doc = {{"name", "iguana"}, {"scores", {2, 4, 6}}};
  auto scores = doc.at("scores");
  auto middle = scores[1];
  scores.erase(0);
  CHECK(middle.get<int>() == 4);
  CHECK(scores[0].get<int>() == 4);
  scores.push_back(iguana::json::object({{"x", false}}));
  scores.push_back(scores);
  CHECK(scores[3].size() == 3);
  CHECK(scores[3][2]["x"].get<bool>() == false);
  CHECK(doc.erase("name"));
  CHECK_FALSE(doc.erase("name"));
  auto serialized = doc.dump();
  doc.compact();
  CHECK(doc.dump() == serialized);

  auto dense = iguana::json::parse(R"([0,1,2,[],{},3])");
  dense.erase(2);
  dense.push_back(nullptr);
  CHECK(dense.size() == 6);
  CHECK(dense[2].is_array());
  CHECK(dense[3].is_object());
  CHECK(dense[5].is_null());

  std::vector<int> collected;
  for (auto value : iguana::json::parse("[1,2,3]")) {
    collected.push_back(value.get<int>());
  }
  CHECK(collected == std::vector<int>{1, 2, 3});

  auto mixed = iguana::json::parse(R"([1,2,[3,4],{"k":[5]},6])");
  auto mixed_copy = mixed;
  CHECK(mixed_copy.dump() == mixed.dump());

  auto object = iguana::json::parse(
      R"({"z":1,"f":2,"e":3,"d":4,"c":5,"b":6,"h":7,"g":8,"a":9,"i":10})");
  auto stable = object["z"];
  object["aa"] = 11;
  CHECK(object.erase("e"));
  CHECK(object["aa"].get<int>() == 11);
  CHECK(stable.get<int>() == 1);
  CHECK_FALSE(object.contains("e"));

  std::string fields = "{";
  for (int i = 63; i >= 0; --i) {
    fields += "\"k" + std::to_string(i) + "\":" + std::to_string(i) + ',';
  }
  fields.back() = '}';
  auto large = iguana::json::parse(fields);
  CHECK(large["k41"].get<int>() == 41);
  for (int i = 0; i < 18; ++i) {
    CHECK(large.erase("k" + std::to_string(i)));
  }
  for (int i = 17; i >= 0; --i) {
    large["k" + std::to_string(i)] = i;
  }
  CHECK(large.size() == 64);
  CHECK(large["k0"].get<int>() == 0);
  CHECK_FALSE(iguana::json::try_parse(fields.substr(0, fields.size() - 1) +
                                      R"(,"k41":0})"));
}

TEST_CASE("DOM preserves integers and validates syntax") {
  auto numbers = iguana::parse(
      "[-9223372036854775808,9223372036854775807,18446744073709551615,1.0,1e-"
      "4]");
  CHECK(numbers[0].get<int64_t>() == std::numeric_limits<int64_t>::min());
  CHECK(numbers[1].get<int64_t>() == std::numeric_limits<int64_t>::max());
  CHECK(numbers[2].get<uint64_t>() == std::numeric_limits<uint64_t>::max());
  CHECK_FALSE(numbers[2].try_get<int64_t>());
  CHECK_FALSE(numbers[0].try_get<uint64_t>());
  CHECK_FALSE(numbers[1].try_get<int>());
  CHECK_FALSE(numbers[3].try_get<int>());
  CHECK(numbers[3].get<double>() == 1.0);
  CHECK(numbers[4].get<double>() == 0.0001);
  CHECK(iguana::json::parse(numbers.dump())[0].get<int64_t>() ==
        std::numeric_limits<int64_t>::min());

  for (std::string_view invalid : {"",
                                   "[",
                                   "{",
                                   "[1,]",
                                   R"({"x":})",
                                   R"({"x":1,})",
                                   "1 true",
                                   "01",
                                   "1.",
                                   "1e",
                                   "1e9999",
                                   "nullx",
                                   "tru",
                                   "+1",
                                   "[-]",
                                   R"("\z")",
                                   R"("\uD800")",
                                   R"("\uDC00")",
                                   R"("\uD800\u0000")",
                                   "\"\xc0\x80\"",
                                   "\"\xed\xa0\x80\"",
                                   "\"\xf4\x90\x80\x80\"",
                                   "\"a\x01\""}) {
    CAPTURE(invalid);
    CHECK_FALSE(iguana::json::try_parse(invalid));
  }

  auto duplicates = R"({"a":1,"\u0061":2})";
  std::error_code ec;
  iguana::parse(numbers, duplicates, ec);
  CHECK(ec == iguana::make_error_code(iguana::json_errc::duplicate_key));
  CHECK(numbers.size() == 5);
  iguana::parse(numbers, "[ ]", ec);
  CHECK_FALSE(ec);
  CHECK(numbers.empty());
  iguana::parse(numbers, std::string("[4,5]"));
  CHECK(numbers[1].get<int>() == 5);

  try {
    iguana::json::parse("[1] x");
    FAIL("expected json_parse_error");
  } catch (const iguana::json_parse_error& error) {
    CHECK(error.offset() == 4);
    CHECK(error.code() ==
          iguana::make_error_code(iguana::json_errc::trailing_input));
  }

  auto deep = std::string(512, '[') + "0" + std::string(512, ']');
  CHECK(iguana::json::try_parse(deep).has_value());
  CHECK_FALSE(iguana::json::try_parse("[" + deep + "]"));
}

TEST_CASE("DOM decodes Unicode and round trips escapes") {
  auto doc = iguana::json::parse(
      R"({"\u4e2d":"\uD83D\uDE00", "text":"a\u0000b\n\t\\\"","slash":"\/"})");
  CHECK(doc["\xe4\xb8\xad"].get<std::string_view>() == "\xf0\x9f\x98\x80");
  CHECK(doc["slash"].get<std::string>() == "/");
  CHECK(doc["text"].get<std::string>() == std::string("a\0b\n\t\\\"", 7));
  CHECK(iguana::json::parse(doc.dump()).dump() == doc.dump());

  for (size_t size = 0; size < 40; ++size) {
    auto text = "\"" + std::string(size, 'a') + "\xf0\x9f\x98\x80\\n\"";
    auto value = iguana::json::parse(text);
    CHECK(value.get<std::string>().size() == size + 5);
    CHECK(iguana::json::parse(value.dump()).get<std::string>() ==
          value.get<std::string>());
  }
}

TEST_CASE("DOM validates UTF-8 at block boundaries and decodes escape runs") {
  const std::string three_byte = "\xe4\xb8\xad";
  const std::string mixed =
      "\xc2\x80\xe4\xb8\xad\xf1\x80\x80\x80"
      "\xe0\xa0\x80\xf0\x90\x80\x80\xf4\x8f\xbf\xbf";
  for (size_t i = 0; i < 16; ++i) {
    const auto prefix = three_byte + std::string(i, 'a');
    const auto tail = std::string(32, 'b');
    const auto input = "\"" + prefix + mixed + tail + "\"";
    CHECK(iguana::json::parse(input).get<std::string>() ==
          prefix + mixed + tail);

    for (std::string_view bad :
         {"\x80", "\xc1\xbf", "\xe0\x9f\xbf", "\xed\xa0\x80",
          "\xf0\x8f\xbf\xbf", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80",
          "\xe4\xb8"}) {
      auto invalid = "\"" + prefix + std::string(bad) + tail + "\"";
      try {
        iguana::json::parse(invalid);
        FAIL("expected invalid UTF-8");
      } catch (const iguana::json_parse_error& error) {
        CHECK(error.code() ==
              iguana::make_error_code(iguana::json_errc::invalid_utf8));
        CHECK(error.offset() == prefix.size() + 1);
      }
    }
  }

  std::string encoded = R"("prefix)";
  std::string decoded = "prefix";
  for (unsigned i = 0; i < 96; ++i) {
    encoded += R"(\u4E2d\uD83D\uDE00\n\u0000)";
    decoded += three_byte + "\xf0\x9f\x98\x80\n";
    decoded.push_back('\0');
  }
  encoded += R"(tail")";
  decoded += "tail";
  CHECK(iguana::json::parse(encoded).get<std::string>() == decoded);
  CHECK_FALSE(iguana::json::try_parse(encoded.substr(0, encoded.size() - 16)));
}

TEST_CASE("DOM verifies cached keys against each object") {
  auto doc = iguana::json::parse(
      R"([{"id":1,"name":"one"},{"id":2,"name":"two"},)"
      R"({"id":3,"name_more":"three"},{"id":4,"\u006eame":"four"},)"
      R"({"id":5,"name":"five"}])");
  CHECK(doc[1]["name"] == "two");
  CHECK_FALSE(doc[2].contains("name"));
  CHECK(doc[2]["name_more"] == "three");
  CHECK(doc[3]["name"] == "four");
  CHECK(doc[4]["name"] == "five");
}
