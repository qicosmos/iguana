#include "yaml_benchmark.hpp"

const int iterations = 10000;
void test_deserialize() {
  std::cout << "============ deserialize appveyor.yml  ===============\n";
  std::string filestr = yaml_file_content("../data/appveyor.yml");
  std::vector<char> src(filestr.begin(), filestr.end());
#ifdef DEBUG
  std::cout << "debug mode to validator the result\n\n";
#endif
  {
    ScopedTimer timer("test deserialize appveyor.yml");
    for (int i = 0; i < iterations; ++i) {
      appveyor_t app;
      iguana::from_yaml(app, src.begin(), src.end());
#ifdef DEBUG
      validator(app);
#endif
    }
  }
}

int main() { test_deserialize(); }