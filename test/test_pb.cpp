#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>

#include "iguana/dynamic.hpp"
#include "iguana/pb_writer.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/iguana.hpp"
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
void print_hex_str(const std::string &str) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : str) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  std::cout << oss.str() << std::endl;
}

static void append_varint(std::string &out, uint64_t value) {
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<char>(value));
}

static void append_tag(std::string &out, uint32_t field_no,
                       iguana::WireType wire_type) {
  append_varint(out,
                (static_cast<uint64_t>(field_no) << 3) |
                    static_cast<uint32_t>(wire_type));
}

static std::chrono::system_clock::time_point make_system_time_point(
    std::chrono::seconds seconds, std::chrono::nanoseconds nanos) {
  using clock_duration = std::chrono::system_clock::duration;
  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<clock_duration>(seconds + nanos)};
}

static std::string make_nested_unknown_group(size_t depth) {
  std::string group;
  for (size_t i = 0; i < depth; ++i) {
    append_tag(group, 4, iguana::WireType::StartGroup);
  }
  append_tag(group, 5, iguana::WireType::Varint);
  append_varint(group, 99);
  for (size_t i = 0; i < depth; ++i) {
    append_tag(group, 4, iguana::WireType::EndGroup);
  }
  return group;
}

[[maybe_unused]] static void append_fixed32(std::string &out, uint32_t value) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<char>((value >> (8 * i)) & 0xff));
  }
}

#define PUBLIC(T) : public iguana::base_impl<T>

struct point_t {
  point_t() = default;
  point_t(int a, double b) : x(a), y(b) {}
  int x;
  double y;
};
YLT_REFL(point_t, x, y);

namespace my_space {
struct inner_struct {
  inner_struct() = default;
  inner_struct(int a, int b, int c) : x(a), y(b), z(c) {}
  int x;
  int y;
  int z;
};

inline auto get_members_impl(inner_struct *) {
  return std::make_tuple(iguana::build_pb_field<&inner_struct::x, 7>("a"),
                         iguana::build_pb_field<&inner_struct::y, 9>("b"),
                         iguana::build_pb_field<&inner_struct::z, 12>("c"));
}
}  // namespace my_space

struct test_pb_st1 {
  test_pb_st1() = default;
  test_pb_st1(int a, iguana::sint32_t b, iguana::sint64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
};
YLT_REFL(test_pb_st1, x, y, z);

struct test_pb_sts PUBLIC(test_pb_sts) {
  test_pb_sts() = default;
  test_pb_sts(std::vector<test_pb_st1> l) : list(std::move(l)) {}
  std::vector<test_pb_st1> list;
};
YLT_REFL(test_pb_sts, list);

struct test_pb_st2 {
  test_pb_st2() = default;
  test_pb_st2(int a, iguana::fixed32_t b, iguana::fixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::fixed32_t y;
  iguana::fixed64_t z;
};
YLT_REFL(test_pb_st2, x, y, z);

struct test_pb_st3 {
  test_pb_st3() = default;
  test_pb_st3(int a, iguana::sfixed32_t b, iguana::sfixed64_t c)
      : x(a), y(b), z(c) {}
  int x;
  iguana::sfixed32_t y;
  iguana::sfixed64_t z;
};
YLT_REFL(test_pb_st3, x, y, z);

struct test_pb_legacy_oneof {
  int32_t id{};
  std::variant<std::monostate, int32_t, std::string> result;
  int32_t tail{};
};

inline auto get_members_impl(test_pb_legacy_oneof *) {
  return iguana::pb_members(
      iguana::pb_field<&test_pb_legacy_oneof::id, 1>("id"),
      iguana::pb_oneof_field<&test_pb_legacy_oneof::result, 5, 8>("result"),
      iguana::pb_field<&test_pb_legacy_oneof::tail, 9>("tail"));
}

struct test_pb_legacy_schema {
  std::string payload;
  int32_t delta{};
  std::vector<int64_t> deltas;
  uint32_t f32{};
  int64_t sf64{};
  std::chrono::system_clock::time_point created_at;
  std::chrono::nanoseconds timeout{};
  std::vector<std::chrono::nanoseconds> spans;
};

inline auto get_members_impl(test_pb_legacy_schema *) {
  return iguana::pb_members(
      iguana::pb_bytes_field<&test_pb_legacy_schema::payload, 3>("payload"),
      iguana::pb_zigzag_field<&test_pb_legacy_schema::delta, 5>("delta"),
      iguana::pb_zigzag_field<&test_pb_legacy_schema::deltas, 6>("deltas"),
      iguana::pb_fixed_field<&test_pb_legacy_schema::f32, 7>("f32"),
      iguana::pb_fixed_field<&test_pb_legacy_schema::sf64, 8>("sf64"),
      iguana::pb_timestamp_field<&test_pb_legacy_schema::created_at, 9>(
          "created_at"),
      iguana::pb_duration_field<&test_pb_legacy_schema::timeout, 10>(
          "timeout"),
      iguana::pb_duration_field<&test_pb_legacy_schema::spans, 11>("spans"));
}

struct test_pb_legacy_schema_expected {
  std::string payload;
  iguana::sint32_t delta;
  std::vector<iguana::sint64_t> deltas;
  iguana::fixed32_t f32;
  iguana::sfixed64_t sf64;
  iguana::pb_timestamp created_at;
  iguana::pb_duration timeout;
  std::vector<iguana::pb_duration> spans;
};
YLT_REFL_PB(test_pb_legacy_schema_expected, (payload, 3), (delta, 5),
            (deltas, 6), (f32, 7), (sf64, 8), (created_at, 9), (timeout, 10),
            (spans, 11));

struct test_pb_legacy_optional {
  std::optional<int32_t> count;
  std::optional<std::string> label;
  std::optional<int32_t> legacy;
};

inline auto get_members_impl(test_pb_legacy_optional *) {
  return iguana::pb_members(
      iguana::pb_optional_field<&test_pb_legacy_optional::count, 1>("count"),
      iguana::pb_optional_field<&test_pb_legacy_optional::label, 2>("label"),
      iguana::pb_field<&test_pb_legacy_optional::legacy, 3>("legacy"));
}

struct test_pb_legacy_combined_schema {
  std::optional<int32_t> maybe_delta;
  std::vector<uint32_t> packed_fixed;
};

inline auto get_members_impl(test_pb_legacy_combined_schema *) {
  return iguana::pb_members(
      iguana::pb_field_ex<&test_pb_legacy_combined_schema::maybe_delta, 6>(
          "maybe_delta", iguana::pb_optional, iguana::pb_zigzag),
      iguana::pb_fixed_field<&test_pb_legacy_combined_schema::packed_fixed, 7>(
          "packed_fixed"));
}

struct test_pb_legacy_combined_schema_expected {
  std::optional<iguana::sint32_t> maybe_delta;
  std::vector<iguana::fixed32_t> packed_fixed;
};
YLT_REFL_PB(test_pb_legacy_combined_schema_expected, (maybe_delta, 6),
            (packed_fixed, 7));

#ifdef YLT_USE_CXX26_REFLECTION
struct test_pb_annotation_wire {
  int x;
  [[= iguana::pb_zigzag]] int32_t y;
  [[= iguana::pb_zigzag]] int64_t z;
  [[= iguana::pb_fixed]] uint32_t f32;
  [[= iguana::pb_fixed]] int64_t sf64;
};

struct test_pb_annotation_wire_expected {
  int x;
  iguana::sint32_t y;
  iguana::sint64_t z;
  iguana::fixed32_t f32;
  iguana::sfixed64_t sf64;
};
YLT_REFL(test_pb_annotation_wire_expected, x, y, z, f32, sf64);

struct test_pb_annotation_wire_container {
  [[= iguana::pb_field(6)]]
  [[= iguana::pb_zigzag]] std::optional<int32_t> maybe_delta;
  [[= iguana::pb_field(7)]]
  [[= iguana::pb_fixed]] std::vector<uint32_t> packed_fixed;
  [[= iguana::pb_field(8)]]
  [[= iguana::pb_zigzag]] std::vector<int64_t> deltas;
};

struct test_pb_annotation_wire_container_expected {
  std::optional<iguana::sint32_t> maybe_delta;
  std::vector<iguana::fixed32_t> packed_fixed;
  std::vector<iguana::sint64_t> deltas;
};
YLT_REFL_PB(test_pb_annotation_wire_container_expected, (maybe_delta, 6),
            (packed_fixed, 7), (deltas, 8));

struct test_pb_annotation_bytes {
  [[= iguana::pb_field(3)]]
  [[= iguana::pb_bytes]] std::string payload;
  [[= iguana::pb_field(4)]] std::string plain;
};

struct test_pb_annotation_bytes_expected {
  std::string payload;
  std::string plain;
};
YLT_REFL_PB(test_pb_annotation_bytes_expected, (payload, 3), (plain, 4));

struct test_pb_annotation_oneof {
  [[= iguana::pb_field(1)]] int32_t id;
  [[= iguana::pb_oneof<5, 8>]]
  std::variant<std::monostate, int32_t, std::string> result;
  [[= iguana::pb_field(9)]] int32_t tail;
};

struct test_pb_annotation_chrono {
  [[= iguana::pb_field(2)]]
  [[= iguana::as_timestamp]]
  std::chrono::system_clock::time_point created_at;
  [[= iguana::pb_field(4)]]
  [[= iguana::as_duration]] std::chrono::nanoseconds timeout;
  [[= iguana::pb_field(6)]]
  [[= iguana::as_timestamp]]
  std::optional<std::chrono::system_clock::time_point> maybe_at;
  [[= iguana::pb_field(7)]]
  [[= iguana::as_duration]] std::vector<std::chrono::nanoseconds> spans;
};

struct test_pb_annotation_chrono_expected {
  iguana::pb_timestamp created_at;
  iguana::pb_duration timeout;
  std::optional<iguana::pb_timestamp> maybe_at;
  std::vector<iguana::pb_duration> spans;
};
YLT_REFL_PB(test_pb_annotation_chrono_expected, (created_at, 2), (timeout, 4),
            (maybe_at, 6), (spans, 7));

struct test_pb_annotation_optional {
  [[= iguana::pb_field(1)]]
  [[= iguana::pb_optional]]
  std::optional<int32_t> count;
  [[= iguana::pb_field(2)]]
  [[= iguana::pb_optional]]
  std::optional<std::string> label;
  [[= iguana::pb_field(3)]] std::optional<int32_t> legacy;
};
#endif

struct test_pb_st4 {
  test_pb_st4() = default;
  test_pb_st4(int a, std::string b) : x(a), y(std::move(b)) {}
  int x;
  std::string y;
};
YLT_REFL(test_pb_st4, x, y);

struct test_pb_st5 {
  test_pb_st5() = default;
  test_pb_st5(int a, std::string_view b) : x(a), y(b) {}
  int x;
  std::string_view y;
};
YLT_REFL(test_pb_st5, x, y);

struct test_pb_st6 {
  test_pb_st6() = default;
  test_pb_st6(std::optional<int> a, std::optional<std::string> b)
      : x(std::move(a)), y(std::move(b)) {}
  std::optional<int> x;
  std::optional<std::string> y;
};
YLT_REFL(test_pb_st6, x, y);

struct pair_t
    : public iguana::base_impl<pair_t, iguana::ENABLE_XML | iguana::ENABLE_PB |
                                           iguana::ENABLE_YAML |
                                           iguana::ENABLE_JSON> {
  pair_t() = default;
  pair_t(int a, int b) : x(a), y(b) {}
  int x;
  int y;
};
YLT_REFL(pair_t, x, y);

struct message_t PUBLIC(message_t) {
  message_t() = default;
  message_t(int a, pair_t b) : id(a), t(b) {}
  int id;
  pair_t t;
};
YLT_REFL(message_t, id, t);

struct test_pb_st8 {
  test_pb_st8() = default;
  test_pb_st8(int a, pair_t b, message_t c) : x(a), y(b), z(c) {}

  int x;
  pair_t y;
  message_t z;
};
YLT_REFL(test_pb_st8, x, y, z);

struct test_pb_st9 {
  test_pb_st9() = default;
  test_pb_st9(int a, std::vector<int> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<int> y;
  std::string z;
};
YLT_REFL(test_pb_st9, x, y, z);

struct test_pb_st10 {
  test_pb_st10() = default;
  test_pb_st10(int a, std::vector<message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<message_t> y;
  std::string z;
};
YLT_REFL(test_pb_st10, x, y, z);

struct test_pb_st11 PUBLIC(test_pb_st11) {
  test_pb_st11() = default;
  test_pb_st11(int a, std::vector<std::optional<message_t>> b,
               std::vector<std::string> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
YLT_REFL(test_pb_st11, x, y, z);

struct test_pb_st12 {
  test_pb_st12() = default;
  test_pb_st12(int a, std::map<int, std::string> b,
               std::map<std::string, int> c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
YLT_REFL(test_pb_st12, x, y, z);

struct test_pb_st13 {
  test_pb_st13() = default;
  test_pb_st13(int a, std::map<int, message_t> b, std::string c)
      : x(a), y(std::move(b)), z(std::move(c)) {}

  int x;
  std::map<int, message_t> y;
  std::string z;
};
YLT_REFL(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 {
  test_pb_st14() = default;
  test_pb_st14(int a, colors_t b, level_t c) : x(a), y(b), z(c) {}
  int x;
  colors_t y;
  level_t z;
};
YLT_REFL(test_pb_st14, x, y, z);

struct test_pb_merge_inner {
  int32_t a{};
  int32_t b{};
};
YLT_REFL(test_pb_merge_inner, a, b);

struct test_pb_merge_outer {
  test_pb_merge_inner msg;
  std::optional<test_pb_merge_inner> maybe;
  std::variant<int32_t, test_pb_merge_inner, std::string> choice;
};
YLT_REFL(test_pb_merge_outer, msg, maybe, choice);

namespace client {
struct person {
  person() = default;
  person(std::string s, int d) : name(s), age(d) {}
  std::string name;
  int64_t age;
};

YLT_REFL(person, name, age);
}  // namespace client

struct my_struct PUBLIC(my_struct) {
  my_struct() = default;
  my_struct(int a, bool b, iguana::fixed64_t c) : x(a), y(b), z(c) {}
  int x;
  bool y;
  iguana::fixed64_t z;
  bool operator==(const my_struct &other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};
YLT_REFL(my_struct, x, y, z);

struct nest1 PUBLIC(nest1) {
  nest1() = default;
  nest1(std::string s, my_struct t, int d)
      : name(std::move(s)), value(t), var(d) {}
  std::string name;
  my_struct value;
  int var;
  std::variant<int, double> mv;
};
YLT_REFL(nest1, name, value, var, mv);

struct numer_st PUBLIC(numer_st) {
  numer_st() = default;
  numer_st(bool x, double y, float z) : a(x), b(y), c(z) {}
  bool a;
  double b;
  float c;
};
YLT_REFL(numer_st, a, b, c);

struct MyPerson : public iguana::base_impl<MyPerson, iguana::ENABLE_JSON> {
  MyPerson() = default;
  MyPerson(std::string s, int d) : name(s), age(d) {}
  std::string name;
  int64_t age;
  bool operator==(const MyPerson &other) const {
    return name == other.name && age == other.age;
  }
};

YLT_REFL(MyPerson, name, age);
struct person PUBLIC(person) {
  person() = default;
  person(int32_t a, std::string b, int c, double d)
      : id(a), name(std::move(b)), age(c), salary(d) {}
  int32_t id;
  std::string name;
  int age;
  double salary;
};
YLT_REFL(person, id, name, age, salary);

enum Color { Red = 0, Black = 2, Green = 4 };

namespace iguana {
template <>
struct enum_value<Color> {
  constexpr static std::array<int, 3> value = {0, 2, 4};
};
}  // namespace iguana

struct vector_t {
  int id;
  Color color;
  std::variant<int, pair_t, std::string> variant;
  std::vector<int> ids;
  std::vector<pair_t> pairs;
  std::vector<std::string> strs;
  std::map<std::string, pair_t> map;
  std::string name;
  std::optional<int> op_val;
};
YLT_REFL(vector_t, id, color, variant, ids, pairs, strs, map, name, op_val);

#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
TEST_CASE("struct to proto") {
  {
    std::string str;
    iguana::to_proto<vector_t>(str, "pb");
    std::cout << str;
    CHECK(str.find(R"(syntax = "proto3";)") != std::string::npos);
    CHECK(str.find("message vector_t") != std::string::npos);
    CHECK(str.find("map<string, pair_t>  map = 9;") != std::string::npos);
    CHECK(str.find("Green = 4;") != std::string::npos);

    const auto proto_path =
        std::filesystem::temp_directory_path() / "iguana_test_vector.proto";
    std::ofstream file(proto_path, std::ios::binary);
    iguana::to_proto_file<vector_t>(file, "pb");
    file.sync_with_stdio(true);
    file.flush();
    file.close();

    size_t size = std::filesystem::file_size(proto_path);
    std::ifstream in(proto_path, std::ios::binary);
    std::string read_str;
    read_str.resize(size);
    in.read(read_str.data(), size);
    CHECK(read_str.find("map<string, pair_t>  map = 9;") != std::string::npos);
    CHECK(read_str.find("Green = 4;") != std::string::npos);
    std::filesystem::remove(proto_path);
  }
  {
    std::string str;
    iguana::to_proto<test_pb_st8>(str, "pb");
    std::cout << str;
    CHECK(str.find("message_t z = 3;") != std::string::npos);
    CHECK(str.find("message message_t") != std::string::npos);
    CHECK(str.find("pair_t t = 2;") != std::string::npos);
    CHECK(str.find("message pair_t") != std::string::npos);
  }
  {
    std::string str;
    iguana::to_proto<test_pb_st1>(str, "pb");
    std::cout << str;
    CHECK(str.find("sint64 z = 3;") != std::string::npos);

    iguana::to_proto<test_pb_st2, false>(str);
    std::cout << str;
    CHECK(str.find("fixed64 z = 3;") != std::string::npos);

    iguana::to_proto<test_pb_st3, false>(str);
    std::cout << str;
    CHECK(str.find("sfixed64 z = 3;") != std::string::npos);

    iguana::to_proto<test_pb_legacy_oneof, false>(str);
    CHECK(str.find("oneof result") != std::string::npos);
    CHECK(str.find("int32  one_of_int32 = 5;") != std::string::npos);
    CHECK(str.find("string  one_of_string = 8;") != std::string::npos);
    CHECK(str.find("monostate") == std::string::npos);

    iguana::to_proto<test_pb_legacy_schema>(str);
    CHECK(str.find(R"(import "google/protobuf/timestamp.proto";)") !=
          std::string::npos);
    CHECK(str.find(R"(import "google/protobuf/duration.proto";)") !=
          std::string::npos);
    CHECK(str.find("bytes  payload = 3;") != std::string::npos);
    CHECK(str.find("sint32 delta = 5;") != std::string::npos);
    CHECK(str.find("sint64 deltas = 6;") != std::string::npos);
    CHECK(str.find("fixed32 f32 = 7;") != std::string::npos);
    CHECK(str.find("sfixed64 sf64 = 8;") != std::string::npos);
    CHECK(str.find("google.protobuf.Timestamp created_at = 9;") !=
          std::string::npos);
    CHECK(str.find("google.protobuf.Duration timeout = 10;") !=
          std::string::npos);
    CHECK(str.find("repeated google.protobuf.Duration spans = 11;") !=
          std::string::npos);

    iguana::to_proto<test_pb_legacy_optional, false>(str);
    CHECK(str.find("optional  int32 count = 1;") != std::string::npos);
    CHECK(str.find("optional  string  label = 2;") != std::string::npos);
    CHECK(str.find("int32 legacy = 3;") != std::string::npos);

    iguana::to_proto<test_pb_legacy_combined_schema, false>(str);
    CHECK(str.find("optional  sint32 maybe_delta = 6;") !=
          std::string::npos);
    CHECK(str.find("fixed32 packed_fixed = 7;") != std::string::npos);

#ifdef YLT_USE_CXX26_REFLECTION
    iguana::to_proto<test_pb_annotation_wire, false>(str);
    CHECK(str.find("sint32 y = 2;") != std::string::npos);
    CHECK(str.find("sint64 z = 3;") != std::string::npos);
    CHECK(str.find("fixed32 f32 = 4;") != std::string::npos);
    CHECK(str.find("sfixed64 sf64 = 5;") != std::string::npos);

    iguana::to_proto<test_pb_annotation_wire_container, false>(str);
    CHECK(str.find("sint32 maybe_delta = 6;") != std::string::npos);
    CHECK(str.find("fixed32 packed_fixed = 7;") != std::string::npos);
    CHECK(str.find("sint64 deltas = 8;") != std::string::npos);

    iguana::to_proto<test_pb_annotation_bytes, false>(str);
    CHECK(str.find("bytes  payload = 3;") != std::string::npos);
    CHECK(str.find("string  plain = 4;") != std::string::npos);

    iguana::to_proto<test_pb_annotation_oneof, false>(str);
    CHECK(str.find("oneof result") != std::string::npos);
    CHECK(str.find("int32  one_of_int32 = 5;") != std::string::npos);
    CHECK(str.find("string  one_of_string = 8;") != std::string::npos);
    CHECK(str.find("monostate") == std::string::npos);

    iguana::to_proto<test_pb_annotation_chrono>(str);
    CHECK(str.find(R"(import "google/protobuf/timestamp.proto";)") !=
          std::string::npos);
    CHECK(str.find(R"(import "google/protobuf/duration.proto";)") !=
          std::string::npos);
    CHECK(str.find("google.protobuf.Timestamp created_at = 2;") !=
          std::string::npos);
    CHECK(str.find("google.protobuf.Duration timeout = 4;") !=
          std::string::npos);
    CHECK(str.find("google.protobuf.Timestamp maybe_at = 6;") !=
          std::string::npos);
    CHECK(str.find("repeated google.protobuf.Duration spans = 7;") !=
          std::string::npos);

    iguana::to_proto<test_pb_annotation_optional, false>(str);
    CHECK(str.find("optional  int32 count = 1;") != std::string::npos);
    CHECK(str.find("optional  string  label = 2;") != std::string::npos);
    CHECK(str.find("int32 legacy = 3;") != std::string::npos);
#endif

    iguana::to_proto<message_t, false>(str);
    std::cout << str;
    CHECK(str.find("pair_t t = 2;") != std::string::npos);
  }
  {
    std::string str;
    iguana::to_proto<person>(str);
    std::cout << str;
    CHECK(str.find("int32 age = 3;") != std::string::npos);
  }
}
#endif

TEST_CASE("test YLT_REFL") {
  {
    my_struct temp_my{1, false, {3}};
    nest1 t{"Hi", temp_my, 5};

    auto const temp_variant = std::variant<int, double>{1};
    t.set_field_value("mv", temp_variant);

    // dynamic any
    auto const &any_name = t.get_field_any("name");
    assert(std::any_cast<std::string>(any_name) == "Hi");

    auto const &any_value = t.get_field_any("value");
    assert(std::any_cast<my_struct>(any_value) == temp_my);

    auto const &any_var = t.get_field_any("var");
    assert(std::any_cast<int>(any_var) == 5);

    auto const &mvariant_any = t.get_field_any("mv");
    auto const &mvariant =
        std::any_cast<std::variant<int, double>>(mvariant_any);
    assert(mvariant == temp_variant);
  }
  {
    std::cout << "test\n";
    auto t = iguana::create_instance("nest1");
    std::vector<std::string_view> fields_name = t->get_fields_name();
    CHECK(fields_name ==
          std::vector<std::string_view>{"name", "value", "var", "mv"});

    my_struct mt{2, true, {42}};
    t->set_field_value("value", mt);
    t->set_field_value("name", std::string("test"));
    t->set_field_value("var", 41);
    nest1 *st = dynamic_cast<nest1 *>(t.get());
    auto p = *st;
    std::cout << p.name << "\n";
    auto &r0 = t->get_field_value<std::string>("name");
    CHECK(r0 == "test");
    auto &r = t->get_field_value<int>("var");
    CHECK(r == 41);
    auto &r1 = t->get_field_value<my_struct>("value");
    CHECK(r1.x == 2);

    t->set_field_value<std::string>("name", "hello");
    auto &s = t->get_field_value<std::string>("name");
    CHECK(s == "hello");

    CHECK_THROWS_AS(t->set_field_value<int>("name", 42), std::invalid_argument);
  }

  {
    // to_json is an const member_function now
    MyPerson const p1{"xiaoming", 10};
    std::string str;
    p1.to_json(str);

    // p1.to_pb(str); // compile failed

    MyPerson p2;
    p2.from_json(str);

    assert(p1 == p2);
  }
  {
    auto t = iguana::create_instance("pair_t");
    t->set_field_value("x", 12);
    t->set_field_value("y", 24);
    auto &r0 = t->get_field_value<int>("x");
    CHECK(r0 == 12);
    auto &r = t->get_field_value<int>("y");
    CHECK(r == 24);

    std::string str;
    t->to_pb(str);

    pair_t t1;
    t1.from_pb(str);

    pair_t *st = dynamic_cast<pair_t *>(t.get());
    CHECK(st->x == t1.x);
    CHECK(st->y == t1.y);
    std::string xml;
    st->to_xml(xml);
    std::cout << xml << "\n";
    pair_t s;
    s.from_xml(xml);
    std::cout << s.x << " " << s.y << "\n";
    CHECK(st->x == s.x);
    CHECK(st->y == s.y);

    // std::string json;
    // t->to_json(json);
    // std::cout << json << "\n";

    // s = {};
    // s.from_json(json);
    // std::cout << s.x << " " << s.y << "\n";
    // CHECK(st->x == s.x);
    // CHECK(st->y == s.y);

    std::string yaml;
    t->to_yaml(yaml);
    std::cout << yaml << "\n";

    s = {};
    s.from_yaml(yaml);
    std::cout << s.x << " " << s.y << "\n";
    CHECK(st->x == s.x);
    CHECK(st->y == s.y);
  }
  auto t = iguana::create_instance("numer_st");
  t->set_field_value<bool>("a", true);
  t->set_field_value<double>("b", 25);
  t->set_field_value<float>("c", 42);
  auto &r0 = t->get_field_value<bool>("a");
  CHECK(r0);
  auto &r = t->get_field_value<double>("b");
  CHECK(r == 25);
  auto &r1 = t->get_field_value<float>("c");
  CHECK(r1 == 42);

  numer_st *st = dynamic_cast<numer_st *>(t.get());
  CHECK(st->a == true);
  CHECK(st->b == 25);
  CHECK(st->c == 42);

  std::string str;
  t->to_pb(str);

  numer_st t1;
  t1.from_pb(str);

  CHECK(st->a == t1.a);
  CHECK(st->b == t1.b);
  CHECK(st->c == t1.c);
}

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{41, 42, 43};

    std::string str;
    iguana::to_pb(inner, str);

    my_space::inner_struct inner1;
    iguana::from_pb(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);

    std::stringstream ss;
    iguana::to_pb(inner, ss);
    std::string str1 = ss.str();
    my_space::inner_struct inner2;
    iguana::from_pb(inner2, str);
    CHECK(inner.x == inner2.x);
    CHECK(inner.y == inner2.y);
    CHECK(inner.z == inner2.z);
  }

  {
    test_pb_sts p{};
    p.list.push_back(test_pb_st1{41, {42}, {43}});
    std::string str;
    p.to_pb(str);

    test_pb_sts p1{};
    p1.from_pb(str);
    CHECK(p.list[0].z == p1.list[0].z);
  }

  {
    test_pb_st1 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st1 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.x == st2.x);
    CHECK(st1.y.val == st2.y.val);
    CHECK(st1.z.val == st2.z.val);
  }

  {
    test_pb_st2 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st2 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{41, {42}, {43}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st3 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_legacy_oneof st1{7, int32_t{42}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    std::string expected;
    append_tag(expected, 1, iguana::WireType::Varint);
    append_varint(expected, 7);
    append_tag(expected, 5, iguana::WireType::Varint);
    append_varint(expected, 42);
    append_tag(expected, 9, iguana::WireType::Varint);
    append_varint(expected, 9);
    CHECK(str == expected);

    test_pb_legacy_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::get<int32_t>(st2.result) == 42);
    CHECK(st2.tail == st1.tail);
  }
  {
    test_pb_legacy_oneof st1{7, std::string{"chosen"}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    std::string expected;
    append_tag(expected, 1, iguana::WireType::Varint);
    append_varint(expected, 7);
    append_tag(expected, 8, iguana::WireType::LengthDelimeted);
    append_varint(expected, 6);
    expected.append("chosen");
    append_tag(expected, 9, iguana::WireType::Varint);
    append_varint(expected, 9);
    CHECK(str == expected);

    test_pb_legacy_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::get<std::string>(st2.result) == "chosen");
    CHECK(st2.tail == st1.tail);
  }
  {
    test_pb_legacy_oneof st1{7, std::monostate{}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    std::string expected;
    append_tag(expected, 1, iguana::WireType::Varint);
    append_varint(expected, 7);
    append_tag(expected, 9, iguana::WireType::Varint);
    append_varint(expected, 9);
    CHECK(str == expected);

    test_pb_legacy_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::holds_alternative<std::monostate>(st2.result));
    CHECK(st2.tail == st1.tail);
  }
  {
    std::string str;
    append_tag(str, 1, iguana::WireType::Varint);
    append_varint(str, 7);
    append_tag(str, 5, iguana::WireType::Varint);
    append_varint(str, 42);
    append_tag(str, 8, iguana::WireType::LengthDelimeted);
    append_varint(str, 6);
    str.append("chosen");
    append_tag(str, 9, iguana::WireType::Varint);
    append_varint(str, 9);

    test_pb_legacy_oneof st;
    iguana::from_pb(st, str);
    CHECK(st.id == 7);
    CHECK(std::get<std::string>(st.result) == "chosen");
    CHECK(st.tail == 9);
  }
  {
    using namespace std::chrono;
    test_pb_legacy_schema st1{
        std::string{"\x01\x02\x7fpayload", 10},
        -42,
        {-1, 0, 1234567890123LL},
        0xDEADBEEFu,
        -45,
        make_system_time_point(seconds{1680000000}, nanoseconds{123}),
        nanoseconds{-1500000000},
        {nanoseconds{0}, nanoseconds{2500000000}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_legacy_schema st2;
    iguana::from_pb(st2, str);
    CHECK(st2.payload == st1.payload);
    CHECK(st2.delta == st1.delta);
    CHECK(st2.deltas == st1.deltas);
    CHECK(st2.f32 == st1.f32);
    CHECK(st2.sf64 == st1.sf64);
    CHECK(st2.created_at == st1.created_at);
    CHECK(st2.timeout == st1.timeout);
    CHECK(st2.spans == st1.spans);

    test_pb_legacy_schema_expected expected{
        st1.payload,
        iguana::sint32_t{st1.delta},
        {iguana::sint64_t{st1.deltas[0]}, iguana::sint64_t{st1.deltas[1]},
         iguana::sint64_t{st1.deltas[2]}},
        iguana::fixed32_t{st1.f32},
        iguana::sfixed64_t{st1.sf64},
        iguana::pb_timestamp{st1.created_at},
        iguana::pb_duration{st1.timeout},
        {iguana::pb_duration{st1.spans[0]},
         iguana::pb_duration{st1.spans[1]}}};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
  {
    test_pb_legacy_optional st1{int32_t{0}, std::string{}, int32_t{0}};
    std::string str;
    iguana::to_pb(st1, str);

    std::string expected;
    expected.push_back(static_cast<char>(8));
    expected.push_back(static_cast<char>(0));
    expected.push_back(static_cast<char>(18));
    expected.push_back(static_cast<char>(0));
    CHECK(str == expected);

    test_pb_legacy_optional st2;
    iguana::from_pb(st2, str);
    REQUIRE(st2.count.has_value());
    CHECK(*st2.count == 0);
    REQUIRE(st2.label.has_value());
    CHECK(st2.label->empty());
    CHECK(!st2.legacy.has_value());

    st1.legacy = 7;
    str.clear();
    iguana::to_pb(st1, str);
    iguana::from_pb(st2, str);
    REQUIRE(st2.legacy.has_value());
    CHECK(*st2.legacy == 7);
  }
  {
    test_pb_legacy_combined_schema st1{
        int32_t{-7}, {0xDEADBEEFu, 7u}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_legacy_combined_schema st2;
    iguana::from_pb(st2, str);
    CHECK(st2.maybe_delta == st1.maybe_delta);
    CHECK(st2.packed_fixed == st1.packed_fixed);

    test_pb_legacy_combined_schema_expected expected{
        iguana::sint32_t{-7},
        {iguana::fixed32_t{0xDEADBEEFu}, iguana::fixed32_t{7u}}};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
#ifdef YLT_USE_CXX26_REFLECTION
  {
    test_pb_annotation_wire st1{41, -42, -43, 44, -45};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_wire st2;
    iguana::from_pb(st2, str);
    CHECK(st1.x == st2.x);
    CHECK(st1.y == st2.y);
    CHECK(st1.z == st2.z);
    CHECK(st1.f32 == st2.f32);
    CHECK(st1.sf64 == st2.sf64);

    test_pb_annotation_wire_expected expected{
        41, {-42}, {-43}, {44}, {-45}};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
  {
    test_pb_annotation_wire_container st1{
        int32_t{-7}, {0xDEADBEEFu, 7u}, {-1, 0, 1234567890123LL}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_wire_container st2;
    iguana::from_pb(st2, str);
    CHECK(st2.maybe_delta == st1.maybe_delta);
    CHECK(st2.packed_fixed == st1.packed_fixed);
    CHECK(st2.deltas == st1.deltas);

    test_pb_annotation_wire_container_expected expected{
        iguana::sint32_t{-7},
        {iguana::fixed32_t{0xDEADBEEFu}, iguana::fixed32_t{7u}},
        {iguana::sint64_t{-1}, iguana::sint64_t{0},
         iguana::sint64_t{1234567890123LL}}};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
  {
    std::string str;
    append_tag(str, 7, iguana::WireType::LengthDelimeted);
    append_varint(str, 4);
    append_fixed32(str, 1);
    append_tag(str, 7, iguana::WireType::LengthDelimeted);
    append_varint(str, 4);
    append_fixed32(str, 2);
    append_tag(str, 8, iguana::WireType::LengthDelimeted);
    append_varint(str, 1);
    append_varint(str, 1);
    append_tag(str, 8, iguana::WireType::LengthDelimeted);
    append_varint(str, 1);
    append_varint(str, 4);

    test_pb_annotation_wire_container st;
    iguana::from_pb(st, str);
    CHECK(st.packed_fixed == std::vector<uint32_t>{1, 2});
    CHECK(st.deltas == std::vector<int64_t>{-1, 2});
  }
  {
    test_pb_annotation_bytes st1{std::string{"\x01\x02\x7fpayload", 10},
                                 "plain"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_bytes st2;
    iguana::from_pb(st2, str);
    CHECK(st1.payload == st2.payload);
    CHECK(st1.plain == st2.plain);

    test_pb_annotation_bytes_expected expected{st1.payload, st1.plain};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
  {
    test_pb_annotation_oneof st1{7, int32_t{42}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::get<int32_t>(st2.result) == 42);
    CHECK(st2.tail == st1.tail);
  }
  {
    test_pb_annotation_oneof st1{7, std::string{"chosen"}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::get<std::string>(st2.result) == "chosen");
    CHECK(st2.tail == st1.tail);
  }
  {
    test_pb_annotation_oneof st1{7, std::monostate{}, 9};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_oneof st2;
    iguana::from_pb(st2, str);
    CHECK(st2.id == st1.id);
    CHECK(std::holds_alternative<std::monostate>(st2.result));
    CHECK(st2.tail == st1.tail);
  }
  {
    using namespace std::chrono;
    test_pb_annotation_chrono st1{
        make_system_time_point(seconds{1680000000}, nanoseconds{123}),
        nanoseconds{-1500000000},
        make_system_time_point(seconds{-2}, nanoseconds{250000000}),
        {nanoseconds{0}, nanoseconds{2500000000}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_annotation_chrono st2;
    iguana::from_pb(st2, str);
    CHECK(st2.created_at == st1.created_at);
    CHECK(st2.timeout == st1.timeout);
    REQUIRE(st2.maybe_at.has_value());
    CHECK(*st2.maybe_at == *st1.maybe_at);
    CHECK(st2.spans == st1.spans);

    test_pb_annotation_chrono_expected expected{
        iguana::pb_timestamp{st1.created_at},
        iguana::pb_duration{st1.timeout},
        iguana::pb_timestamp{*st1.maybe_at},
        {iguana::pb_duration{st1.spans[0]}, iguana::pb_duration{st1.spans[1]}}};
    std::string expected_str;
    iguana::to_pb(expected, expected_str);
    CHECK(str == expected_str);
  }
  {
    test_pb_annotation_optional st1{int32_t{0}, std::string{}, int32_t{0}};
    std::string str;
    iguana::to_pb(st1, str);

    std::string expected;
    expected.push_back(static_cast<char>(8));
    expected.push_back(static_cast<char>(0));
    expected.push_back(static_cast<char>(18));
    expected.push_back(static_cast<char>(0));
    CHECK(str == expected);

    test_pb_annotation_optional st2;
    iguana::from_pb(st2, str);
    REQUIRE(st2.count.has_value());
    CHECK(*st2.count == 0);
    REQUIRE(st2.label.has_value());
    CHECK(st2.label->empty());
    CHECK(!st2.legacy.has_value());

    st1.legacy = 7;
    str.clear();
    iguana::to_pb(st1, str);
    iguana::from_pb(st2, str);
    REQUIRE(st2.legacy.has_value());
    CHECK(*st2.legacy == 7);
  }
#endif
  {
    test_pb_st4 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st4 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st5 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{41, "it is a test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st6 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // sub nested objects
    nest1 v{"Hi", {1, false, {3}}, 5}, v2{};
    std::string s;
    iguana::to_pb(v, s);
    iguana::from_pb(v2, s);
    CHECK(v.var == v2.var);
    CHECK(v.value.y == v2.value.y);
    CHECK(v.value.z == v2.value.z);

    test_pb_st8 st1{1, {3, 4}, {5, {7, 8}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st8 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z.t.y == st2.z.t.y);
  }

  {
    // repeated messages
    test_pb_st9 st1{1, {2, 4, 6}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st9 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    auto append = [](std::string& out, uint8_t byte) {
      out.push_back(static_cast<char>(byte));
    };

    std::string unpacked;
    append(unpacked, 8);
    append(unpacked, 1);
    append(unpacked, 16);
    append(unpacked, 2);
    append(unpacked, 16);
    append(unpacked, 4);
    append(unpacked, 16);
    append(unpacked, 6);
    append(unpacked, 26);
    append(unpacked, 4);
    unpacked.append("test");

    test_pb_st9 st1;
    iguana::from_pb(st1, unpacked);
    CHECK(st1.x == 1);
    CHECK(st1.y == std::vector<int>{2, 4, 6});
    CHECK(st1.z == "test");

    std::string mixed;
    append(mixed, 8);
    append(mixed, 1);
    append(mixed, 18);
    append(mixed, 2);
    append(mixed, 2);
    append(mixed, 4);
    append(mixed, 16);
    append(mixed, 6);
    append(mixed, 26);
    append(mixed, 4);
    mixed.append("test");

    test_pb_st9 st2;
    iguana::from_pb(st2, mixed);
    CHECK(st2.x == 1);
    CHECK(st2.y == std::vector<int>{2, 4, 6});
    CHECK(st2.z == "test");

    std::string multi_packed;
    append_tag(multi_packed, 1, iguana::WireType::Varint);
    append_varint(multi_packed, 1);
    append_tag(multi_packed, 2, iguana::WireType::LengthDelimeted);
    append_varint(multi_packed, 2);
    append_varint(multi_packed, 2);
    append_varint(multi_packed, 4);
    append_tag(multi_packed, 2, iguana::WireType::LengthDelimeted);
    append_varint(multi_packed, 1);
    append_varint(multi_packed, 6);
    append_tag(multi_packed, 3, iguana::WireType::LengthDelimeted);
    append_varint(multi_packed, 4);
    multi_packed.append("test");

    test_pb_st9 st3;
    iguana::from_pb(st3, multi_packed);
    CHECK(st3.x == 1);
    CHECK(st3.y == std::vector<int>{2, 4, 6});
    CHECK(st3.z == "test");
  }
  {
    std::string str;
    std::string msg_a;
    append_tag(msg_a, 1, iguana::WireType::Varint);
    append_varint(msg_a, 7);
    append_tag(str, 1, iguana::WireType::LengthDelimeted);
    append_varint(str, msg_a.size());
    str.append(msg_a);

    std::string msg_b;
    append_tag(msg_b, 2, iguana::WireType::Varint);
    append_varint(msg_b, 9);
    append_tag(str, 1, iguana::WireType::LengthDelimeted);
    append_varint(str, msg_b.size());
    str.append(msg_b);

    std::string maybe_a;
    append_tag(maybe_a, 1, iguana::WireType::Varint);
    append_varint(maybe_a, 11);
    append_tag(str, 2, iguana::WireType::LengthDelimeted);
    append_varint(str, maybe_a.size());
    str.append(maybe_a);

    std::string maybe_b;
    append_tag(maybe_b, 2, iguana::WireType::Varint);
    append_varint(maybe_b, 13);
    append_tag(str, 2, iguana::WireType::LengthDelimeted);
    append_varint(str, maybe_b.size());
    str.append(maybe_b);

    test_pb_merge_outer st;
    iguana::from_pb(st, str);
    CHECK(st.msg.a == 7);
    CHECK(st.msg.b == 9);
    REQUIRE(st.maybe.has_value());
    CHECK(st.maybe->a == 11);
    CHECK(st.maybe->b == 13);
  }
  {
    std::string str;
    std::string choice_a;
    append_tag(choice_a, 1, iguana::WireType::Varint);
    append_varint(choice_a, 1);
    append_tag(str, 4, iguana::WireType::LengthDelimeted);
    append_varint(str, choice_a.size());
    str.append(choice_a);

    std::string choice_b;
    append_tag(choice_b, 2, iguana::WireType::Varint);
    append_varint(choice_b, 2);
    append_tag(str, 4, iguana::WireType::LengthDelimeted);
    append_varint(str, choice_b.size());
    str.append(choice_b);

    test_pb_merge_outer st;
    iguana::from_pb(st, str);
    REQUIRE(std::holds_alternative<test_pb_merge_inner>(st.choice));
    CHECK(std::get<test_pb_merge_inner>(st.choice).a == 0);
    CHECK(std::get<test_pb_merge_inner>(st.choice).b == 2);

    append_tag(str, 5, iguana::WireType::LengthDelimeted);
    append_varint(str, 6);
    str.append("winner");
    iguana::from_pb(st, str);
    REQUIRE(std::holds_alternative<std::string>(st.choice));
    CHECK(std::get<std::string>(st.choice) == "winner");
  }
  {
    test_pb_st10 st1{1, {{5, {7, 8}}, {9, {11, 12}}}, "test"};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st10 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t m{1, {3, 4}};
    test_pb_st11 st1{1, {m}, {}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    auto m = std::make_shared<message_t>();
    m->id = 10;
    m->t.x = 11;
    m->t.y = 12;
    std::string s;
    m->to_pb(s);

    std::shared_ptr<iguana::base> base = m;
    std::string str;
    base->to_pb(str);

    CHECK(s == str);

    base->from_pb(str);
  }
  {
    message_t st1{};
    std::string str;
    iguana::to_pb(st1, str);

    message_t st2{};
    iguana::from_pb(st2, str);
    CHECK(st1.id == st2.id);
  }
  {
    test_pb_st11 st1{
        1, {message_t{5, {7, 8}}, message_t{9, {11, 12}}}, {"test"}};
    std::string str;
    iguana::to_pb(st1, str);

    std::string str1;
    st1.to_pb(str1);

    test_pb_st11 st3;
    st3.from_pb(str1);

    CHECK(str == str1);

    test_pb_st11 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(st3.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, "test"}, {2, "ok"}}, {{"test", 4}, {"ok", 6}}};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st12 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{1, {{1, ""}, {0, "ok"}}, {{"", 4}, {"ok", 0}}};
    std::string str;
    iguana::to_pb(st1, str);  // error
    print_hex_str(str);
    test_pb_st12 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    auto append = [](std::string& out, uint8_t byte) {
      out.push_back(static_cast<char>(byte));
    };

    std::string str;
    append(str, 18);
    append(str, 11);
    append(str, 18);
    append(str, 5);
    str.append("first");
    append(str, 72);
    append(str, 1);
    append(str, 8);
    append(str, 7);

    append(str, 18);
    append(str, 8);
    append(str, 18);
    append(str, 4);
    str.append("last");
    append(str, 8);
    append(str, 7);

    test_pb_st12 st;
    iguana::from_pb(st, str);
    REQUIRE(st.y.size() == 1);
    CHECK(st.y.at(7) == "last");
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(1, message_t{2, {3, 4}});
    st1.y.emplace(2, message_t{4, {6, 8}});
    st1.z = "test";
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st13 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(2, message_t{});
    st1.y.emplace(3, message_t{});
    st1.z = "test";
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st13 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // enum
    test_pb_st14 st1{1, colors_t::black, level_t::info};
    std::string str;
    iguana::to_pb(st1, str);

    test_pb_st14 st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // bool float double
    numer_st n{true, 10.25, 4.578}, n1;
    std::string str;
    iguana::to_pb(n, str);

    iguana::from_pb(n1, str);
    CHECK(n1.a == n.a);
    CHECK(n1.b == n.b);
    CHECK(n1.c == n.c);
  }
}

TEST_CASE("proto3 wire-only invalid input handling") {
  {
    std::string str;
    append_varint(str, 0);
    append_varint(str, 1);
    test_pb_st9 st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
  {
    std::string str;
    append_varint(str, (1U << 3U) | 6U);
    append_varint(str, 1);
    test_pb_st9 st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
  {
    std::string str;
    append_tag(str, 3, iguana::WireType::LengthDelimeted);
    append_varint(str, 5);
    str.push_back('x');
    test_pb_st9 st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
  {
    std::string str;
    append_tag(str, 2, iguana::WireType::LengthDelimeted);
    append_varint(str, 1);
    str.push_back(static_cast<char>(0x80));
    append_tag(str, 3, iguana::WireType::LengthDelimeted);
    append_varint(str, 1);
    str.push_back('x');
    test_pb_st9 st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
  {
    std::string str;
    append_tag(str, 1, iguana::WireType::Varint);
    for (int i = 0; i < 9; ++i) {
      str.push_back(static_cast<char>(0x80));
    }
    str.push_back(static_cast<char>(0x02));
    test_pb_st9 st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
#ifdef YLT_USE_CXX26_REFLECTION
  {
    std::string str;
    append_tag(str, 7, iguana::WireType::LengthDelimeted);
    append_varint(str, 5);
    str.append(5, '\0');
    test_pb_annotation_wire_container st;
    CHECK_THROWS_AS(iguana::from_pb(st, str), std::invalid_argument);
  }
#endif
}

TEST_CASE("test members") {
  using namespace iguana;
  using namespace iguana::detail;

  my_space::inner_struct inner{41, 42, 43};
  const auto &map = detail::get_members(inner);
  std::visit(
      [&inner](auto &member) mutable {
        CHECK(member.field_no == 9);
        CHECK(member.field_name == "b");
        CHECK(member.value(inner) == 42);
      },
      map.at(9));

  point_t pt{2, 3};
  const auto &arr1 = detail::get_members(pt);
  auto &val = arr1.at(1);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

struct test_variant {
  test_variant() = default;
  test_variant(int a, std::variant<double, std::string, int> b, double c)
      : x(a), y(std::move(b)), z(c) {}
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
YLT_REFL(test_variant, x, y, z);

TEST_CASE("test variant") {
  // {
  //   constexpr auto tp = detail::get_members_tuple<test_variant>();
  //   static_assert(std::get<0>(tp).field_no == 1);
  //   static_assert(std::get<1>(tp).field_no == 2);
  //   static_assert(std::get<2>(tp).field_no == 3);
  //   static_assert(std::get<3>(tp).field_no == 4);
  //   static_assert(std::get<4>(tp).field_no == 5);
  // }
  // {
  //   constexpr static auto map = detail::get_members<test_variant>();
  //   static_assert(map.find(1) != map.end());
  //   static_assert(map.find(2) != map.end());
  //   static_assert(map.find(3) != map.end());
  //   static_assert(map.find(4) != map.end());
  //   auto val1 = map.find(2);
  //   auto val2 = map.find(3);
  //   std::visit(
  //       [](auto &member) mutable {
  //         CHECK(member.field_no == 2);
  //         CHECK(member.field_name == "y");
  //       },
  //       val1->second);
  //   std::visit(
  //       [](auto &member) mutable {
  //         CHECK(member.field_no == 3);
  //         CHECK(member.field_name == "y");
  //       },
  //       val2->second);
  // }
  {
    test_variant st1 = {5, "Hello, variant!", 3.14};
    std::string str;
    iguana::to_pb(st1, str);
    test_variant st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<std::string>(st2.y) == "Hello, variant!");
  }
  {
    test_variant st1;
    st1.x = 5;
    st1.y.emplace<double>(3.88);
    st1.z = 3.14;
    std::string str;
    iguana::to_pb(st1, str);
    test_variant st2;
    iguana::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<double>(st2.y) == 3.88);
  }
}
// hand-written get_members_impl
struct Person362 {
  std::string name;
  int32_t age;
  std::vector<std::string> emails;
};
inline auto get_members_impl(Person362 *) {
  return std::make_tuple(
      iguana::build_pb_field<&Person362::name, 10>("name"),
      iguana::build_pb_field<&Person362::age, 20>("age"),
      iguana::build_pb_field<&Person362::emails, 9>("emails"));
}
YLT_REFL(Person362, name, age, emails);

// YLT_REFL_PB macro
struct Person362Macro {
  std::string name;
  int32_t age;
  std::vector<std::string> emails;
};
YLT_REFL_PB(Person362Macro, (name, 10), (age, 20), (emails, 9));

#ifdef YLT_USE_CXX26_REFLECTION
struct Person362Annotation {
  [[= iguana::pb_field(10)]] std::string name;
  [[= iguana::pb_field(20)]] int32_t age;
  [[= iguana::pb_field(9)]] std::vector<std::string> emails;
};
#endif

TEST_CASE("test issue 362 custom field numbers") {
  {
    std::string str;
    iguana::to_proto<Person362>(str, "Demo");
    std::cout << str;
    CHECK(str.find("syntax = \"proto3\";") != std::string::npos);
    CHECK(str.find("package Demo;") != std::string::npos);
    CHECK(str.find("message Person362") != std::string::npos);
    CHECK(str.find("name = 10;") != std::string::npos);
    CHECK(str.find("age = 20;") != std::string::npos);
    CHECK(str.find("emails = 9;") != std::string::npos);

    Person362 p1{"Alice", 30, {"a@b.com", "c@d.com"}};
    std::string buf;
    iguana::to_pb(p1, buf);
    Person362 p2;
    iguana::from_pb(p2, buf);
    CHECK(p2.name == p1.name);
    CHECK(p2.age == p1.age);
    CHECK(p2.emails == p1.emails);

    std::string json;
    iguana::to_json(p1, json);
    CHECK(json.find("\"name\"") != std::string::npos);
    CHECK(json.find("\"age\"") != std::string::npos);
  }

  // ── Method 2: YLT_REFL_PB macro ──
  {
    std::string str;
    iguana::to_proto<Person362Macro>(str, "Demo");
    std::cout << str;
    CHECK(str.find("syntax = \"proto3\";") != std::string::npos);
    CHECK(str.find("package Demo;") != std::string::npos);
    CHECK(str.find("message Person362Macro") != std::string::npos);
    CHECK(str.find("name = 10;") != std::string::npos);
    CHECK(str.find("age = 20;") != std::string::npos);
    CHECK(str.find("emails = 9;") != std::string::npos);

    Person362Macro p1{"Bob", 25, {"x@y.com"}};
    std::string buf;
    iguana::to_pb(p1, buf);
    Person362Macro p2;
    iguana::from_pb(p2, buf);
    CHECK(p2.name == p1.name);
    CHECK(p2.age == p1.age);
    CHECK(p2.emails == p1.emails);

    std::string json;
    iguana::to_json(p1, json);
    CHECK(json.find("\"name\"") != std::string::npos);
    CHECK(json.find("\"age\"") != std::string::npos);

    Person362 q1{"Bob", 25, {"x@y.com"}};
    std::string buf1, buf2;
    iguana::to_pb(q1, buf1);
    iguana::to_pb(p1, buf2);
    CHECK(buf1 == buf2);
  }

#ifdef YLT_USE_CXX26_REFLECTION
  {
    std::string str;
    iguana::to_proto<Person362Annotation>(str, "Demo");
    CHECK(str.find("message Person362Annotation") != std::string::npos);
    CHECK(str.find("name = 10;") != std::string::npos);
    CHECK(str.find("age = 20;") != std::string::npos);
    CHECK(str.find("emails = 9;") != std::string::npos);

    Person362Annotation p1{"Cora", 29, {"c@d.com"}};
    std::string buf;
    iguana::to_pb(p1, buf);
    Person362Annotation p2;
    iguana::from_pb(p2, buf);
    CHECK(p2.name == p1.name);
    CHECK(p2.age == p1.age);
    CHECK(p2.emails == p1.emails);

    Person362Macro macro{"Cora", 29, {"c@d.com"}};
    std::string macro_buf;
    iguana::to_pb(macro, macro_buf);
    CHECK(buf == macro_buf);
  }
#endif
}

// fix bug: missing continuation bit when encoding large filed number
struct large_field_number_test {
  int32_t value;
};
YLT_REFL_PB(large_field_number_test, (value, 128));

TEST_CASE("test for big filed number") {
  large_field_number_test st{300};
  std::string buf;
  iguana::to_pb(st, buf);
  large_field_number_test st2;
  iguana::from_pb(st2, buf);
  CHECK(st2.value == 300);
}

// Forward compatibility
struct fc_old {
  int32_t x;
  std::string tag;
};
YLT_REFL_PB(fc_old, (x, 2), (tag, 8));

struct fc_old_keep_unknown_legacy {
  int32_t x;
  std::string unknown;
  std::string tag;
};

inline auto get_members_impl(fc_old_keep_unknown_legacy *) {
  return iguana::pb_members(
      iguana::pb_field<&fc_old_keep_unknown_legacy::x, 2>("x"),
      iguana::pb_unknown_fields_field<&fc_old_keep_unknown_legacy::unknown>(
          "unknown"),
      iguana::pb_field<&fc_old_keep_unknown_legacy::tag, 8>("tag"));
}

#ifdef YLT_USE_CXX26_REFLECTION
struct fc_old_keep_unknown {
  [[= iguana::pb_field(2)]] int32_t x;
  [[= iguana::pb_unknown_fields]] std::string unknown;
  [[= iguana::pb_field(8)]] std::string tag;
};
#endif

// WireType::Varint — int32, int64, bool, sint32, sint64
struct fc_new_int32 {
  int32_t x;
  int32_t extra;
  std::string tag;
};
struct fc_new_int64 {
  int32_t x;
  int64_t extra;
  std::string tag;
};
struct fc_new_bool {
  int32_t x;
  bool extra;
  std::string tag;
};
struct fc_new_sint32 {
  int32_t x;
  iguana::sint32_t extra;
  std::string tag;
};
struct fc_new_sint64 {
  int32_t x;
  iguana::sint64_t extra;
  std::string tag;
};
YLT_REFL_PB(fc_new_int32, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_int64, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_bool, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_sint32, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_sint64, (x, 2), (extra, 4), (tag, 8));

// WireType::Fixed32 — float, fixed32_t, sfixed32_t
struct fc_new_float {
  int32_t x;
  float extra;
  std::string tag;
};
struct fc_new_fixed32 {
  int32_t x;
  iguana::fixed32_t extra;
  std::string tag;
};
struct fc_new_sfixed32 {
  int32_t x;
  iguana::sfixed32_t extra;
  std::string tag;
};
YLT_REFL_PB(fc_new_float, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_fixed32, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_sfixed32, (x, 2), (extra, 4), (tag, 8));

// WireType::Fixed64 — double, fixed64_t, sfixed64_t
struct fc_new_double {
  int32_t x;
  double extra;
  std::string tag;
};
struct fc_new_fixed64 {
  int32_t x;
  iguana::fixed64_t extra;
  std::string tag;
};
struct fc_new_sfixed64 {
  int32_t x;
  iguana::sfixed64_t extra;
  std::string tag;
};
YLT_REFL_PB(fc_new_double, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_fixed64, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_sfixed64, (x, 2), (extra, 4), (tag, 8));

// WireType::LengthDelimited — string, packed vector, nested message
struct fc_inner {
  int32_t v;
};
YLT_REFL(fc_inner, v);
struct fc_new_string {
  int32_t x;
  std::string extra;
  std::string tag;
};
struct fc_new_vec {
  int32_t x;
  std::vector<int32_t> extra;
  std::string tag;
};
struct fc_new_msg {
  int32_t x;
  fc_inner extra;
  std::string tag;
};
YLT_REFL_PB(fc_new_string, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_vec, (x, 2), (extra, 4), (tag, 8));
YLT_REFL_PB(fc_new_msg, (x, 2), (extra, 4), (tag, 8));

// variant (oneof) as unknown field — wire type follows the active alternative
struct fc_new_variant {
  int32_t x;
  std::variant<int32_t, double> extra;
  std::string tag;
};
YLT_REFL_PB(fc_new_variant, (x, 2), (extra, 4), (tag, 8));

// Helper: serialize New → deserialize with fc_old → verify retained fields
template <typename New>
static void check_fc(const New &src) {
  std::string buf;
  iguana::to_pb(src, buf);
  fc_old obj{};
  REQUIRE_NOTHROW(iguana::from_pb(obj, buf));
  CHECK(obj.x == src.x);
  CHECK(obj.tag == src.tag);
}

template <typename New>
static void check_fc_unknown_roundtrip_legacy(const New &src) {
  std::string buf;
  iguana::to_pb(src, buf);

  fc_old_keep_unknown_legacy obj{};
  REQUIRE_NOTHROW(iguana::from_pb(obj, buf));
  CHECK(obj.x == src.x);
  CHECK(obj.tag == src.tag);
  CHECK(!obj.unknown.empty());

  std::string out;
  iguana::to_pb(obj, out);
  New back{};
  REQUIRE_NOTHROW(iguana::from_pb(back, out));
  CHECK(back.x == src.x);
  CHECK(back.extra == src.extra);
  CHECK(back.tag == src.tag);
}

#ifdef YLT_USE_CXX26_REFLECTION
template <typename New>
static void check_fc_unknown_roundtrip(const New &src) {
  std::string buf;
  iguana::to_pb(src, buf);

  fc_old_keep_unknown obj{};
  REQUIRE_NOTHROW(iguana::from_pb(obj, buf));
  CHECK(obj.x == src.x);
  CHECK(obj.tag == src.tag);
  CHECK(!obj.unknown.empty());

  std::string out;
  iguana::to_pb(obj, out);
  New back{};
  REQUIRE_NOTHROW(iguana::from_pb(back, out));
  CHECK(back.x == src.x);
  CHECK(back.extra == src.extra);
  CHECK(back.tag == src.tag);
}
#endif

TEST_CASE("forward compatibility - skip unknown fields of all wire types") {
  // Varint
  check_fc(fc_new_int32{42, 99, "hello"});
  check_fc(fc_new_int64{42, 9999999999LL, "hello"});
  check_fc(fc_new_bool{42, true, "hello"});
  check_fc(fc_new_sint32{42, iguana::sint32_t{-7}, "hello"});
  check_fc(fc_new_sint64{42, iguana::sint64_t{-100}, "hello"});
  // Fixed32
  check_fc(fc_new_float{42, 3.14f, "hello"});
  check_fc(fc_new_fixed32{42, iguana::fixed32_t{0xDEADBEEFu}, "hello"});
  check_fc(fc_new_sfixed32{42, iguana::sfixed32_t{-1}, "hello"});
  // Fixed64
  check_fc(fc_new_double{42, 3.14159265358979, "hello"});
  check_fc(fc_new_fixed64{42, iguana::fixed64_t{0xCAFEBABEu}, "hello"});
  check_fc(fc_new_sfixed64{42, iguana::sfixed64_t{-999}, "hello"});
  // LengthDelimited
  check_fc(fc_new_string{42, "unknown_data", "hello"});
  check_fc(fc_new_vec{42, {1, 2, 3, 4, 5}, "hello"});
  check_fc(fc_new_msg{42, {7}, "hello"});
  // variant (oneof) — active = int32 (Varint)
  check_fc(fc_new_variant{42, int32_t{55}, "hello"});
  // variant (oneof) — active = double (Fixed64)
  check_fc(fc_new_variant{42, double{2.718}, "hello"});
}

TEST_CASE("legacy preserve unknown fields descriptor") {
  check_fc_unknown_roundtrip_legacy(fc_new_int32{42, 99, "hello"});
  check_fc_unknown_roundtrip_legacy(
      fc_new_double{42, 3.14159265358979, "hello"});
  check_fc_unknown_roundtrip_legacy(
      fc_new_string{42, "unknown_data", "hello"});

  fc_old old{42, "hello"};
  std::string buf;
  iguana::to_pb(old, buf);
  std::string group;
  group.push_back(static_cast<char>((4 << 3) | 3));
  group.push_back(static_cast<char>((5 << 3) | 0));
  group.push_back(static_cast<char>(99));
  group.push_back(static_cast<char>((4 << 3) | 4));
  buf.append(group);

  fc_old_keep_unknown_legacy obj{};
  REQUIRE_NOTHROW(iguana::from_pb(obj, buf));
  CHECK(obj.x == old.x);
  CHECK(obj.tag == old.tag);
  CHECK(obj.unknown == group);

  std::string out;
  iguana::to_pb(obj, out);
  CHECK(out == buf);

  std::string proto;
  iguana::to_proto<fc_old_keep_unknown_legacy, false>(proto);
  CHECK(proto.find(" unknown =") == std::string::npos);
  CHECK(proto.find("int32 x = 2;") != std::string::npos);
  CHECK(proto.find("string  tag = 8;") != std::string::npos);
}

TEST_CASE("unknown group recursion limit") {
  fc_old old{42, "hello"};
  std::string buf;
  iguana::to_pb(old, buf);
  buf.append(make_nested_unknown_group(iguana::detail::max_pb_recursion_depth +
                                       5));

  fc_old_keep_unknown_legacy obj{};
  CHECK_THROWS_AS(iguana::from_pb(obj, buf), std::runtime_error);
}

#ifdef YLT_USE_CXX26_REFLECTION
TEST_CASE("cxx26 preserve unknown fields annotation") {
  check_fc_unknown_roundtrip(fc_new_int32{42, 99, "hello"});
  check_fc_unknown_roundtrip(fc_new_double{42, 3.14159265358979, "hello"});
  check_fc_unknown_roundtrip(fc_new_string{42, "unknown_data", "hello"});

  fc_old old{42, "hello"};
  std::string buf;
  iguana::to_pb(old, buf);
  std::string group;
  group.push_back(static_cast<char>((4 << 3) | 3));
  group.push_back(static_cast<char>((5 << 3) | 0));
  group.push_back(static_cast<char>(99));
  group.push_back(static_cast<char>((4 << 3) | 4));
  buf.append(group);

  fc_old_keep_unknown obj{};
  REQUIRE_NOTHROW(iguana::from_pb(obj, buf));
  CHECK(obj.x == old.x);
  CHECK(obj.tag == old.tag);
  CHECK(obj.unknown == group);

  std::string out;
  iguana::to_pb(obj, out);
  CHECK(out == buf);

  std::string proto;
  iguana::to_proto<fc_old_keep_unknown, false>(proto);
  CHECK(proto.find(" unknown =") == std::string::npos);
  CHECK(proto.find("int32 x = 2;") != std::string::npos);
  CHECK(proto.find("string  tag = 8;") != std::string::npos);
}
#endif

#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
