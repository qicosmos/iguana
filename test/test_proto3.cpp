#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "proto/unittest_proto3.h"  // msg reflection

TEST_CASE("test BaseTypeMsg") {
  {  // normal test
    stpb::BaseTypeMsg se_st{0,     100,  200,   300,     400,
                            31.4f, 62.8, false, "World", stpb::Enum::ZERO};
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
    stpb::BaseTypeMsg se_st{0,
                            std::numeric_limits<int32_t>::min(),
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
    stpb::BaseTypeMsg se_st{0,
                            std::numeric_limits<int32_t>::max(),
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
  stpb::simple_t2 t{0, -100, 2, stpb::Color::Blue, 4};
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
    stpb::IguanaTypeMsg se_st{0, {100}, {200}, {300}, {400}, {31}, {32}};
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
    stpb::IguanaTypeMsg se_st{0,
                              {std::numeric_limits<int32_t>::min()},
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
    stpb::IguanaTypeMsg se_st{0,
                              {std::numeric_limits<int32_t>::max()},
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
        0,
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
    stpb::RepeatBaseTypeMsg se_st{0,
                                  {std::numeric_limits<uint32_t>::max(),
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
        0,
        {{0}, {1}, {3}},
        {{4}, {5}, {6}},
        {{7}, {8}, {9}},
        {{10}, {11}, {12}},
        {{13}, {14}, {15}},
        {},
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
  // {
  //   stpb::NestedMsg se_st{
  //       /* base_msg */ {100, 200, 300, 400, 31.4f, 62.8, false, "World",
  //                       stpb::Enum::BAZ},
  //       /* repeat_base_msg */
  //       {{1, 2, 3, 4, 5.5f, 6.6, true, "Hello", stpb::Enum::FOO},
  //        {7, 8, 9, 10, 11.11f, 12.12, false, "Hi", stpb::Enum::BAR}},
  //       /* iguana_type_msg */ {{100}, {200}, {300}, {400}, {31}, {32}},
  //       /* repeat_iguna_msg */
  //       {{{1}, {2}, {3}}, {{4}, {5}, {6}}, {{7}, {8}, {9}}},
  //       /* repeat_repeat_base_msg */
  //       {{{1, 2, 3},
  //         {4, 5, 6},
  //         {7, 8, 9},
  //         {10, 11, 12},
  //         {13.1, 14.2, 15.3},
  //         {16.4, 17.5, 18.6},
  //         {"a", "b", "c"},
  //         {stpb::Enum::FOO, stpb::Enum::BAR, stpb::Enum::BAZ}},
  //        {{19, 20, 21},
  //         {22, 23, 24},
  //         {25, 26, 27},
  //         {28, 29, 30},
  //         {31.1, 32.2, 33.3},
  //         {34.4, 35.5, 36.6},
  //         {"x", "y", "z"},
  //         {stpb::Enum::ZERO, stpb::Enum::NEG, stpb::Enum::FOO}}}};

  //   std::string st_ss;
  //   iguana::to_pb(se_st, st_ss);

  //   pb::NestedMsg se_msg;
  //   SetNestedMsg(se_st, se_msg);

  //   std::string pb_ss;
  //   se_msg.SerializeToString(&pb_ss);

  //   CHECK(st_ss == pb_ss);

  //   stpb::NestedMsg dese_st{};
  //   iguana::from_pb(dese_st, st_ss);

  //   pb::NestedMsg dese_msg;
  //   dese_msg.ParseFromString(pb_ss);

  //   CheckNestedMsg(dese_st, dese_msg);
  // }
  {  // test empty values
    stpb::NestedMsg se_st{
        0,
        /* base_msg */ {0, 0, 0, 0, 0, 0.0f, 0.0, true, "", stpb::Enum::ZERO},
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
  // {
  //   stpb::MapMsg se_st{};

  //   se_st.sfix64_str_map.emplace(iguana::sfixed64_t{10}, "ten");
  //   se_st.sfix64_str_map.emplace(iguana::sfixed64_t{20}, "twenty");

  //   se_st.str_iguana_type_msg_map.emplace(
  //       "first", stpb::IguanaTypeMsg{{10}, {20}, {30}, {40}, {50}, {60}});
  //   se_st.str_iguana_type_msg_map.emplace(
  //       "second", stpb::IguanaTypeMsg{{11}, {21}, {31}, {41}, {51}, {61}});

  //   se_st.int_repeat_base_msg_map.emplace(
  //       1, stpb::RepeatBaseTypeMsg{{1, 2},
  //                                  {3, 4},
  //                                  {5, 6},
  //                                  {7, 8},
  //                                  {9.0f, 10.0f},
  //                                  {11.0, 12.0},
  //                                  {"one", "two"},
  //                                  {stpb::Enum::FOO, stpb::Enum::BAR}});
  //   se_st.int_repeat_base_msg_map.emplace(
  //       2, stpb::RepeatBaseTypeMsg{{2, 3},
  //                                  {4, 5},
  //                                  {6, 7},
  //                                  {8, 9},
  //                                  {10.0f, 11.0f},
  //                                  {12.0, 13.0},
  //                                  {"three", "four"},
  //                                  {stpb::Enum::BAZ, stpb::Enum::NEG}});

  //   std::string st_ss;
  //   iguana::to_pb(se_st, st_ss);

  //   pb::MapMsg se_msg{};
  //   SetMapMsg(se_st, se_msg);
  //   std::string pb_ss;
  //   se_msg.SerializeToString(&pb_ss);
  //   // It's okay not to satisfy this.
  //   // CHECK(st_ss == pb_ss);
  //   CHECK(st_ss.size() == pb_ss.size());
  //   stpb::MapMsg dese_st{};
  //   iguana::from_pb(dese_st, pb_ss);
  //   pb::MapMsg dese_msg;
  //   dese_msg.ParseFromString(st_ss);
  //   CheckMapMsg(dese_st, dese_msg);
  // }
  {
    // key empty
    stpb::MapMsg se_st{};
    se_st.sfix64_str_map.emplace(iguana::sfixed64_t{30}, "");
    se_st.str_iguana_type_msg_map.emplace(
        "", stpb::IguanaTypeMsg{0, {0}, {0}, {0}, {0}, {0}, {0}});
    se_st.int_repeat_base_msg_map.emplace(
        3, stpb::RepeatBaseTypeMsg{0, {}, {}, {}, {}, {}, {}, {}, {}});
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
    stpb::BaseOneofMsg se_st{0, 123, 3.14159, 456.78};
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
    stpb::BaseOneofMsg se_st{0, 123, std::string("Hello"), 456.78};
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
    stpb::BaseTypeMsg baseTypeMsg{0,     100,  200,   300,     400,
                                  31.4f, 62.8, false, "World", stpb::Enum::BAZ};
    stpb::BaseOneofMsg se_st{0, 123, baseTypeMsg, 456.78};

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
    stpb::BaseOneofMsg se_st{0, 123, {}, 456.78};

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
    stpb::BaseOneofMsg baseOneof{0, 123, std::string("Hello"), 456.78};
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

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
