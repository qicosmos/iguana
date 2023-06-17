#include "yaml_benchmark.hpp"

const int iterations = 10000;
void test_deserialize() {
  std::cout << "============ deserialize appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  std::vector<char> src(filestr.begin(), filestr.end());
  {
    ScopedTimer timer("test deserialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      appveyor_t app;
      iguana::from_yaml(app, src.begin(), src.end());
    }
  }
  std::cout << "============ deserialize travis.yml  ===============\n";
  std::string travisstr = yaml_file_content("../data/travis.yml");
  {
    ScopedTimer timer("test deserialize travis.yml");
    for (int i = 0; i < iterations; ++i) {
      travis_t tra;
      iguana::from_yaml(tra, travisstr.begin(), travisstr.end());
    }
  }
}

int main() { test_deserialize(); }