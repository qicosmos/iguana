#include <filesystem>
#include <fstream>

#include "iguana/pb_writer.hpp"
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/iguana.hpp"
#include "proto/unittest_proto3.h"  // msg reflection

#if defined(STRUCT_PB_WITH_PROTO)
namespace {
static void append_varint(std::string& out, uint64_t value) {
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<char>(value));
}

static void append_tag(std::string& out, uint32_t field_no,
                       iguana::WireType wire_type) {
  append_varint(out, (static_cast<uint64_t>(field_no) << 3) |
                         static_cast<uint32_t>(wire_type));
}

static void append_fixed64(std::string& out, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<char>((value >> (8 * i)) & 0xff));
  }
}

static void append_varint_field(std::string& out, uint32_t field_no,
                                uint64_t value) {
  append_tag(out, field_no, iguana::WireType::Varint);
  append_varint(out, value);
}

static void append_fixed64_field(std::string& out, uint32_t field_no,
                                 uint64_t value) {
  append_tag(out, field_no, iguana::WireType::Fixed64);
  append_fixed64(out, value);
}

static void append_length_delimited(std::string& out, uint32_t field_no,
                                    std::string_view payload) {
  append_tag(out, field_no, iguana::WireType::LengthDelimeted);
  append_varint(out, payload.size());
  out.append(payload.data(), payload.size());
}
}  // namespace

TEST_CASE("test BaseTypeMsg") {
  {  // normal test
    stpb::BaseTypeMsg se_st{
        100, 200, 300, 400, 31.4f, 62.8, false, "World", stpb::Enum::ZERO};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }

  {  // test min and empty str
    stpb::BaseTypeMsg se_st{std::numeric_limits<int32_t>::min(),
                            std::numeric_limits<int64_t>::min(),
                            std::numeric_limits<uint32_t>::min(),
                            std::numeric_limits<uint64_t>::min(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<double>::lowest(),
                            false,
                            "",
                            stpb::Enum::NEG};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }
  {  // test max and long str
    stpb::BaseTypeMsg se_st{std::numeric_limits<int32_t>::max(),
                            std::numeric_limits<int64_t>::max(),
                            std::numeric_limits<uint32_t>::max(),
                            std::numeric_limits<uint64_t>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<double>::max(),
                            true,
                            std::string(1000, 'x'),
                            stpb::Enum::BAZ};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test person and monster") {
  stpb::simple_t2 t{-100, 2, stpb::Color::Blue, 4};
  std::string str;
  iguana::to_pb(t, str);

  stpb::simple_t2 t2;
  iguana::from_pb(t2, str);
  CHECK(t.c == t2.c);

  pb::Simple2 s;
  s.set_a(-100);
  s.set_b(2);
  s.set_c(pb::Color::Blue);
  s.set_d(4);

  std::string pb_str;
  s.SerializeToString(&pb_str);

  CHECK(str == pb_str);
}

TEST_CASE("test person and monster") {
  auto pb_monster = protobuf_sample::create_monster();
  auto sp_monster = create_sp_monster();

  std::string pb_str;
  std::string sp_str;

  pb_monster.SerializeToString(&pb_str);
  iguana::to_pb(sp_monster, sp_str);

  CHECK(pb_str == sp_str);

  mygame::Monster m;
  m.ParseFromString(pb_str);
  CHECK(m.name() == pb_monster.name());

  stpb::Monster spm;
  iguana::from_pb(spm, sp_str);
  CHECK(spm.name == sp_monster.name);

  auto pb_person = protobuf_sample::create_person();
  auto sp_person = create_person();
  pb_person.SerializePartialToString(&pb_str);
  iguana::to_pb(sp_person, sp_str);

  CHECK(pb_str == sp_str);

  mygame::person pp;
  pp.ParseFromString(pb_str);
  CHECK(pp.name() == pb_person.name());

  stpb::person p;
  iguana::from_pb(p, sp_str);
  CHECK(p.name == sp_person.name);
}

TEST_CASE("test IguanaTypeMsg") {
  {  // test normal value
    stpb::IguanaTypeMsg se_st{{100}, {200}, {300}, {400}, {31}, {32}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg{};
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }

  {  // test min value
    stpb::IguanaTypeMsg se_st{{std::numeric_limits<int32_t>::min()},
                              {std::numeric_limits<int64_t>::min()},
                              {std::numeric_limits<uint32_t>::min()},
                              {std::numeric_limits<uint64_t>::min()},
                              {std::numeric_limits<int32_t>::lowest()},
                              {std::numeric_limits<int64_t>::lowest()}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg{};
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::IguanaTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
  {  // test max value
    stpb::IguanaTypeMsg se_st{{std::numeric_limits<int32_t>::max()},
                              {std::numeric_limits<int64_t>::max()},
                              {std::numeric_limits<uint32_t>::max()},
                              {std::numeric_limits<uint64_t>::max()},
                              {std::numeric_limits<int32_t>::max()},
                              {std::numeric_limits<int64_t>::max()}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg;
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
  {  // test empty
    stpb::IguanaTypeMsg se_st{};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg;
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test RepeatBaseTypeMsg") {
  {
    stpb::RepeatBaseTypeMsg se_st{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9},
        {10, 11, 12},
        {13.1, 14.2, 15.3},
        {16.4, 17.5, 18.6},
        {"a", "b", "c"},
        {stpb::Enum::BAZ, stpb::Enum::ZERO, stpb::Enum::NEG}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::RepeatBaseTypeMsg se_msg;
    SetRepeatBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::RepeatBaseTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
  {  // max and min vlaue
    stpb::RepeatBaseTypeMsg se_st{{std::numeric_limits<uint32_t>::max(),
                                   std::numeric_limits<uint32_t>::min()},
                                  {std::numeric_limits<uint64_t>::max(),
                                   std::numeric_limits<uint64_t>::min()},
                                  {std::numeric_limits<int32_t>::max(),
                                   std::numeric_limits<int32_t>::min()},
                                  {std::numeric_limits<int64_t>::max(),
                                   std::numeric_limits<int64_t>::min()},
                                  {},
                                  {std::numeric_limits<double>::max(),
                                   std::numeric_limits<double>::min()},
                                  {"", "", ""},
                                  {stpb::Enum::NEG, stpb::Enum::FOO}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::RepeatBaseTypeMsg se_msg;
    SetRepeatBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::RepeatBaseTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test RepeatIguanaTypeMsg") {
  {
    stpb::RepeatIguanaTypeMsg se_st{
        {{0}, {1}, {3}},    {{4}, {5}, {6}},    {{7}, {8}, {9}},
        {{10}, {11}, {12}}, {{13}, {14}, {15}}, {},
    };
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::RepeatIguanaTypeMsg se_msg;
    SetRepeatIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::RepeatIguanaTypeMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);
    pb::RepeatIguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatIguanaTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test NestedMsg") {
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
  {
    std::string str;
    iguana::to_proto<stpb::NestedMsg>(str, "test");
    std::cout << str;

    const auto proto_path =
        std::filesystem::temp_directory_path() / "iguana_NestedMsg.proto";
    std::ofstream out(proto_path, std::ios::binary);
    iguana::to_proto_file<stpb::NestedMsg>(out, "test");
    out.close();
    std::filesystem::remove(proto_path);
  }
#endif
  {
    stpb::NestedMsg se_st{
        /* base_msg */ stpb::BaseTypeMsg{100, 200, 300, 400, 31.4f, 62.8, false,
                                         "World", stpb::Enum::BAZ},
        /* repeat_base_msg */
        std::vector<stpb::BaseTypeMsg>{
            {1, 2, 3, 4, 5.5f, 6.6, true, "Hello", stpb::Enum::FOO},
            {7, 8, 9, 10, 11.11f, 12.12, false, "Hi", stpb::Enum::BAR}},
        /* iguana_type_msg */
        stpb::IguanaTypeMsg{{100}, {200}, {300}, {400}, {31}, {32}},
        /* repeat_iguna_msg */
        std::vector{stpb::IguanaTypeMsg{{1}, {2}, {3}},
                    stpb::IguanaTypeMsg{{4}, {5}, {6}},
                    stpb::IguanaTypeMsg{{7}, {8}, {9}}},
        /* repeat_repeat_base_msg */
        std::vector<stpb::RepeatBaseTypeMsg>{
            {{1, 2, 3},
             {4, 5, 6},
             {7, 8, 9},
             {10, 11, 12},
             {13.1, 14.2, 15.3},
             {16.4, 17.5, 18.6},
             {"a", "b", "c"},
             {stpb::Enum::FOO, stpb::Enum::BAR, stpb::Enum::BAZ}},
            {{19, 20, 21},
             {22, 23, 24},
             {25, 26, 27},
             {28, 29, 30},
             {31.1, 32.2, 33.3},
             {34.4, 35.5, 36.6},
             {"x", "y", "z"},
             {stpb::Enum::ZERO, stpb::Enum::NEG, stpb::Enum::FOO}}}};

    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::NestedMsg se_msg;
    SetNestedMsg(se_st, se_msg);

    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);

    CHECK(st_ss == pb_ss);

    stpb::NestedMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::NestedMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);

    CheckNestedMsg(dese_st, dese_msg);
  }
  {  // test empty values
    stpb::NestedMsg se_st{
        /* base_msg */ {0, 0, 0, 0, 0.0f, 0.0, true, "", stpb::Enum::ZERO},
        /* repeat_base_msg */ {},
        /* iguana_type_msg */ {},
        /* repeat_iguna_msg */ {},
        /* repeat_repeat_base_msg */ {}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::NestedMsg se_msg;
    SetNestedMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);

    // CHECK(st_ss == pb_ss);
    print_hex_str(st_ss);
    print_hex_str(pb_ss);
    stpb::NestedMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::NestedMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);

    CheckNestedMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test MapMsg") {
  {
    stpb::MapMsg se_st{};

    se_st.sfix64_str_map.emplace(iguana::sfixed64_t{10}, "ten");
    se_st.sfix64_str_map.emplace(iguana::sfixed64_t{20}, "twenty");

    se_st.str_iguana_type_msg_map.emplace(
        "first", stpb::IguanaTypeMsg{{10}, {20}, {30}, {40}, {50}, {60}});
    se_st.str_iguana_type_msg_map.emplace(
        "second", stpb::IguanaTypeMsg{{11}, {21}, {31}, {41}, {51}, {61}});

    se_st.int_repeat_base_msg_map.emplace(
        1, stpb::RepeatBaseTypeMsg{{1, 2},
                                   {3, 4},
                                   {5, 6},
                                   {7, 8},
                                   {9.0f, 10.0f},
                                   {11.0, 12.0},
                                   {"one", "two"},
                                   {stpb::Enum::FOO, stpb::Enum::BAR}});
    se_st.int_repeat_base_msg_map.emplace(
        2, stpb::RepeatBaseTypeMsg{{2, 3},
                                   {4, 5},
                                   {6, 7},
                                   {8, 9},
                                   {10.0f, 11.0f},
                                   {12.0, 13.0},
                                   {"three", "four"},
                                   {stpb::Enum::BAZ, stpb::Enum::NEG}});

    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::MapMsg se_msg{};
    SetMapMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    // It's okay not to satisfy this.
    // CHECK(st_ss == pb_ss);
    CHECK(st_ss.size() == pb_ss.size());
    stpb::MapMsg dese_st{};
    iguana::from_pb(dese_st, pb_ss);
    pb::MapMsg dese_msg;
    dese_msg.ParseFromString(st_ss);
    CheckMapMsg(dese_st, dese_msg);
  }
  {
    // key empty
    stpb::MapMsg se_st{};
    se_st.sfix64_str_map.emplace(iguana::sfixed64_t{30}, "");
    se_st.str_iguana_type_msg_map.emplace(
        "", stpb::IguanaTypeMsg{{0}, {0}, {0}, {0}, {0}, {0}});
    se_st.int_repeat_base_msg_map.emplace(
        3, stpb::RepeatBaseTypeMsg{{}, {}, {}, {}, {}, {}, {}, {}});
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::MapMsg se_msg{};
    SetMapMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::MapMsg dese_st{};
    iguana::from_pb(dese_st, pb_ss);
    pb::MapMsg dese_msg;
    dese_msg.ParseFromString(st_ss);
    CheckMapMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test BaseOneofMsg") {
  {  // test double
    stpb::BaseOneofMsg se_st{123, 3.14159, 456.78};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    // print_hex_str(st_ss);
    // print_hex_str(pb_ss);
    stpb::BaseOneofMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test string
    stpb::BaseOneofMsg se_st{123, std::string("Hello"), 456.78};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseOneofMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test BaseTypeMsg
    stpb::BaseTypeMsg baseTypeMsg{
        100, 200, 300, 400, 31.4f, 62.8, false, "World", stpb::Enum::BAZ};
    stpb::BaseOneofMsg se_st{123, baseTypeMsg, 456.78};

    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseOneofMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test empty variant
    stpb::BaseOneofMsg se_st{123, {}, 456.78};

    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    print_hex_str(st_ss);
    print_hex_str(pb_ss);
    stpb::BaseOneofMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test NestOneofMsg ") {
  {  // Test BaseOneofMsg
    stpb::BaseOneofMsg baseOneof{123, std::string("Hello"), 456.78};
    stpb::NestOneofMsg se_st{{baseOneof}};

    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::NestOneofMsg se_msg;
    SetNestOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::NestOneofMsg dese_st{};
    iguana::from_pb(dese_st, st_ss);

    pb::NestOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckNestOneofMsg(dese_st, dese_msg);
  }
}

TEST_CASE("proto3 protoc wire compatibility edge cases") {
  SUBCASE("fragmented and mixed packed repeated fields") {
    std::string wire;
    std::string packed_first;
    append_varint(packed_first, 1);
    append_varint(packed_first, 2);
    append_length_delimited(wire, 1, packed_first);
    append_varint_field(wire, 1, 3);

    std::string packed_second;
    append_varint(packed_second, 4);
    append_varint(packed_second, 5);
    append_length_delimited(wire, 1, packed_second);

    append_length_delimited(wire, 7, "alpha");
    append_length_delimited(wire, 7, "beta");
    append_varint_field(wire, 8, 1);
    std::string packed_enum;
    append_varint(packed_enum, 123456);
    append_varint(packed_enum, 0);
    append_length_delimited(wire, 8, packed_enum);

    pb::RepeatBaseTypeMsg pb_msg;
    REQUIRE(pb_msg.ParseFromString(wire));

    stpb::RepeatBaseTypeMsg st_msg{};
    iguana::from_pb(st_msg, wire);

    const std::vector<uint32_t> expected_uint32{1, 2, 3, 4, 5};
    const std::vector<std::string> expected_string{"alpha", "beta"};
    const std::vector<stpb::Enum> expected_enum{
        stpb::Enum::FOO, stpb::Enum::BAZ, stpb::Enum::ZERO};
    CHECK(st_msg.repeated_uint32 == expected_uint32);
    CHECK(st_msg.repeated_string == expected_string);
    CHECK(st_msg.repeated_enum == expected_enum);
    CHECK(pb_msg.repeated_uint32_size() == 5);
    CHECK(pb_msg.repeated_string_size() == 2);
    CHECK(pb_msg.repeated_enum_size() == 3);
    CheckRepeatBaseTypeMsg(st_msg, pb_msg);
  }

  SUBCASE("singular message fields merge across chunks") {
    std::string wire;
    std::string base_first;
    append_varint_field(base_first, 1, 11);
    append_length_delimited(wire, 1, base_first);

    std::string base_second;
    append_varint_field(base_second, 3, 33);
    append_length_delimited(base_second, 8, "merged");
    append_length_delimited(wire, 1, base_second);

    append_varint_field(wire, 100, 999);

    pb::NestedMsg pb_msg;
    REQUIRE(pb_msg.ParseFromString(wire));

    stpb::NestedMsg st_msg{};
    iguana::from_pb(st_msg, wire);

    CHECK(pb_msg.has_base_msg());
    CHECK(pb_msg.base_msg().optional_int32() == 11);
    CHECK(pb_msg.base_msg().optional_uint32() == 33);
    CHECK(pb_msg.base_msg().optional_string() == "merged");
    CheckNestedMsg(st_msg, pb_msg);
  }

  SUBCASE("oneof uses the last field seen") {
    std::string wire;
    append_varint_field(wire, 1, 100);

    std::string oneof_msg;
    append_varint_field(oneof_msg, 1, 7);
    append_length_delimited(oneof_msg, 8, "ignored");
    append_length_delimited(wire, 4, oneof_msg);
    append_length_delimited(wire, 3, "final");

    pb::BaseOneofMsg pb_msg;
    REQUIRE(pb_msg.ParseFromString(wire));

    stpb::BaseOneofMsg st_msg{};
    iguana::from_pb(st_msg, wire);

    CHECK(pb_msg.one_of_case() == pb::BaseOneofMsg::kOneOfString);
    CHECK(pb_msg.one_of_string() == "final");
    REQUIRE(std::holds_alternative<std::string>(st_msg.one_of));
    CHECK(std::get<std::string>(st_msg.one_of) == "final");
    CheckBaseOneofMsg(st_msg, pb_msg);
  }

  SUBCASE("map entries allow out-of-order fields and duplicate keys") {
    std::string wire;

    std::string first_sfixed_entry;
    append_fixed64_field(first_sfixed_entry, 1, 42);
    append_length_delimited(first_sfixed_entry, 2, "first");
    append_length_delimited(wire, 1, first_sfixed_entry);

    std::string second_sfixed_entry;
    append_length_delimited(second_sfixed_entry, 2, "last");
    append_fixed64_field(second_sfixed_entry, 1, 42);
    append_length_delimited(wire, 1, second_sfixed_entry);

    std::string repeat_value;
    std::string packed_nums;
    append_varint(packed_nums, 9);
    append_varint(packed_nums, 10);
    append_length_delimited(repeat_value, 1, packed_nums);
    append_length_delimited(repeat_value, 7, "late");

    std::string int_entry;
    append_varint_field(int_entry, 99, 123);
    append_length_delimited(int_entry, 2, repeat_value);
    append_varint_field(int_entry, 1, 7);
    append_length_delimited(wire, 3, int_entry);

    pb::MapMsg pb_msg;
    REQUIRE(pb_msg.ParseFromString(wire));

    stpb::MapMsg st_msg{};
    iguana::from_pb(st_msg, wire);

    auto pb_sfixed_it = pb_msg.sfix64_str_map().find(42);
    REQUIRE(pb_sfixed_it != pb_msg.sfix64_str_map().end());
    CHECK(pb_sfixed_it->second == "last");

    auto st_sfixed_it = st_msg.sfix64_str_map.find(iguana::sfixed64_t{42});
    REQUIRE(st_sfixed_it != st_msg.sfix64_str_map.end());
    CHECK(st_sfixed_it->second == "last");

    auto pb_int_it = pb_msg.int_repeat_base_msg_map().find(7);
    REQUIRE(pb_int_it != pb_msg.int_repeat_base_msg_map().end());
    CHECK(pb_int_it->second.repeated_uint32_size() == 2);
    CHECK(pb_int_it->second.repeated_string_size() == 1);

    auto st_int_it = st_msg.int_repeat_base_msg_map.find(7);
    REQUIRE(st_int_it != st_msg.int_repeat_base_msg_map.end());
    const std::vector<uint32_t> expected_nums{9, 10};
    const std::vector<std::string> expected_strings{"late"};
    CHECK(st_int_it->second.repeated_uint32 == expected_nums);
    CHECK(st_int_it->second.repeated_string == expected_strings);
    CheckMapMsg(st_msg, pb_msg);
  }
}
#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
