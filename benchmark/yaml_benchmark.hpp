#include <iostream>

#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"

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

/*--------- YLT_REFL appveyor.yml-------------*/

struct matrix_t {
  std::string_view compiler;
  std::string_view generator;
  std::string_view configuration;
};
YLT_REFL(matrix_t, compiler, generator, configuration);
struct environment_t {
  std::vector<matrix_t> matrix;
};
YLT_REFL(environment_t, matrix);
struct ex_matrix_t {
  std::string_view fast_finish;
};
YLT_REFL(ex_matrix_t, fast_finish);
struct artifact_t {
  std::string_view path;
  std::string_view name;
};
YLT_REFL(artifact_t, path, name);
struct skip_commit_t {
  std::vector<std::string_view> files;
};
YLT_REFL(skip_commit_t, files);
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
YLT_REFL(appveyor_t, version, image, environment, matrix, install, build_script,
         test_script, artifacts, skip_commits);

/*--------- YLT_REFL travis.yml-------------*/
struct apt_t {
  std::vector<std::string_view> sources;
};
YLT_REFL(apt_t, sources);
struct addon_t {
  apt_t apt;
};
YLT_REFL(addon_t, apt);

struct env_t {
  std::vector<std::string_view> global;
};
YLT_REFL(env_t, global);

struct in_env_t {
  std::string_view env;
};
YLT_REFL(in_env_t, env);

struct mat_t {
  std::vector<in_env_t> include;
};
YLT_REFL(mat_t, include);

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
YLT_REFL(travis_t, sudo, dist, language, addons, env, matrix, install, script,
         after_success);
