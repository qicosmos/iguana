#include "xml_bench.hpp"

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

std::string xml_file_content(const std::string &filename) {
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

void test_deserialize() {
  int iterations = 1000;

  std::cout << "============ deserialize rpm_filelists.xml  ===============\n";
  std::string xmlfilelist = xml_file_content("../data/rpm_filelists.xml");
  {

    ScopedTimer timer("test deserialize rpm_filelists.xml");
    for (int i = 0; i < iterations; ++i) {
      filelists_t filelist;
      iguana::from_xml(filelist, xmlfilelist.begin(), xmlfilelist.end());
    }
  }
  // std::cout << "============ deserialize sample_rss.xml ===============\n";

  // std::string xmlrss = xml_file_content("../data/sample_rss.xml");
  // {

  //   ScopedTimer timer("test deserialize sample_rss.xml");
  //   for (int i = 0; i < iterations; ++i) {
  //     rss_t rss;
  //     iguana::from_xml<true>(rss, xmlrss.begin(), xmlrss.end());
  //     assert(rss.channel.item.size() == 99);
  //   }
  // }
}

void bench_num() {
  int iterations = 1000;
  std::cout << "============ deserialize bench_num.xml  ===============\n";
  std::string xmlnum = xml_file_content("../data/bench_num.xml");
  {
    ScopedTimer timer("test deserialize bench_num.xml");
    for (int i = 0; i < iterations; ++i) {
      store_t s;
      iguana::from_xml(s, xmlnum);
    }
  }
  std::cout << "============ serialize bench_num.xml  ===============\n";
  store_t store;
  iguana::from_xml(store, xmlnum);
  std::string ss;
  ss.reserve(xmlnum.size());
  {
    ScopedTimer timer("test serialize bench_num.xml");
    for (int i = 0; i < iterations; ++i) {
      iguana::to_xml(store, ss);
      ss.clear();
    }
  }
}

int main() {
  test_deserialize();
  bench_num();
}
