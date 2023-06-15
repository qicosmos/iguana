#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <iostream>

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

/*--------------------------- REFLECTION the struct
 * ----------------------------*/

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
  assert(app.version == "{build}");
  assert(app.image == "Visual Studio 2017");
  assert(app.environment.matrix[0].compiler == "msvc-15-seh");
  assert(app.environment.matrix[0].generator == "Visual Studio 15 2017");
  assert(app.environment.matrix[0].configuration == "Debug");

  assert(app.environment.matrix[1].compiler == "msvc-15-seh");
  assert(app.environment.matrix[1].generator == "Visual Studio 15 2017 Win64");
  assert(app.environment.matrix[1].configuration == "Debug");

  assert(app.environment.matrix[2].compiler == "msvc-15-seh");
  assert(app.environment.matrix[2].generator == "Visual Studio 15 2017");
  assert(app.environment.matrix[2].configuration == "Release");

  assert(app.environment.matrix[3].compiler == "msvc-15-seh");
  assert(app.environment.matrix[3].generator == "Visual Studio 15 2017 Win64");
  assert(app.environment.matrix[3].configuration == "Release");

  assert(app.environment.matrix[4].compiler == "msvc-14-seh");
  assert(app.environment.matrix[4].generator == "Visual Studio 14 2015");
  assert(app.environment.matrix[4].configuration == "Debug");

  assert(app.environment.matrix[5].compiler == "msvc-14-seh");
  assert(app.environment.matrix[5].generator == "Visual Studio 14 2015 Win64");
  assert(app.environment.matrix[5].configuration == "Debug");

  assert(app.environment.matrix[6].compiler == "msvc-14-seh");
  assert(app.environment.matrix[6].generator == "Visual Studio 14 2015");
  assert(app.environment.matrix[6].configuration == "Release");

  assert(app.environment.matrix[7].compiler == "msvc-14-seh");
  assert(app.environment.matrix[7].generator == "Visual Studio 14 2015 Win64");
  assert(app.environment.matrix[7].configuration == "Release");

  assert(app.matrix.fast_finish == "true");

  assert(app.install[0] == "git submodule update --init --recursive");
  assert(app.install[1] ==
         "if \"%generator%\"==\"MinGW Makefiles\" (set "
         "\"PATH=%PATH:C:\\Program Files\\Git\\usr\\bin;=%\")");
  assert(app.install[2] ==
         "if not \"%cxx_path%\"==\"\" (set \"PATH=%PATH%;%cxx_path%\")");

  assert(app.build_script[0] == "md _build -Force");
  assert(app.build_script[1] == "cd _build");

  assert(app.test_script[0] == "echo %configuration%");
  assert(app.test_script[1] ==
         "cmake -G \"%generator%\" \"-DCMAKE_BUILD_TYPE=%configuration%\" "
         "-DRYML_DEV=ON ..");
  assert(app.test_script[2] == "dir");
  assert(app.test_script[3] == "dir test");
  assert(app.test_script[4] ==
         "cmake --build . --config %configuration% --target ryml-test");

  assert(app.artifacts[0].path == "_build/CMakeFiles/*.log");
  assert(app.artifacts[0].name == "logs");
  assert(app.artifacts[1].path == "_build/Testing/**/*.xml");
  assert(app.artifacts[1].name == "test_results");

  assert(app.skip_commits.files[0] == ".gitignore");
  assert(app.skip_commits.files[1] == ".travis*");
  assert(app.skip_commits.files[2] == ".ci/travis*");
  assert(app.skip_commits.files[3] == ".ci/dev_*");
  assert(app.skip_commits.files[4] == ".ci/show_*");
  assert(app.skip_commits.files[5] == ".ci/vagrant*");
  assert(app.skip_commits.files[6] == ".ci/Vagrant*");
  assert(app.skip_commits.files[7] == "bm/html/*");
  assert(app.skip_commits.files[8] == "doc/*");
  assert(app.skip_commits.files[9] == "LICENSE.txt");
  assert(app.skip_commits.files[10] == "README.*");
}