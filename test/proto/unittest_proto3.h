#pragma once
#include "iguana/pb_reader.hpp"
#include "iguana/pb_writer.hpp"
#include "unittest_proto3.pb.h"  // protoc gen

#ifdef CHECK
#define PB_CHECK CHECK
#define PUBLIC
#else
#include <cassert>
#define PB_CHECK assert
#define PUBLIC : iguana::pb_base
#endif

// define the struct as msg in proto
namespace stpb {
enum class Enum {
  ZERO = 0,
  FOO = 1,
  BAR = 2,
  BAZ = 123456,
  NEG = -1,  // Intentionally negative.
};

struct BaseTypeMsg PUBLIC {
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

struct IguanaTypeMsg PUBLIC {
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

struct RepeatBaseTypeMsg PUBLIC {
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

struct RepeatIguanaTypeMsg PUBLIC {
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

struct NestedMsg PUBLIC {
  BaseTypeMsg base_msg;
  std::vector<BaseTypeMsg> repeat_base_msg;
  IguanaTypeMsg iguana_type_msg;
  std::vector<IguanaTypeMsg> repeat_iguna_msg;
  std::vector<RepeatBaseTypeMsg> repeat_repeat_base_msg;
};
REFLECTION(NestedMsg, base_msg, repeat_base_msg, iguana_type_msg,
           repeat_iguna_msg, repeat_repeat_base_msg);

struct MapMsg PUBLIC {
  std::unordered_map<iguana::sfixed64_t, std::string> sfix64_str_map{};
  std::unordered_map<std::string, IguanaTypeMsg> str_iguana_type_msg_map{};
  std::map<int, RepeatBaseTypeMsg> int_repeat_base_msg_map{};
};
REFLECTION(MapMsg, sfix64_str_map, str_iguana_type_msg_map,
           int_repeat_base_msg_map);

struct BaseOneofMsg PUBLIC {
  int32_t optional_int32;
  std::variant<double, std::string, BaseTypeMsg> one_of;
  double optional_double;
};
REFLECTION(BaseOneofMsg, optional_int32, one_of, optional_double);

struct NestOneofMsg PUBLIC {
  std::variant<std::string, BaseOneofMsg> nest_one_of_msg;
};
REFLECTION(NestOneofMsg, nest_one_of_msg);

struct simple_t {
  int32_t a;
  int32_t b;
  int64_t c;
  int64_t d;
  std::string_view str;
};
REFLECTION(simple_t, a, b, c, d, str);

struct simple_t1 {
  int32_t a;
  int32_t b;
  int64_t c;
  int64_t d;
  std::string_view str;
};
REFLECTION(simple_t1, a, b, c, d, str);
}  // namespace stpb

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
  PB_CHECK(st.optional_int32 == msg.optional_int32());
  PB_CHECK(st.optional_int64 == msg.optional_int64());
  PB_CHECK(st.optional_uint32 == msg.optional_uint32());
  PB_CHECK(st.optional_uint64 == msg.optional_uint64());
  PB_CHECK(st.optional_float == msg.optional_float());
  PB_CHECK(st.optional_double == msg.optional_double());
  PB_CHECK(st.optional_bool == msg.optional_bool());
  PB_CHECK(st.optional_string == msg.optional_string());
  PB_CHECK(static_cast<int32_t>(st.optional_enum) ==
           static_cast<int32_t>(msg.optional_enum()));
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
  PB_CHECK(st.optional_sint32.val == msg.optional_sint32());
  PB_CHECK(st.optional_sint64.val == msg.optional_sint64());
  PB_CHECK(st.optional_fixed32.val == msg.optional_fixed32());
  PB_CHECK(st.optional_fixed64.val == msg.optional_fixed64());
  PB_CHECK(st.optional_sfixed32.val == msg.optional_sfixed32());
  PB_CHECK(st.optional_sfixed64.val == msg.optional_sfixed64());
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
  for (size_t i = 0; i < st.repeated_uint32.size(); ++i) {
    PB_CHECK(st.repeated_uint32[i] == msg.repeated_uint32(i));
  }
  for (size_t i = 0; i < st.repeated_uint64.size(); ++i) {
    PB_CHECK(st.repeated_uint64[i] == msg.repeated_uint64(i));
  }
  for (size_t i = 0; i < st.repeated_int32.size(); ++i) {
    PB_CHECK(st.repeated_int32[i] == msg.repeated_int32(i));
  }
  for (size_t i = 0; i < st.repeated_int64.size(); ++i) {
    PB_CHECK(st.repeated_int64[i] == msg.repeated_int64(i));
  }
  for (size_t i = 0; i < st.repeated_float.size(); ++i) {
    PB_CHECK(st.repeated_float[i] == msg.repeated_float(i));
  }
  for (size_t i = 0; i < st.repeated_double.size(); ++i) {
    PB_CHECK(st.repeated_double[i] == msg.repeated_double(i));
  }
  for (size_t i = 0; i < st.repeated_string.size(); ++i) {
    PB_CHECK(st.repeated_string[i] == msg.repeated_string(i));
  }
  for (size_t i = 0; i < st.repeated_enum.size(); ++i) {
    PB_CHECK(static_cast<int>(st.repeated_enum[i]) ==
             static_cast<int>(msg.repeated_enum(i)));
  }
}

void SetRepeatIguanaTypeMsg(const stpb::RepeatIguanaTypeMsg& st,
                            pb::RepeatIguanaTypeMsg& msg) {
  for (auto v : st.repeated_sint32) {
    msg.add_repeated_sint32(v.val);
  }
  for (auto v : st.repeated_sint64) {
    msg.add_repeated_sint64(v.val);
  }
  for (auto v : st.repeated_fixed32) {
    msg.add_repeated_fixed32(v.val);
  }
  for (auto v : st.repeated_fixed64) {
    msg.add_repeated_fixed64(v.val);
  }
  for (auto v : st.repeated_sfixed32) {
    msg.add_repeated_sfixed32(v.val);
  }
  for (auto v : st.repeated_sfixed64) {
    msg.add_repeated_sfixed64(v.val);
  }
}

void CheckRepeatIguanaTypeMsg(const stpb::RepeatIguanaTypeMsg& st,
                              const pb::RepeatIguanaTypeMsg& msg) {
  for (size_t i = 0; i < st.repeated_sint32.size(); ++i) {
    PB_CHECK(st.repeated_sint32[i].val == msg.repeated_sint32(i));
  }
  for (size_t i = 0; i < st.repeated_sint64.size(); ++i) {
    PB_CHECK(st.repeated_sint64[i].val == msg.repeated_sint64(i));
  }
  for (size_t i = 0; i < st.repeated_fixed32.size(); ++i) {
    PB_CHECK(st.repeated_fixed32[i].val == msg.repeated_fixed32(i));
  }
  for (size_t i = 0; i < st.repeated_fixed64.size(); ++i) {
    PB_CHECK(st.repeated_fixed64[i].val == msg.repeated_fixed64(i));
  }
  for (size_t i = 0; i < st.repeated_sfixed32.size(); ++i) {
    PB_CHECK(st.repeated_sfixed32[i].val == msg.repeated_sfixed32(i));
  }
  for (size_t i = 0; i < st.repeated_sfixed64.size(); ++i) {
    PB_CHECK(st.repeated_sfixed64[i].val == msg.repeated_sfixed64(i));
  }
}

void SetNestedMsg(const stpb::NestedMsg& st, pb::NestedMsg& msg) {
  SetBaseTypeMsg(st.base_msg, *msg.mutable_base_msg());

  for (const auto& base_msg : st.repeat_base_msg) {
    auto* base_msg_ptr = msg.add_repeat_base_msg();
    SetBaseTypeMsg(base_msg, *base_msg_ptr);
  }

  SetIguanaTypeMsg(st.iguana_type_msg, *msg.mutable_iguana_type_msg());

  for (const auto& iguana_type_msg : st.repeat_iguna_msg) {
    auto* iguana_type_msg_ptr = msg.add_repeat_iguna_msg();
    SetIguanaTypeMsg(iguana_type_msg, *iguana_type_msg_ptr);
  }

  for (const auto& repeat_base_msg : st.repeat_repeat_base_msg) {
    auto* repeat_base_msg_ptr = msg.add_repeat_repeat_base_msg();
    SetRepeatBaseTypeMsg(repeat_base_msg, *repeat_base_msg_ptr);
  }
}

void CheckNestedMsg(const stpb::NestedMsg& st, const pb::NestedMsg& msg) {
  CheckBaseTypeMsg(st.base_msg, msg.base_msg());

  PB_CHECK(st.repeat_base_msg.size() == msg.repeat_base_msg_size());
  for (size_t i = 0; i < st.repeat_base_msg.size(); ++i) {
    CheckBaseTypeMsg(st.repeat_base_msg[i], msg.repeat_base_msg(i));
  }

  CheckIguanaTypeMsg(st.iguana_type_msg, msg.iguana_type_msg());

  PB_CHECK(st.repeat_iguna_msg.size() == msg.repeat_iguna_msg_size());
  for (size_t i = 0; i < st.repeat_iguna_msg.size(); ++i) {
    CheckIguanaTypeMsg(st.repeat_iguna_msg[i], msg.repeat_iguna_msg(i));
  }

  PB_CHECK(st.repeat_repeat_base_msg.size() ==
           msg.repeat_repeat_base_msg_size());
  for (size_t i = 0; i < st.repeat_repeat_base_msg.size(); ++i) {
    CheckRepeatBaseTypeMsg(st.repeat_repeat_base_msg[i],
                           msg.repeat_repeat_base_msg(i));
  }
}

void SetMapMsg(const stpb::MapMsg& st, pb::MapMsg& msg) {
  msg.Clear();
  for (const auto& pair : st.sfix64_str_map) {
    (*msg.mutable_sfix64_str_map())[pair.first.val] = pair.second;
  }
  for (const auto& pair : st.str_iguana_type_msg_map) {
    pb::IguanaTypeMsg* it_msg =
        &((*msg.mutable_str_iguana_type_msg_map())[pair.first]);
    SetIguanaTypeMsg(pair.second, *it_msg);
  }
  for (const auto& pair : st.int_repeat_base_msg_map) {
    pb::RepeatBaseTypeMsg* rb_msg =
        &((*msg.mutable_int_repeat_base_msg_map())[pair.first]);
    SetRepeatBaseTypeMsg(pair.second, *rb_msg);
  }
}

void CheckMapMsg(const stpb::MapMsg& st, const pb::MapMsg& msg) {
  PB_CHECK(msg.sfix64_str_map_size() == st.sfix64_str_map.size());
  for (const auto& pair : st.sfix64_str_map) {
    auto it = msg.sfix64_str_map().find(pair.first.val);
    PB_CHECK(it != msg.sfix64_str_map().end());
    PB_CHECK(it->second == pair.second);
  }
  PB_CHECK(msg.str_iguana_type_msg_map_size() ==
           st.str_iguana_type_msg_map.size());
  for (const auto& pair : st.str_iguana_type_msg_map) {
    auto it = msg.str_iguana_type_msg_map().find(pair.first);
    PB_CHECK(it != msg.str_iguana_type_msg_map().end());
    CheckIguanaTypeMsg(pair.second, it->second);
  }

  PB_CHECK(msg.int_repeat_base_msg_map_size() ==
           st.int_repeat_base_msg_map.size());
  for (const auto& pair : st.int_repeat_base_msg_map) {
    auto it = msg.int_repeat_base_msg_map().find(pair.first);
    PB_CHECK(it != msg.int_repeat_base_msg_map().end());
    CheckRepeatBaseTypeMsg(pair.second, it->second);
  }
}

void SetBaseOneofMsg(const stpb::BaseOneofMsg& st, pb::BaseOneofMsg& msg) {
  msg.set_optional_int32(st.optional_int32);
  msg.set_optional_double(st.optional_double);

  std::visit(
      [&](auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, double>) {
          msg.set_one_of_double(value);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
          msg.set_one_of_string(value);
        }
        else if constexpr (std::is_same_v<T, stpb::BaseTypeMsg>) {
          auto* submsg = msg.mutable_one_of_base_type_msg();
          SetBaseTypeMsg(value, *submsg);
        }
      },
      st.one_of);
}

void CheckBaseOneofMsg(const stpb::BaseOneofMsg& st,
                       const pb::BaseOneofMsg& msg) {
  PB_CHECK(st.optional_int32 == msg.optional_int32());
  PB_CHECK(st.optional_double == msg.optional_double());

  std::visit(
      [&](auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, double>) {
          PB_CHECK(value == msg.one_of_double());
        }
        else if constexpr (std::is_same_v<T, std::string>) {
          PB_CHECK(value == msg.one_of_string());
        }
        else if constexpr (std::is_same_v<T, stpb::BaseTypeMsg>) {
          CheckBaseTypeMsg(value, msg.one_of_base_type_msg());
        }
      },
      st.one_of);
}

void SetNestOneofMsg(const stpb::NestOneofMsg& st, pb::NestOneofMsg& msg) {
  std::visit(
      [&](auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
          msg.set_base_one_of_string(value);
        }
        else if constexpr (std::is_same_v<T, stpb::BaseOneofMsg>) {
          auto* submsg = msg.mutable_base_one_of_msg();
          SetBaseOneofMsg(value, *submsg);
        }
      },
      st.nest_one_of_msg);
}

void CheckNestOneofMsg(const stpb::NestOneofMsg& st,
                       const pb::NestOneofMsg& msg) {
  std::visit(
      [&](auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
          PB_CHECK(value == msg.base_one_of_string());
        }
        else if constexpr (std::is_same_v<T, stpb::BaseOneofMsg>) {
          CheckBaseOneofMsg(value, msg.base_one_of_msg());
        }
      },
      st.nest_one_of_msg);
}

inline void print_hex_str(const std::string& str) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : str) {
    oss << std::setw(2) << static_cast<int>(c);
  }
  std::cout << oss.str() << std::endl;
}
