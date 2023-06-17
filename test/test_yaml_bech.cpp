#include "doctest.h"
#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <optional>
#include <vector>

std::string yaml_file_content(const std::string &filename) {
  std::error_code ec;
  uint64_t size = std::filesystem::file_size(filename, ec);
  if (ec) {
    throw std::runtime_error("file size error " + ec.message());
  }
  if (size == 0) {
    throw std::runtime_error("empty file");
  }
  std::string content;
  content.resize(size);
  std::ifstream file(filename, std::ios::binary);
  file.read(content.data(), content.size());
  return content;
}

struct matrix_t {
  std::string_view compiler;
  std::string_view generator;
  std::string_view configuration;
};
REFLECTION(matrix_t, compiler, generator, configuration);
struct environment_t {
  std::vector<matrix_t> matrix;
};
REFLECTION(environment_t, matrix);
struct ex_matrix_t {
  std::string_view fast_finish;
};
REFLECTION(ex_matrix_t, fast_finish);
struct artifact_t {
  std::string_view path;
  std::string_view name;
};
REFLECTION(artifact_t, path, name);
struct skip_commit_t {
  std::vector<std::string_view> files;
};
REFLECTION(skip_commit_t, files);
struct appveyor_t {
  std::string_view version;
  std::string_view image;
  environment_t environment;
  ex_matrix_t matrix;
  std::vector<std::string_view> install;
  std::vector<std::string_view> build_script;
  std::vector<std::string_view> test_script;
  std::vector<artifact_t> artifacts;
  skip_commit_t skip_commits;
};
REFLECTION(appveyor_t, version, image, environment, matrix, install,
           build_script, test_script, artifacts, skip_commits);

void validator(appveyor_t app) {
  CHECK(app.version == "{build}");
  CHECK(app.image == "Visual Studio 2017");
  CHECK(app.environment.matrix[0].compiler == "msvc-15-seh");
  CHECK(app.environment.matrix[0].generator == "Visual Studio 15 2017");
  CHECK(app.environment.matrix[0].configuration == "Debug");

  CHECK(app.environment.matrix[1].compiler == "msvc-15-seh");
  CHECK(app.environment.matrix[1].generator == "Visual Studio 15 2017 Win64");
  CHECK(app.environment.matrix[1].configuration == "Debug");

  CHECK(app.environment.matrix[2].compiler == "msvc-15-seh");
  CHECK(app.environment.matrix[2].generator == "Visual Studio 15 2017");
  CHECK(app.environment.matrix[2].configuration == "Release");

  CHECK(app.environment.matrix[3].compiler == "msvc-15-seh");
  CHECK(app.environment.matrix[3].generator == "Visual Studio 15 2017 Win64");
  CHECK(app.environment.matrix[3].configuration == "Release");

  CHECK(app.environment.matrix[4].compiler == "msvc-14-seh");
  CHECK(app.environment.matrix[4].generator == "Visual Studio 14 2015");
  CHECK(app.environment.matrix[4].configuration == "Debug");

  CHECK(app.environment.matrix[5].compiler == "msvc-14-seh");
  CHECK(app.environment.matrix[5].generator == "Visual Studio 14 2015 Win64");
  CHECK(app.environment.matrix[5].configuration == "Debug");

  CHECK(app.environment.matrix[6].compiler == "msvc-14-seh");
  CHECK(app.environment.matrix[6].generator == "Visual Studio 14 2015");
  CHECK(app.environment.matrix[6].configuration == "Release");

  CHECK(app.environment.matrix[7].compiler == "msvc-14-seh");
  CHECK(app.environment.matrix[7].generator == "Visual Studio 14 2015 Win64");
  CHECK(app.environment.matrix[7].configuration == "Release");

  CHECK(app.matrix.fast_finish == "true");

  CHECK(app.install[0] == "git submodule update --init --recursive");
  CHECK(app.install[1] ==
        "if \"%generator%\"==\"MinGW Makefiles\" (set \"PATH=%PATH:C:\\Program "
        "Files\\Git\\usr\\bin;=%\")");
  CHECK(app.install[2] ==
        "if not \"%cxx_path%\"==\"\" (set \"PATH=%PATH%;%cxx_path%\")");

  CHECK(app.build_script[0] == "md _build -Force");
  CHECK(app.build_script[1] == "cd _build");

  CHECK(app.test_script[0] == "echo %configuration%");
  CHECK(app.test_script[1] ==
        "cmake -G \"%generator%\" \"-DCMAKE_BUILD_TYPE=%configuration%\" "
        "-DRYML_DEV=ON ..");
  CHECK(app.test_script[2] == "dir");
  CHECK(app.test_script[3] == "dir test");
  CHECK(app.test_script[4] ==
        "cmake --build . --config %configuration% --target ryml-test");

  CHECK(app.artifacts[0].path == "_build/CMakeFiles/*.log");
  CHECK(app.artifacts[0].name == "logs");
  CHECK(app.artifacts[1].path == "_build/Testing/**/*.xml");
  CHECK(app.artifacts[1].name == "test_results");

  CHECK(app.skip_commits.files[0] == ".gitignore");
  CHECK(app.skip_commits.files[1] == ".travis*");
  CHECK(app.skip_commits.files[2] == ".ci/travis*");
  CHECK(app.skip_commits.files[3] == ".ci/dev_*");
  CHECK(app.skip_commits.files[4] == ".ci/show_*");
  CHECK(app.skip_commits.files[5] == ".ci/vagrant*");
  CHECK(app.skip_commits.files[6] == ".ci/Vagrant*");
  CHECK(app.skip_commits.files[7] == "bm/html/*");
  CHECK(app.skip_commits.files[8] == "doc/*");
  CHECK(app.skip_commits.files[9] == "LICENSE.txt");
  CHECK(app.skip_commits.files[10] == "README.*");
}

TEST_CASE("test deserialize appveyor.yml") {
  try {
    std::string filestr = yaml_file_content("../data/appveyor.yml");
    std::vector<char> src(filestr.begin(), filestr.end());
    appveyor_t app;
    iguana::from_yaml(app, src.begin(), src.end());
    validator(app);
  } catch (...) {
    std::cout
        << " read appveyor.yml error, check the location of appveyor.yml\n\n";
  }
}

struct apt_t {
  std::vector<std::string_view> sources;
};
REFLECTION(apt_t, sources);
struct addon_t {
  apt_t apt;
};
REFLECTION(addon_t, apt);

struct env_t {
  std::vector<std::string_view> global;
};
REFLECTION(env_t, global);

struct in_env_t {
  std::string_view env;
};
REFLECTION(in_env_t, env);

struct mat_t {
  std::vector<in_env_t> include;
};
REFLECTION(mat_t, include);

struct travis_t {
  std::string_view sudo;
  std::string_view dist;
  std::string_view language;
  addon_t addons;
  env_t env;
  mat_t matrix;
  std::vector<std::string_view> install;
  std::vector<std::string_view> script;
  std::vector<std::string_view> after_success;
};
REFLECTION(travis_t, sudo, dist, language, addons, env, matrix, install, script,
           after_success);

void validator(travis_t tra) {
  CHECK(tra.sudo == "required");
  CHECK(tra.dist == "trusty");
  CHECK(tra.language == "cpp");
  CHECK(tra.addons.apt.sources[0] == "ubuntu-toolchain-r-test");
  CHECK(tra.env.global[0] == "PATH=/usr/local/bin:$PATH");
  CHECK(tra.matrix.include[0].env == "CXX_=g++-7 A=64 BT=Coverage");
  CHECK(tra.matrix.include[1].env == "CXX_=g++-7 A=32 BT=Coverage");
  CHECK(tra.matrix.include[2].env ==
        "CXX_=g++-7       A=32 BT=Debug   SAN=ALL VG=ON");

  CHECK(tra.matrix.include[3].env ==
        "CXX_=g++-7       A=64 BT=Debug   SAN=ALL VG=ON");
  CHECK(tra.matrix.include[4].env ==
        "CXX_=g++-7       A=32 BT=Release SAN=ALL VG=ON");
  CHECK(tra.matrix.include[5].env ==
        "CXX_=g++-7       A=64 BT=Release SAN=ALL VG=ON");

  CHECK(tra.matrix.include[44].env ==
        "CXX_=clang++-6.0 A=32 BT=Release LINT=clang-tidy");
  CHECK(tra.matrix.include[45].env ==
        "CXX_=clang++-6.0 A=64 BT=Release LINT=clang-tidy");
  CHECK(tra.install[0] == "bash -x .ci/travis-install.sh");
  CHECK(tra.script[0] == "bash -x .ci/travis-test.sh");
  CHECK(tra.after_success[0] == "echo \"Success!\"");
  CHECK(tra.after_success[1] == "bash -x .ci/travis-coverage.sh");
}

TEST_CASE("test deserialize travis.yml") {
  try {
    std::string filestr = yaml_file_content("../data/travis.yml");
    travis_t tra;
    iguana::from_yaml(tra, filestr.begin(), filestr.end());
    validator(tra);
  } catch (...) {
    std::cout << " read travis.yml error, check the location of travis.yml\n\n";
  }
}