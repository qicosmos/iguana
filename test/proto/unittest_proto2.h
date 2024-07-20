#pragma once
#include "unittest_proto2.pb.h"  // protoc gen
namespace stpb2 {

struct SimpleMsg {
  iguana::optional_t<std::string> optional_string{"kim"};
  iguana::optional_t<int32_t> optional_int32;
  iguana::packed_t<iguana::sint32_t> repeated_sint32;
  std::vector<int64_t> repeated_int64;
};
REFLECTION(SimpleMsg, optional_string, optional_int32, repeated_sint32,
           repeated_int64);

struct NestedMsg {
  iguana::optional_t<SimpleMsg> msg;
  std::vector<SimpleMsg> repeated_simple_msg;
};
REFLECTION(NestedMsg, msg, repeated_simple_msg);
}  // namespace stpb2

void CheckSimpleMsg(const stpb2::SimpleMsg& st, const pb2::SimpleMsg& msg) {
  CHECK(st.optional_string.val == msg.optional_string());

  if (msg.has_optional_int32()) {
    CHECK(st.optional_int32.val == msg.optional_int32());
  }

  CHECK(st.repeated_sint32.val.size() == msg.repeated_sint32().size());
  for (size_t i = 0; i < st.repeated_sint32.val.size(); ++i) {
    CHECK(st.repeated_sint32.val[i] == msg.repeated_sint32()[i]);
  }

  CHECK(st.repeated_int64.size() == msg.repeated_int64().size());
  for (size_t i = 0; i < st.repeated_int64.size(); ++i) {
    CHECK(st.repeated_int64[i] == msg.repeated_int64()[i]);
  }
}

void SetSimpleMsg(const stpb2::SimpleMsg& st, pb2::SimpleMsg& msg) {
  msg.set_optional_string(st.optional_string.val);
  msg.set_optional_int32(st.optional_int32.val);
  for (const auto& value : st.repeated_sint32.val) {
    msg.add_repeated_sint32(value.val);
  }

  for (const auto& value : st.repeated_int64) {
    msg.add_repeated_int64(value);
  }
}

void CheckNestedMsg(const stpb2::NestedMsg& st, const pb2::NestedMsg& msg) {
  if (msg.has_msg()) {
    CheckSimpleMsg(st.msg.val, msg.msg());
  }

  CHECK(st.repeated_simple_msg.size() == msg.repeated_simple_msg().size());
  for (size_t i = 0; i < st.repeated_simple_msg.size(); ++i) {
    CheckSimpleMsg(st.repeated_simple_msg[i], msg.repeated_simple_msg()[i]);
  }
}

void SetNestedMsg(const stpb2::NestedMsg& st, pb2::NestedMsg& msg) {
  SetSimpleMsg(st.msg.val, *msg.mutable_msg());
  for (const auto& simple_msg : st.repeated_simple_msg) {
    SetSimpleMsg(simple_msg, *msg.add_repeated_simple_msg());
  }
}

inline void print_hex_str(const std::string& str) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : str) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  std::cout << oss.str() << std::endl;
}