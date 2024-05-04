#include <iostream>

#include "proto/unittest_proto3.h"  // msg reflection
#include "unittest_proto3.pb.h"     // protoc gen

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

void SetBaseTypeMsg(const stpb::BaseTypeMsg& st, pb::BaseTypeMsg& msg) {
  msg.set_optional_int32(st.optional_int32);
  msg.set_optional_int64(st.optional_int64);
  msg.set_optional_uint32(st.optional_uint32);
  msg.set_optional_uint64(st.optional_uint64);
  msg.set_optional_float(st.optional_float);
  msg.set_optional_double(st.optional_double);
  msg.set_optional_bool(st.optional_bool);
  msg.set_optional_string(st.optional_string);
  msg.set_optional_enum(static_cast<pb::Enum>(st.optional_enum));
}

void CheckBaseTypeMsg(const stpb::BaseTypeMsg& st, const pb::BaseTypeMsg& msg) {
  CHECK(st.optional_int32 == msg.optional_int32());
  CHECK(st.optional_int64 == msg.optional_int64());
  CHECK(st.optional_uint32 == msg.optional_uint32());
  CHECK(st.optional_uint64 == msg.optional_uint64());
  CHECK(st.optional_float == msg.optional_float());
  CHECK(st.optional_double == msg.optional_double());
  CHECK(st.optional_bool == msg.optional_bool());
  CHECK(st.optional_string == msg.optional_string());
  CHECK(static_cast<int32_t>(st.optional_enum) ==
        static_cast<int32_t>(msg.optional_enum()));
}

TEST_CASE("test BaseTypeMsg") {
  {
    stpb::BaseTypeMsg se_st{
        .optional_int32 = 100,
        .optional_int64 = 200,
        .optional_uint32 = 300,
        .optional_uint64 = 400,
        .optional_float = 31.4f,
        .optional_double = 62.8,
        .optional_bool = false,
        .optional_string = "World",
        .optional_enum = stpb::Enum::ZERO,
    };
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st;
    iguana::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }
}

void SetIguanaTypeMsg(const stpb::IguanaTypeMsg& st, pb::IguanaTypeMsg& msg) {
  msg.set_optional_sint32(st.optional_sint32.val);
  msg.set_optional_sint64(st.optional_sint64.val);
  msg.set_optional_fixed32(st.optional_fixed32.val);
  msg.set_optional_fixed64(st.optional_fixed64.val);
  msg.set_optional_sfixed32(st.optional_sfixed32.val);
  msg.set_optional_sfixed64(st.optional_sfixed64.val);
}

void CheckIguanaTypeMsg(const stpb::IguanaTypeMsg& st,
                        const pb::IguanaTypeMsg& msg) {
  CHECK(st.optional_sint32 == msg.optional_sint32());
  CHECK(st.optional_sint64 == msg.optional_sint64());
  CHECK(st.optional_fixed32 == msg.optional_fixed32());
  CHECK(st.optional_fixed64 == msg.optional_fixed64());
  CHECK(st.optional_sfixed32 == msg.optional_sfixed32());
  CHECK(st.optional_sfixed64 == msg.optional_sfixed64());
}

TEST_CASE("test IguanaTypeMsg") {
  {
    stpb::IguanaTypeMsg se_st{{100}, {200}, {300}, {400}, {31}, {32}};
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg;
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st;
    iguana::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
}

void SetRepeatBaseTypeMsg(const stpb::RepeatBaseTypeMsg& st,
                          pb::RepeatBaseTypeMsg& msg) {
  for (auto v : st.repeated_uint32) {
    msg.add_repeated_uint32(v);
  }
  for (auto v : st.repeated_uint64) {
    msg.add_repeated_uint64(v);
  }
  for (auto v : st.repeated_int32) {
    msg.add_repeated_int32(v);
  }
  for (auto v : st.repeated_int64) {
    msg.add_repeated_int64(v);
  }
  for (auto v : st.repeated_float) {
    msg.add_repeated_float(v);
  }
  for (auto v : st.repeated_double) {
    msg.add_repeated_double(v);
  }
  for (auto v : st.repeated_string) {
    msg.add_repeated_string(v);
  }
  for (auto v : st.repeated_enum) {
    msg.add_repeated_enum(static_cast<pb::Enum>(v));
  }
}

void CheckRepeatBaseTypeMsg(const stpb::RepeatBaseTypeMsg& st,
                            const pb::RepeatBaseTypeMsg& msg) {
  for (int i = 0; i < st.repeated_uint32.size(); ++i) {
    CHECK(st.repeated_uint32[i] == msg.repeated_uint32(i));
  }
  for (int i = 0; i < st.repeated_uint64.size(); ++i) {
    CHECK(st.repeated_uint64[i] == msg.repeated_uint64(i));
  }
  for (int i = 0; i < st.repeated_int32.size(); ++i) {
    CHECK(st.repeated_int32[i] == msg.repeated_int32(i));
  }
  for (int i = 0; i < st.repeated_int64.size(); ++i) {
    CHECK(st.repeated_int64[i] == msg.repeated_int64(i));
  }
  for (int i = 0; i < st.repeated_float.size(); ++i) {
    CHECK(st.repeated_float[i] == msg.repeated_float(i));
  }
  for (int i = 0; i < st.repeated_double.size(); ++i) {
    CHECK(st.repeated_double[i] == msg.repeated_double(i));
  }
  for (int i = 0; i < st.repeated_string.size(); ++i) {
    CHECK(st.repeated_string[i] == msg.repeated_string(i));
  }
  for (int i = 0; i < st.repeated_enum.size(); ++i) {
    CHECK(static_cast<int>(st.repeated_enum[i]) ==
          static_cast<int>(msg.repeated_enum(i)));
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

    stpb::RepeatBaseTypeMsg dese_st;
    iguana::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
  {
    // max and min vlaue
    stpb::RepeatBaseTypeMsg se_st{
        {std::numeric_limits<uint32_t>::max(),
         std::numeric_limits<uint32_t>::min()},
        {std::numeric_limits<uint64_t>::max(),
         std::numeric_limits<uint64_t>::min()},
        {std::numeric_limits<int32_t>::max(),
         std::numeric_limits<int32_t>::min()},
        {std::numeric_limits<int64_t>::max(),
         std::numeric_limits<int64_t>::min()},
        {std::numeric_limits<float>::max(), std::numeric_limits<float>::min()},
        {std::numeric_limits<double>::max(),
         std::numeric_limits<double>::min()},
        {"", "", ""},                         // Empty strings
        {stpb::Enum::NEG, stpb::Enum::FOO}};  // Include negative enum
    std::string st_ss;
    iguana::to_pb(se_st, st_ss);

    pb::RepeatBaseTypeMsg se_msg;
    SetRepeatBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::RepeatBaseTypeMsg dese_st;
    iguana::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP