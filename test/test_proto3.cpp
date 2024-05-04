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
        .optional_enum = stpb::Enum::BAZ,
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

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP