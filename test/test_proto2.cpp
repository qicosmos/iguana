#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "iguana/pb_reader.hpp"
#include "iguana/pb_writer.hpp"
#include "proto/unittest_proto2.h"  // msg reflection

TEST_CASE("test SimpleMsg") {
  {
    stpb2::SimpleMsg st;
    st.optional_string.val = "Hello";
    st.optional_int32.val = 42;
    st.repeated_sint32.val = std::vector<iguana::sint32_t>{{1}, {2}, {3}};
    st.repeated_int64 = std::vector<int64_t>{100, 200, 3};

    std::string st_ss;
    iguana::to_pb<true>(st, st_ss);

    pb2::SimpleMsg msg;
    SetSimpleMsg(st, msg);
    std::string pb_ss;
    msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb2::SimpleMsg deserialized_st;
    iguana::from_pb(deserialized_st, st_ss);
    pb2::SimpleMsg deserialized_msg;
    deserialized_msg.ParseFromString(pb_ss);
    CheckSimpleMsg(deserialized_st, deserialized_msg);
  }
  {
    stpb2::SimpleMsg st;
    std::string st_ss;
    iguana::to_pb<true>(st, st_ss);

    pb2::SimpleMsg msg;
    std::string pb_ss;
    msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb2::SimpleMsg deserialized_st;
    iguana::from_pb(deserialized_st, st_ss);
    pb2::SimpleMsg deserialized_msg;
    deserialized_msg.ParseFromString(pb_ss);
    CheckSimpleMsg(deserialized_st, deserialized_msg);
  }
  {
    stpb2::SimpleMsg st;
    st.optional_string.val = "Hello";
    st.optional_int32.val = 42;

    std::string st_ss;
    iguana::to_pb<true>(st, st_ss);

    pb2::SimpleMsg msg;
    SetSimpleMsg(st, msg);
    std::string pb_ss;
    msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb2::SimpleMsg deserialized_st;
    iguana::from_pb(deserialized_st, st_ss);
    pb2::SimpleMsg deserialized_msg;
    deserialized_msg.ParseFromString(pb_ss);
    CheckSimpleMsg(deserialized_st, deserialized_msg);
  }
}

TEST_CASE("test NestedMsg") {
  {
    stpb2::NestedMsg st;
    auto& simple_msg = st.msg.val;
    simple_msg.optional_string.val = "Hello";
    simple_msg.optional_int32.val = 42;
    simple_msg.repeated_sint32.val =
        std::vector<iguana::sint32_t>{{1}, {2}, {3}};
    simple_msg.repeated_int64 = std::vector<int64_t>{100, 200, 3};

    std::string st_ss;
    iguana::to_pb<true>(st, st_ss);

    pb2::NestedMsg msg;
    SetNestedMsg(st, msg);
    std::string pb_ss;
    msg.SerializeToString(&pb_ss);
    print_hex_str(st_ss);
    print_hex_str(pb_ss);
    CHECK(st_ss == pb_ss);

    stpb2::NestedMsg deserialized_st;
    iguana::from_pb(deserialized_st, st_ss);
    pb2::NestedMsg deserialized_msg;
    deserialized_msg.ParseFromString(pb_ss);
    CheckNestedMsg(deserialized_st, deserialized_msg);
  }
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
