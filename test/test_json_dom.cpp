#define DOCTEST_CONFIG_IMPLEMENT

#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "doctest.h"
#include "iguana/json_dom.hpp"

#ifndef IGUANA_TEST_DATA_DIR
#define IGUANA_TEST_DATA_DIR "../data"
#endif

namespace {

std::string read_text(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

std::string utf8_musical_symbol_g_clef() {
  std::string out;
  out.push_back(static_cast<char>(0xF0));
  out.push_back(static_cast<char>(0x9D));
  out.push_back(static_cast<char>(0x84));
  out.push_back(static_cast<char>(0x9E));
  return out;
}

}  // namespace

TEST_CASE("json dom parses scalar values") {
  using namespace iguana::json;

  auto doc = parse(
      R"({"nil":null,"ok":true,"no":false,"i":-42,"u":18446744073709551615,"f":6.02e23})");
  auto root = doc.root();

  CHECK(root.is_object());
  CHECK(root["nil"].is_null());
  CHECK(root["ok"].as<bool>());
  CHECK_FALSE(root["no"].as_bool());
  CHECK(root["i"].as<int>() == -42);
  CHECK(root["u"].as<uint64_t>() == (std::numeric_limits<uint64_t>::max)());
  CHECK(root["f"].as<double>() == doctest::Approx(6.02e23));
  CHECK(root.value_or<int>("missing", 7) == 7);
  CHECK_FALSE(root.find("missing"));
}

TEST_CASE("json dom can parse numbers lazily") {
  using namespace iguana::json;

  auto doc = parse(R"({"i":-42,"u":18446744073709551615,"f":6.02e23})",
                   {.numbers = number_mode::lazy});
  auto root = doc.root();

  CHECK(root["i"].type() == kind::raw_number);
  CHECK(root["i"].number_token() == "-42");
  CHECK(root["i"].as_int64() == -42);
  CHECK(root["u"].as_uint64() == (std::numeric_limits<uint64_t>::max)());
  CHECK(root["f"].as_double() == doctest::Approx(6.02e23));
  CHECK(root["f"].is_number());

  parse_options trusted;
  trusted.numbers = number_mode::lazy;
  trusted.validate_numbers = false;
  auto trusted_doc = parse_view(R"([0,-12,3.5,4e6])", trusted);
  CHECK(trusted_doc[0].number_token() == "0");
  CHECK(trusted_doc[1].as_int64() == -12);
  CHECK(trusted_doc[2].as_double() == doctest::Approx(3.5));
  CHECK(trusted_doc[3].as_double() == doctest::Approx(4e6));
}

TEST_CASE("json dom decodes strings and unicode escapes") {
  using namespace iguana::json;

  auto doc = parse(
      R"({"s":"line\n\t\\\"\/\b\f\r","unicode":"\u0041","pair":"\uD834\uDD1E"})");
  auto root = doc.root();

  CHECK(root["s"].as_string_view() == std::string_view("line\n\t\\\"/\b\f\r"));
  CHECK(root["unicode"].as_string_view() == "A");
  CHECK(root["pair"].as_string_view() == utf8_musical_symbol_g_clef());

  std::string odd_boundary = "\"";
  odd_boundary.append(30, 'a');
  odd_boundary += "\\\"b\"";
  auto odd_doc = parse(odd_boundary);
  CHECK(odd_doc.root().as_string_view() == std::string(30, 'a') + "\"b");

  std::string even_boundary = "\"";
  even_boundary.append(29, 'a');
  even_boundary += "\\\\\"";
  auto even_doc = parse(even_boundary);
  CHECK(even_doc.root().as_string_view() == std::string(29, 'a') + "\\");
}

TEST_CASE("json dom provides nlohmann-like object and array access") {
  using namespace iguana::json;

  auto doc = parse(
      R"({"items":[{"id":1,"name":"one"},{"id":2,"name":"two"},{"id":3,"name":"three"}],"meta":{"count":3}})");
  auto root = doc.root();
  auto items = root["items"].as_array();

  CHECK(items.size() == 3);
  CHECK(items[1]["name"].as<std::string>() == "two");
  CHECK(root["meta"]["count"].as<int>() == 3);
  CHECK(root.contains("items"));
  CHECK_FALSE(root.contains("absent"));
  CHECK(root["items"][0]["id"].as_uint64() == 1);
  CHECK(root["items"][2]["id"].as_uint64() == 3);

  uint64_t id_sum = 0;
  for (auto item : items) {
    id_sum += item["id"].as_uint64();
  }
  CHECK(id_sum == 6);

  size_t member_count = 0;
  for (auto item : root.as_object()) {
    CHECK_FALSE(item.key.empty());
    CHECK(item.val.valid());
    ++member_count;
  }
  CHECK(member_count == 2);
}

TEST_CASE("json dom owned and view parse keep stable string references") {
  using namespace iguana::json;

  std::string input = R"({"name":"iguana","nested":["a","b"]})";

  auto owned = parse(input);
  auto borrowed = parse_view(input);

  CHECK(owned.owns_input());
  CHECK_FALSE(borrowed.owns_input());
  CHECK(owned["name"].as_string_view() == "iguana");
  CHECK(borrowed["name"].as_string_view() == "iguana");
  CHECK(borrowed["name"].as_string_view().data() > input.data());
  CHECK(borrowed["name"].as_string_view().data() < input.data() + input.size());

  auto moved = std::move(owned);
  CHECK(moved.owns_input());
  CHECK(moved["nested"][1].as_string_view() == "b");

  auto copied = moved;
  CHECK(copied.owns_input());
  CHECK(copied["name"].as_string_view() == "iguana");

  document reusable;
  parse_view(reusable, input);
  CHECK_FALSE(reusable.owns_input());
  CHECK(reusable["nested"].size() == 2);
  parse_view(reusable, R"({"next":42})");
  CHECK(reusable.contains("next"));
  CHECK_FALSE(reusable.contains("name"));
  CHECK(reusable["next"].as_uint64() == 42);

  parse(reusable, R"(["owned"])");
  CHECK(reusable.owns_input());
  CHECK(reusable[0].as_string_view() == "owned");
}

TEST_CASE("json dom reports invalid json") {
  using namespace iguana::json;

  const std::vector<std::string> invalid_inputs = {
      R"([1,])", R"({"a":1,})", R"({"a":1 "b":2})",
      R"("\z")", R"("\uD800")", R"(01)",
      R"(-01)",  R"(1.)",       R"(1e)",
      R"(1e+)",  R"(-)",        R"(truex)",
      R"("a"x)", R"([1 2])",    R"([] true)",
  };

  for (const auto& input : invalid_inputs) {
    CAPTURE(input);
    CHECK_THROWS_AS(parse(input), parse_error);
  }

  std::string control = "\"";
  control.push_back('\x01');
  control.push_back('"');
  CHECK_THROWS_AS(parse(control), parse_error);

  std::string bad_utf8 = "\"";
  bad_utf8.push_back(static_cast<char>(0xC0));
  bad_utf8.push_back(static_cast<char>(0x80));
  bad_utf8.push_back('"');
  CHECK_THROWS_AS(parse(bad_utf8), parse_error);
}

TEST_CASE("json dom stage2 handles deep nesting iteratively") {
  using namespace iguana::json;

  constexpr size_t depth = 4096;
  std::string input(depth, '[');
  input += '0';
  input.append(depth, ']');

  auto doc = parse_view(input);
  auto current = doc.root();
  for (size_t i = 0; i < depth; ++i) {
    REQUIRE(current.is_array());
    current = current[0];
  }
  CHECK(current.as_uint64() == 0);
}

TEST_CASE("json dom validates utf-8 across simd block boundaries") {
  using namespace iguana::json;

  for (const size_t ascii_prefix : {30u, 62u}) {
    std::string input = "\"";
    input.append(ascii_prefix, 'a');
    input += utf8_musical_symbol_g_clef();
    input.append(160, 'b');
    input += '"';
    auto doc = parse_view(input);
    CHECK(doc.root().as_string_view().size() == ascii_prefix + 4 + 160);
  }

  std::string separated = "\"";
  separated.append(59, 'a');
  separated += utf8_musical_symbol_g_clef();
  separated.append(64, 'b');
  separated += utf8_musical_symbol_g_clef();
  separated.append(64, 'c');
  separated += '"';
#if defined(__AVX2__)
  CHECK(detail::simd_utf8_checker::validate(separated));
#endif
  CHECK(parse_view(separated).root().as_string_view().size() ==
        separated.size() - 2);

  std::string dense = "\"";
  for (size_t i = 0; i < 1024; ++i) {
    dense += utf8_musical_symbol_g_clef();
  }
  dense += '"';
#if defined(__AVX2__)
  CHECK(detail::simd_utf8_checker::has_dense_non_ascii_sample(dense));
  CHECK(detail::simd_utf8_checker::validate(dense));
#endif
  CHECK(parse_view(dense).root().as_string_view().size() == 4096);

  auto invalid_dense = dense;
  invalid_dense[2049] = static_cast<char>(0xC0);
#if defined(__AVX2__)
  CHECK_FALSE(detail::simd_utf8_checker::validate(invalid_dense));
#endif
  CHECK_THROWS_AS(parse_view(invalid_dense), parse_error);

  auto make_invalid = [](std::initializer_list<unsigned char> bytes) {
    std::string input = "\"";
    input.append(8, 'a');
    for (const auto byte : bytes) {
      input.push_back(static_cast<char>(byte));
    }
    input.append(180, 'b');
    input += '"';
    return input;
  };

  const auto overlong = make_invalid({0xC0, 0x80});
#if defined(__AVX2__)
  CHECK_FALSE(detail::simd_utf8_checker::validate(overlong));
#endif
  CHECK_THROWS_AS(parse_view(overlong), parse_error);
  CHECK_THROWS_AS(parse_view(make_invalid({0xED, 0xA0, 0x80})), parse_error);
  CHECK_THROWS_AS(parse_view(make_invalid({0xF4, 0x90, 0x80, 0x80})),
                  parse_error);
  CHECK_THROWS_AS(parse_view(make_invalid({0x80})), parse_error);
}

TEST_CASE("json dom preserves duplicate keys by default and can reject them") {
  using namespace iguana::json;

  auto doc = parse(R"({"a":1,"a":2})");
  auto root = doc.root();

  CHECK(root.as_object().size() == 2);
  CHECK(root["a"].as_uint64() == 2);
  CHECK_THROWS_AS(parse(R"({"a":1,"a":2})", {.reject_duplicate_keys = true}),
                  parse_error);

  auto trusted_doc = parse_trusted_view(R"({"a":1,"a":2,"b":3})");
  auto trusted_root = trusted_doc.root();
  CHECK(trusted_root.as_object().size() == 3);
  CHECK(trusted_root["a"].as_uint64() == 2);

  std::vector<std::string_view> keys;
  for (auto item : trusted_root.as_object()) {
    keys.push_back(item.key);
  }
  CHECK(keys == std::vector<std::string_view>{"a", "a", "b"});
}

TEST_CASE("json dom parses benchmark data sets") {
  using namespace iguana::json;

  const std::vector<std::string> files = {
      "apache_builds.json",  "canada.json",        "citm_catalog.json",
      "github_events.json",  "gsoc-2018.json",     "instruments.json",
      "marine_ik.json",      "mesh.json",          "mesh.pretty.json",
      "numbers.json",        "random.json",        "twitter.json",
      "twitterescaped.json", "update-center.json",
  };

  const auto data_dir = std::filesystem::path(IGUANA_TEST_DATA_DIR);
  for (const auto& file : files) {
    CAPTURE(file);
    auto input = read_text(data_dir / file);
    auto doc = parse(input);
    auto view_doc = parse_view(input);
    auto trusted_doc = parse_trusted_view(input);
    CHECK(doc.node_count() > 0);
    CHECK((doc.root().is_array() || doc.root().is_object()));
    CHECK(view_doc.node_count() > 0);
    CHECK((view_doc.root().is_array() || view_doc.root().is_object()));
    CHECK(trusted_doc.node_count() > 0);
    CHECK((trusted_doc.root().is_array() || trusted_doc.root().is_object()));
  }
}

int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
