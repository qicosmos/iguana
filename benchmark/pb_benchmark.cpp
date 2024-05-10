#include <google/protobuf/arena.h>

#include "../test/proto/unittest_proto3.h"

class ScopedTimer {
 public:
  ScopedTimer(const char *name)
      : m_name(name), m_beg(std::chrono::high_resolution_clock::now()) {}
  ScopedTimer(const char *name, uint64_t &ns) : ScopedTimer(name) {
    m_ns = &ns;
  }
  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_beg);
    if (m_ns)
      *m_ns = dur.count();
    else
      std::cout << std::left << std::setw(45) << m_name << " : " << std::right
                << std::setw(12) << dur.count() << " ns\n";
  }

 private:
  const char *m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
  uint64_t *m_ns = nullptr;
};

void bench(int Count) {
  stpb::BaseTypeMsg base_type_st{std::numeric_limits<int32_t>::max(),
                                 std::numeric_limits<int64_t>::max(),
                                 std::numeric_limits<uint32_t>::max(),
                                 std::numeric_limits<uint64_t>::max(),
                                 std::numeric_limits<float>::max(),
                                 std::numeric_limits<double>::max(),
                                 true,
                                 std::string(1, 'x'),
                                 stpb::Enum::BAZ};
  pb::BaseTypeMsg base_type_msg;
  SetBaseTypeMsg(base_type_st, base_type_msg);

  stpb::IguanaTypeMsg iguana_type_st{{std::numeric_limits<int32_t>::max()},
                                     {std::numeric_limits<int64_t>::max()},
                                     {std::numeric_limits<uint32_t>::max()},
                                     {std::numeric_limits<uint64_t>::max()},
                                     {std::numeric_limits<int32_t>::max()},
                                     {std::numeric_limits<int64_t>::max()}};
  pb::IguanaTypeMsg iguana_type_msg{};
  SetIguanaTypeMsg(iguana_type_st, iguana_type_msg);

  stpb::RepeatBaseTypeMsg re_base_type_st{
      {std::numeric_limits<uint32_t>::max(),
       std::numeric_limits<uint32_t>::min()},
      {std::numeric_limits<uint64_t>::max(),
       std::numeric_limits<uint64_t>::min()},
      {std::numeric_limits<int32_t>::max(),
       std::numeric_limits<int32_t>::min()},
      {std::numeric_limits<int64_t>::max(),
       std::numeric_limits<int64_t>::min()},
      {},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::min()},
      {std::string(1, 'x'), std::string(1, 'x'), std::string(1, 'x')},
      {stpb::Enum::NEG, stpb::Enum::FOO}};
  pb::RepeatBaseTypeMsg re_base_type_msg;
  SetRepeatBaseTypeMsg(re_base_type_st, re_base_type_msg);

  stpb::RepeatIguanaTypeMsg re_iguana_type_st{
      {{0}, {1}, {3}},    {{4}, {5}, {6}},    {{7}, {8}, {9}},
      {{10}, {11}, {12}}, {{13}, {14}, {15}}, {},
  };
  pb::RepeatIguanaTypeMsg re_iguana_type_msg;
  SetRepeatIguanaTypeMsg(re_iguana_type_st, re_iguana_type_msg);

  stpb::NestedMsg nest_st{
      /* base_msg */ {100, 200, 300, 400, 31.4f, 62.8, false, "World",
                      stpb::Enum::BAZ},
      /* repeat_base_msg */
      {{1, 2, 3, 4, 5.5f, 6.6, true, "Hello", stpb::Enum::FOO},
       {7, 8, 9, 10, 11.11f, 12.12, false, "Hi", stpb::Enum::BAR}},
      /* iguana_type_msg */ {{100}, {200}, {300}, {400}, {31}, {32}},
      /* repeat_iguna_msg */
      {{{1}, {2}, {3}}, {{4}, {5}, {6}}, {{7}, {8}, {9}}},
      /* repeat_repeat_base_msg */
      {{{1, 2, 3},
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
  pb::NestedMsg nest_msg;
  SetNestedMsg(nest_st, nest_msg);

  stpb::MapMsg map_st{};
  map_st.sfix64_str_map.emplace(iguana::sfixed64_t{10}, "ten");
  map_st.sfix64_str_map.emplace(iguana::sfixed64_t{20}, "twenty");

  map_st.str_iguana_type_msg_map.emplace(
      "first", stpb::IguanaTypeMsg{{10}, {20}, {30}, {40}, {50}, {60}});
  map_st.str_iguana_type_msg_map.emplace(
      "second", stpb::IguanaTypeMsg{{11}, {21}, {31}, {41}, {51}, {61}});

  map_st.int_repeat_base_msg_map.emplace(
      1, stpb::RepeatBaseTypeMsg{{1, 2},
                                 {3, 4},
                                 {5, 6},
                                 {7, 8},
                                 {9.0f, 10.0f},
                                 {11.0, 12.0},
                                 {"one", "two"},
                                 {stpb::Enum::FOO, stpb::Enum::BAR}});
  map_st.int_repeat_base_msg_map.emplace(
      2, stpb::RepeatBaseTypeMsg{{2, 3},
                                 {4, 5},
                                 {6, 7},
                                 {8, 9},
                                 {10.0f, 11.0f},
                                 {12.0, 13.0},
                                 {"three", "four"},
                                 {stpb::Enum::BAZ, stpb::Enum::NEG}});
  pb::MapMsg map_msg{};
  SetMapMsg(map_st, map_msg);

  stpb::BaseTypeMsg baseTypeMsg{
      100, 200, 300, 400, 31.4f, 62.8, false, "World", stpb::Enum::BAZ};
  stpb::BaseOneofMsg base_one_of_st{123, baseTypeMsg, 456.78};
  pb::BaseOneofMsg base_one_of_msg;
  SetBaseOneofMsg(base_one_of_st, base_one_of_msg);

  {
    ScopedTimer timer("protobuf serialize");
    for (int i = 0; i < Count; ++i) {
      std::string str_base_msg_ss;
      str_base_msg_ss.reserve(base_type_msg.ByteSizeLong());
      base_type_msg.SerializeToString(&str_base_msg_ss);

      std::string str_iguana_msg_ss;
      str_iguana_msg_ss.reserve(iguana_type_msg.ByteSizeLong());
      iguana_type_msg.SerializeToString(&str_iguana_msg_ss);

      std::string re_base_type_msg_ss;
      re_base_type_msg_ss.reserve(re_base_type_msg.ByteSizeLong());
      re_base_type_msg.SerializeToString(&re_base_type_msg_ss);

      std::string re_iguana_type_msg_ss;
      re_iguana_type_msg_ss.reserve(re_iguana_type_msg.ByteSizeLong());
      re_iguana_type_msg.SerializeToString(&re_iguana_type_msg_ss);

      std::string nest_msg_ss;
      nest_msg.SerializeToString(&nest_msg_ss);

      std::string map_msg_ss;
      map_msg.SerializeToString(&map_msg_ss);

      std::string base_one_of_msg_ss;
      base_one_of_msg.SerializeToString(&base_one_of_msg_ss);
    }
  }

  {
    ScopedTimer timer("struct_pb serialize");
    for (int i = 0; i < Count; ++i) {
      std::string str_base_st_ss;
      iguana::to_pb(base_type_st, str_base_st_ss);

      std::string str_iguana_st_ss;
      iguana::to_pb(iguana_type_st, str_iguana_st_ss);

      std::string re_base_type_st_ss;
      iguana::to_pb(re_base_type_st, re_base_type_st_ss);

      std::string re_iguana_type_st_ss;
      iguana::to_pb(re_iguana_type_st, re_iguana_type_st_ss);

      std::string nest_st_ss;
      iguana::to_pb(nest_st, nest_st_ss);

      std::string map_st_ss;
      iguana::to_pb(map_st, map_st_ss);

      std::string base_one_of_st_ss;
      iguana::to_pb(base_one_of_st, base_one_of_st_ss);
    }
  }

  std::string str_base_st_ss;
  iguana::to_pb(base_type_st, str_base_st_ss);

  std::string str_iguana_st_ss;
  iguana::to_pb(iguana_type_st, str_iguana_st_ss);

  std::string re_base_type_st_ss;
  iguana::to_pb(re_base_type_st, re_base_type_st_ss);

  std::string re_iguana_type_st_ss;
  iguana::to_pb(re_iguana_type_st, re_iguana_type_st_ss);

  std::string nest_st_ss;
  iguana::to_pb(nest_st, nest_st_ss);

  std::string map_st_ss;
  iguana::to_pb(map_st, map_st_ss);

  std::string base_one_of_st_ss;
  iguana::to_pb(base_one_of_st, base_one_of_st_ss);

  std::string str_base_msg_ss;
  base_type_msg.SerializeToString(&str_base_msg_ss);

  std::string str_iguana_msg_ss;
  iguana_type_msg.SerializeToString(&str_iguana_msg_ss);

  std::string re_base_type_msg_ss;
  re_base_type_msg.SerializeToString(&re_base_type_msg_ss);

  std::string re_iguana_type_msg_ss;
  re_iguana_type_msg.SerializeToString(&re_iguana_type_msg_ss);

  std::string nest_msg_ss;
  nest_msg.SerializeToString(&nest_msg_ss);

  std::string map_msg_ss;
  map_msg.SerializeToString(&map_msg_ss);

  std::string base_one_of_msg_ss;
  base_one_of_msg.SerializeToString(&base_one_of_msg_ss);

  {
    ScopedTimer timer("struct_pb deserialize");
    for (int i = 0; i < Count; ++i) {
      stpb::BaseTypeMsg base_type_st_de;
      iguana::from_pb(base_type_st_de, str_base_st_ss);

      stpb::IguanaTypeMsg str_iguana_st_de;
      iguana::from_pb(str_iguana_st_de, str_iguana_st_ss);

      stpb::RepeatBaseTypeMsg re_base_type_st_de;
      iguana::from_pb(re_base_type_st_de, re_base_type_st_ss);

      stpb::RepeatIguanaTypeMsg re_iguana_type_st_de;
      iguana::from_pb(re_iguana_type_st_de, re_iguana_type_st_ss);

      stpb::NestedMsg nest_st_de;
      iguana::from_pb(nest_st_de, nest_st_ss);

      stpb::MapMsg map_st_de;
      iguana::from_pb(map_st_de, map_st_ss);

      stpb::BaseOneofMsg base_one_of_st_de;
      iguana::from_pb(base_one_of_st_de, base_one_of_st_ss);
    }
  }

  {
    ScopedTimer timer("protobuf deserialize");
    for (int i = 0; i < Count; ++i) {
      pb::BaseTypeMsg base_type_msg_de;
      base_type_msg_de.ParseFromString(str_base_msg_ss);

      pb::IguanaTypeMsg iguana_type_msg_de;
      iguana_type_msg_de.ParseFromString(str_iguana_msg_ss);

      pb::RepeatBaseTypeMsg re_base_type_msg_de;
      re_base_type_msg_de.ParseFromString(re_base_type_msg_ss);

      pb::RepeatIguanaTypeMsg re_iguana_type_msg_de;
      re_iguana_type_msg_de.ParseFromString(re_iguana_type_msg_ss);

      pb::NestedMsg nest_msg_de;
      nest_msg_de.ParseFromString(nest_msg_ss);

      pb::MapMsg map_msg_de;
      map_msg_de.ParseFromString(map_msg_ss);

      pb::BaseOneofMsg base_one_of_msg_de;
      base_one_of_msg_de.ParseFromString(base_one_of_msg_ss);
    }
  }
}

int main() { bench(10000); }
