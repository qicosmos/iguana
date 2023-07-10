#include "../rapidxml/rapidxml.hpp"
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
const int iterations = 1000;
void bench_de_sample_filelists() {

  std::string xmlfilelist = xml_file_content("../data/rpm_filelists.xml");
  rapidxml::xml_document<> doc;
  {
    ScopedTimer timer("rapidxml fastest parse rpm_filelists.xml");
    for (int i = 0; i < iterations; ++i) {
      doc.parse<rapidxml::parse_fastest>(xmlfilelist.data());
      doc.clear();
    }
  }

  filelists_t filelist;
  {
    ScopedTimer timer("iguana_xml deserialize rpm_filelists.xml");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_xml(filelist, xmlfilelist.begin(), xmlfilelist.end());
      filelist.package.clear();
    }
  }
}

void bench_de_sample_rss() {
  std::string xmlrss = xml_file_content("../data/sample_rss.xml");
  rapidxml::xml_document<> doc;
  {
    ScopedTimer timer("rapidxml fastest parse sample_rss.xml");
    for (int i = 0; i < iterations; ++i) {
      doc.parse<rapidxml::parse_fastest>(xmlrss.data());
      doc.clear();
    }
  }

  rss_t rss;
  {
    ScopedTimer timer("iguana_xml deserialize sample_rss.xml");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_xml(rss, xmlrss.begin(), xmlrss.end());
      rss.channel.item.clear();
    }
  }
}

void bench_num() {
  std::string xmlnum = xml_file_content("../data/bench_num.xml");

  store_t s;
  {
    ScopedTimer timer("iguana_xml deserialize bench_num.xml");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_xml(s, xmlnum);
      s.goods.clear();
    }
  }

  rapidxml::xml_document<> doc;
  {
    ScopedTimer timer("rapidxml fastest parse bench_num.xml");
    for (int i = 0; i < iterations; ++i) {

      doc.parse<rapidxml::parse_fastest>(xmlnum.data());
      doc.clear();
    }
  }

  // store_t store;
  // iguana::from_xml(store, xmlnum);
  // std::string ss;
  // ss.reserve(xmlnum.size());
  // {
  //   ScopedTimer timer("iguana_xml serialize bench_num.xml");
  //   for (int i = 0; i < iterations; ++i) {
  //     iguana::to_xml(store, ss);
  //     ss.clear();
  //   }
  // }
}

int main() {
  bench_de_sample_filelists();
  bench_de_sample_rss();
  bench_num();
}
