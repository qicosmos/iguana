#include "yaml_benchmark.hpp"
#ifdef HAS_RAPIDYAML
#define RYML_SINGLE_HDR_DEFINE_NOW
#include "rapidyaml/ryml_all.hpp"
#endif

const int iterations = 100000;
void test_deserialize() {
  std::cout
      << "============ deserialize iguana appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  //    ryml::Tree tree = ryml::parse_in_place(filestr.data());
  std::vector<char> src(filestr.begin(), filestr.end());
  appveyor_t app;
  {
    ScopedTimer timer("test deserialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_yaml(app, src.begin(), src.end());
    }
  }
  std::cout << "============ deserialize iguana travis.yml  ===============\n";
  std::string travisstr = yaml_file_content("../data/travis.yml");
  travis_t tra;
  {
    ScopedTimer timer("test deserialize travis.yml");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_yaml(tra, travisstr.begin(), travisstr.end());
    }
  }
}

#ifdef HAS_RAPIDYAML
void test_deserialize_ryml() {
  std::cout << "============ deserialize ryml appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  ryml::Tree tree1;
  {
    ScopedTimer timer("test deserialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      ryml::parse_in_place(filestr.data());
    }
  }
  std::cout << "============ deserialize ryml travis.yml  ===============\n";
  std::string travisstr = yaml_file_content("../data/travis.yml");
  ryml::Tree tree2;
  {
    ScopedTimer timer("test deserialize travis.yml");
    for (int i = 0; i < iterations; ++i) {
      ryml::parse_in_place(travisstr.data());
    }
  }
}
#endif

void test_serialize() {
  std::cout << "============ serialize iguana appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  appveyor_t app;
  iguana::from_yaml(app, filestr);

  std::string str;
  iguana::to_yaml(app, str);

  {
    ScopedTimer timer("test serialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      str.clear();
      iguana::to_yaml(app, str);
    }
  }
  std::cout << "============ serialize iguana travis.yml  ===============\n";
  std::string travisstr = yaml_file_content("../data/travis.yml");
  travis_t tra;
  iguana::from_yaml(tra, travisstr);
  iguana::to_yaml(tra, str);

  {
    ScopedTimer timer("test serialize travis.yml");
    for (int i = 0; i < iterations; ++i) {
      str.clear();
      iguana::to_yaml(tra, str);
    }
  }
}

#ifdef HAS_RAPIDYAML
void test_serialize_ryml() {
  std::cout << "============ serialize ryml appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  auto tree1 = ryml::parse_in_place(filestr.data());
  std::string str = ryml::emitrs_yaml<std::string>(tree1);

  {
    ScopedTimer timer("test serialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      str.clear();
      str = ryml::emitrs_yaml<std::string>(tree1);
    }
  }
  std::cout << "============ serialize ryml travis.yml  ===============\n";
  std::string travisstr = yaml_file_content("../data/travis.yml");
  auto tree2 = ryml::parse_in_place(travisstr.data());
  str = ryml::emitrs_yaml<std::string>(tree2);

  {
    ScopedTimer timer("test serialize travis.yml");
    for (int i = 0; i < iterations; ++i) {
      str.clear();
      str = ryml::emitrs_yaml<std::string>(tree2);
    }
  }
}
#endif

int main() {
  test_deserialize();
#ifdef HAS_RAPIDYAML
  test_deserialize_ryml();
#endif
  test_serialize();
#ifdef HAS_RAPIDYAML
  test_serialize_ryml();
#endif
}