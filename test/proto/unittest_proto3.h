#pragma once
#include "iguana/pb_reader.hpp"
#include "iguana/pb_writer.hpp"

// define the struct as msg in proto
namespace stpb {
enum class Enum {
  ZERO = 0,
  FOO = 1,
  BAR = 2,
  BAZ = 123456,
  NEG = -1,  // Intentionally negative.
};

struct BaseTypeMsg {
  int32_t optional_int32;
  int64_t optional_int64;
  uint32_t optional_uint32;
  uint64_t optional_uint64;
  float optional_float;
  double optional_double;
  bool optional_bool;
  std::string optional_string;
  Enum optional_enum;
  bool operator==(const BaseTypeMsg& other) const {
    return optional_int32 == other.optional_int32 &&
           optional_int64 == other.optional_int64 &&
           optional_uint32 == other.optional_uint32 &&
           optional_uint64 == other.optional_uint64 &&
           optional_float == other.optional_float &&
           optional_double == other.optional_double &&
           optional_bool == other.optional_bool &&
           optional_string == other.optional_string &&
           optional_enum == other.optional_enum;
  }
};
REFLECTION(BaseTypeMsg, optional_int32, optional_int64, optional_uint32,
           optional_uint64, optional_float, optional_double, optional_bool,
           optional_string, optional_enum);

struct IguanaTypeMsg {
  iguana::sint32_t optional_sint32;
  iguana::sint64_t optional_sint64;
  iguana::fixed32_t optional_fixed32;
  iguana::fixed64_t optional_fixed64;
  iguana::sfixed32_t optional_sfixed32;
  iguana::sfixed64_t optional_sfixed64;

  bool operator==(const IguanaTypeMsg& other) const {
    return optional_sint32 == other.optional_sint32 &&
           optional_sint64 == other.optional_sint64 &&
           optional_fixed32 == other.optional_fixed32 &&
           optional_fixed64 == other.optional_fixed64 &&
           optional_sfixed32 == other.optional_sfixed32 &&
           optional_sfixed64 == other.optional_sfixed64;
  }
};
REFLECTION(IguanaTypeMsg, optional_sint32, optional_sint64, optional_fixed32,
           optional_fixed64, optional_sfixed32, optional_sfixed64);

struct RepeatBaseTypeMsg {
  std::vector<uint32_t> repeated_uint32;
  std::vector<uint64_t> repeated_uint64;
  std::vector<int32_t> repeated_int32;
  std::vector<int64_t> repeated_int64;
  std::vector<float> repeated_float;
  std::vector<double> repeated_double;
  std::vector<std::string> repeated_string;
  std::vector<Enum> repeated_enum;
};

REFLECTION(RepeatBaseTypeMsg, repeated_uint32, repeated_uint64, repeated_int32,
           repeated_int64, repeated_float, repeated_double, repeated_string,
           repeated_enum);

struct RepeatIguanaTypeMsg {
  std::vector<iguana::sfixed32_t> repeated_sint32;
  std::vector<iguana::sfixed64_t> repeated_sint64;
  std::vector<iguana::fixed32_t> repeated_fixed32;
  std::vector<iguana::fixed64_t> repeated_fixed64;
  std::vector<iguana::sfixed32_t> repeated_sfixed32;
  std::vector<iguana::sfixed64_t> repeated_sfixed64;
};

REFLECTION(RepeatIguanaTypeMsg, repeated_sint32, repeated_sint64,
           repeated_fixed32, repeated_fixed64, repeated_sfixed32,
           repeated_sfixed64);

struct NestedMsg {
  BaseTypeMsg base_msg;
  std::vector<BaseTypeMsg> repeat_base_msg;
  IguanaTypeMsg iguana_type_msg;
  std::vector<IguanaTypeMsg> repeat_iguna_msg;
  std::vector<RepeatBaseTypeMsg> repeat_repeat_base_msg;
};
REFLECTION(NestedMsg, base_msg, repeat_base_msg, iguana_type_msg,
           repeat_iguna_msg, repeat_repeat_base_msg);

struct MapMsg {
  std::unordered_map<iguana::sfixed64_t, std::string> sfix64_str_map{};
  std::unordered_map<std::string, IguanaTypeMsg> str_iguana_type_msg_map{};
  std::map<int, RepeatBaseTypeMsg> int_repeat_base_msg_map{};
};
REFLECTION(MapMsg, sfix64_str_map, str_iguana_type_msg_map,
           int_repeat_base_msg_map);

struct BaseOneofMsg {
  int x;
  std::variant<double, std::string> y;
  double z;
};
REFLECTION(BaseOneofMsg, x, y, z);

}  // namespace stpb