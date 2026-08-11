#include "json_benchmark.h"

#include <cstdlib>
#include <iomanip>
#include <stdexcept>

#include "iguana/json_dom.hpp"

#ifdef HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#ifdef HAS_SIMDJSON
#include <simdjson.h>
#endif

#ifdef HAS_YYJSON
#include <yyjson.h>
#endif

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

namespace {
volatile size_t json_parse_sink = 0;

void consume_json_parse_result(size_t value) {
  json_parse_sink = json_parse_sink + value;
}

void print_parse_result(const std::string &name, uint64_t ns, size_t bytes,
                        int iterations) {
  const double seconds = static_cast<double>(ns) / 1000000000.0;
  const double gib = static_cast<double>(bytes) *
                     static_cast<double>(iterations) /
                     (1024.0 * 1024.0 * 1024.0);
  const double gib_per_second = seconds > 0 ? gib / seconds : 0.0;

  auto flags = std::cout.flags();
  auto precision = std::cout.precision();
  std::cout << std::left << std::setw(45) << name << " : " << std::right
            << std::setw(12) << ns << " ns  " << std::fixed
            << std::setprecision(3) << gib_per_second << " GiB/s\n";
  std::cout.flags(flags);
  std::cout.precision(precision);
}

bool should_run_parser(std::string_view parser_name,
                       std::string_view parser_filter) {
  return parser_filter.empty() ||
         parser_name.find(parser_filter) != std::string_view::npos;
}
}  // namespace

inline constexpr std::string_view json0 = R"(
{
   "fixed_name_object": {
      "name0": "James",
      "name1": "Abraham",
      "name2": "Susan",
      "name3": "Frank",
      "name4": "Alicia"
   },
   "another_object": {
      "string": "here is some text",
      "another_string": "Hello World",
      "boolean": false,
      "nested_object": {
         "v3s": [[0.12345, 0.23456, 0.001345],
                  [0.3894675, 97.39827, 297.92387],
                  [18.18, 87.289, 2988.298]],
         "id": "298728949872"
      }
   },
   "string_array": ["Cat", "Dog", "Elephant", "Tiger"],
   "string": "Hello world",
   "number": 3.14,
   "boolean": true,
   "another_bool": false
}
)";

struct fixed_name_object_t {
  std::string name0{};
  std::string name1{};
  std::string name2{};
  std::string name3{};
  std::string name4{};

#ifdef HAS_RAPIDJSON
  template <typename Writer>
  void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("name0");
    writer.String(name0);
    writer.String("name1");
    writer.String(name1);
    writer.String("name2");
    writer.String(name2);
    writer.String("name3");
    writer.String(name3);
    writer.String("name4");
    writer.String(name4);
    writer.EndObject();
  }
#endif
};
YLT_REFL(fixed_name_object_t, name0, name1, name2, name3, name4);

struct nested_object_t {
  std::vector<std::array<double, 3>> v3s{};
  std::string id{};

#ifdef HAS_RAPIDJSON
  template <typename Writer>
  void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("v3s");
    writer.StartArray();
    for (auto &arr : v3s) {
      writer.StartArray();
      for (auto &d : arr) {
        writer.Double(d);
      }

      writer.EndArray();
    }

    writer.EndArray();
    writer.String("id");
    writer.String(id);
    writer.EndObject();
  }
#endif
};
YLT_REFL(nested_object_t, v3s, id);

struct another_object_t {
  std::string string{};
  std::string another_string{};
  bool boolean{};
  nested_object_t nested_object{};

#ifdef HAS_RAPIDJSON
  template <typename Writer>
  void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("string");
    writer.String(string);
    writer.String("another_string");
    writer.String(another_string);
    writer.String("boolean");
    writer.Bool(boolean);
    writer.String("nested_object");
    nested_object.Serialize(writer);
    writer.EndObject();
  }
#endif
};
YLT_REFL(another_object_t, string, another_string, boolean, nested_object);

struct obj_t {
  //   fixed_object_t fixed_object{};
  fixed_name_object_t fixed_name_object{};
  another_object_t another_object{};
  std::vector<std::string> string_array{};
  std::string string{};
  double number{};
  bool boolean{};
  bool another_bool{};

#ifdef HAS_RAPIDJSON
  template <typename Writer>
  void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("fixed_name_object");
    fixed_name_object.Serialize(writer);
    writer.String("another_object");
    another_object.Serialize(writer);
    writer.String("string_array");
    writer.StartArray();
    for (auto &str : string_array) {
      writer.String(str);
    }
    writer.EndArray();
    writer.String("string");
    writer.String(string);

    writer.String("number");
    writer.Double(number);

    writer.String("boolean");
    writer.Bool(boolean);

    writer.String("another_bool");
    writer.Bool(another_bool);

    writer.EndObject();
  }
#endif
};
YLT_REFL(obj_t, fixed_name_object, another_object, string_array, string, number,
         boolean, another_bool);

obj_t create_object() {
  fixed_name_object_t fix_obj = {"James", "Abraham", "Susan", "Frank",
                                 "Alicia"};

  nested_object_t nested_obj = {{{0.12345, 0.23456, 0.001345},
                                 {0.3894675, 97.39827, 297.92387},
                                 {18.18, 87.289, 2988.298}},
                                "298728949872"};

  another_object_t another_obj = {"here is some text", "Hello World", false,
                                  nested_obj};

  obj_t obj = {fix_obj,       another_obj, {"Cat", "Dog", "Elephant", "Tiger"},
               "Hello world", 3.14,        true,
               false};
  return obj;
}

template <typename T, typename S>
void test_from_json(std::string filename, T &obj, const S &json_str,
                    const int size) {
  iguana::from_json(obj, std::begin(json_str), std::end(json_str));
  std::string iguana_str = "iguana from_json " + filename;
  {
    ScopedTimer timer(iguana_str.data());
    for (int i = 0; i < size; ++i) {
      iguana::from_json(obj, std::begin(json_str), std::end(json_str));
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::Document doc;
  doc.Parse(json_str.data(), json_str.size());
  std::vector<std::string> json_str_arr(size);
  for (int i = 0; i < size; ++i) {
    json_str_arr[i] = json_str;
  }
  std::string rapidjson_str = "rapidjson parse " + filename;
  {
    ScopedTimer timer(rapidjson_str.data());
    for (int i = 0; i < size; ++i) {
      doc = {};
      doc.ParseInsitu(const_cast<char *>(json_str_arr[i].data()));
    }
  }
#endif
}

template <typename S>
void test_dom_parse(std::string filename, const S &json_str, const int size) {
  std::string iguana_str = "iguana parse " + filename;
  iguana::jvalue val;
  uint64_t ns = 0;
  {
    ScopedTimer timer(iguana_str.data(), ns);
    for (int i = 0; i < size; ++i) {
      iguana::parse(val, json_str);
    }
  }
  print_parse_result(iguana_str, ns, json_str.size(), size);
}

template <typename S>
void test_iguana_json_dom_parse(std::string filename, const S &json_str,
                                const int size) {
  std::string name = "iguana json::dom parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      auto doc = iguana::json::parse(
          std::string_view(json_str.data(), json_str.size()));
      consume_json_parse_result(doc.node_count());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

template <typename S>
void test_iguana_json_dom_view_parse(std::string filename, const S &json_str,
                                     const int size) {
  std::string name = "iguana json::dom view parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      auto doc = iguana::json::parse_view(
          std::string_view(json_str.data(), json_str.size()));
      consume_json_parse_result(doc.node_count());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

template <typename S>
void test_iguana_json_dom_reuse_view_parse(std::string filename,
                                           const S &json_str, const int size) {
  std::string name = "iguana json::dom reuse view parse " + filename;
  iguana::json::document doc;
  doc.reserve(json_str.size());
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      iguana::json::parse_view(
          doc, std::string_view(json_str.data(), json_str.size()));
      consume_json_parse_result(doc.node_count());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

template <typename S>
void test_iguana_json_dom_lazy_reuse_view_parse(std::string filename,
                                                const S &json_str,
                                                const int size) {
  std::string name = "iguana json::dom lazy reuse view parse " + filename;
  iguana::json::document doc;
  doc.reserve(json_str.size());
  iguana::json::parse_options options;
  options.numbers = iguana::json::number_mode::lazy;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      iguana::json::parse_view(
          doc, std::string_view(json_str.data(), json_str.size()), options);
      consume_json_parse_result(doc.node_count());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

template <typename S>
void test_iguana_json_dom_trusted_reuse_view_parse(std::string filename,
                                                   const S &json_str,
                                                   const int size) {
  std::string name = "iguana json::dom trusted reuse view parse " + filename;
  iguana::json::document doc;
  doc.reserve(json_str.size());
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      iguana::json::parse_trusted_view(
          doc, std::string_view(json_str.data(), json_str.size()));
      consume_json_parse_result(doc.node_count());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

#ifdef HAS_NLOHMANN_JSON
template <typename S>
void test_nlohmann_dom_parse(std::string filename, const S &json_str,
                             const int size) {
  std::string name = "nlohmann parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      auto val = nlohmann::json::parse(json_str.begin(), json_str.end());
      consume_json_parse_result(val.size());
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}
#endif

#ifdef HAS_YYJSON
template <typename S>
void test_yyjson_dom_parse(std::string filename, const S &json_str,
                           const int size) {
  std::string name = "yyjson parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      yyjson_doc *doc = yyjson_read(json_str.data(), json_str.size(), 0);
      if (doc == nullptr) {
        throw std::runtime_error("yyjson failed to parse " + filename);
      }
      yyjson_val *root = yyjson_doc_get_root(doc);
      consume_json_parse_result(root == nullptr ? 0 : yyjson_get_type(root));
      yyjson_doc_free(doc);
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}

template <typename S>
void test_yyjson_insitu_dom_parse(std::string filename, const S &json_str,
                                  const int size) {
  std::vector<std::string> json_copies(size);
  for (int i = 0; i < size; ++i) {
    json_copies[i].assign(json_str.begin(), json_str.end());
  }

  std::string name = "yyjson insitu parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      yyjson_doc *doc =
          yyjson_read_opts(json_copies[i].data(), json_copies[i].size(),
                           YYJSON_READ_INSITU, nullptr, nullptr);
      if (doc == nullptr) {
        throw std::runtime_error("yyjson insitu failed to parse " + filename);
      }
      yyjson_val *root = yyjson_doc_get_root(doc);
      consume_json_parse_result(root == nullptr ? 0 : yyjson_get_type(root));
      yyjson_doc_free(doc);
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}
#endif

#ifdef HAS_SIMDJSON
template <typename S>
void test_simdjson_dom_parse(std::string filename, const S &json_str,
                             const int size) {
  simdjson::padded_string padded(json_str.data(), json_str.size());
  simdjson::dom::parser parser;
  if (auto error = parser.allocate(padded.size())) {
    throw std::runtime_error(simdjson::error_message(error));
  }

  std::string name = "simdjson dom parse " + filename;
  uint64_t ns = 0;
  {
    ScopedTimer timer(name.data(), ns);
    for (int i = 0; i < size; ++i) {
      auto doc = parser.parse(padded);
      if (doc.error()) {
        throw std::runtime_error(simdjson::error_message(doc.error()));
      }
      simdjson::dom::element_type type;
      if (auto error = doc.type().get(type)) {
        throw std::runtime_error(simdjson::error_message(error));
      }
      consume_json_parse_result(static_cast<size_t>(type));
    }
  }
  print_parse_result(name, ns, json_str.size(), size);
}
#endif

void test_to_json() {
  int iterations = 100000;
  obj_t obj = create_object();

  iguana::string_stream ss;
  iguana::to_json(obj, ss);

  {
    ScopedTimer timer("iguana   to  json");
    for (int i = 0; i < iterations; ++i) {
      ss.clear();
      iguana::to_json(obj, ss);
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  obj.Serialize(writer);

  {
    ScopedTimer timer("rapidjson to json");
    for (int i = 0; i < iterations; ++i) {
      sb.Clear();
      writer.Reset(sb);
      obj.Serialize(writer);
    }
  }
#endif
}

using variant = std::variant<FeatureCollection, apache_builds, citm_object_t,
                             gsoc_object_t, mesh_t, random_t,
                             githubEvents::events_t, marine_ik::marine_ik_t,
                             std::vector<iguana::numeric_str>, instruments_t>;

static std::map<std::string, variant> test_map{
    {"../data/canada.json", FeatureCollection{}},
    {"../data/apache_builds.json", apache_builds{}},
    {"../data/citm_catalog.json", citm_object_t{}},
    {"../data/gsoc-2018.json", gsoc_object_t{}},
    {"../data/mesh.pretty.json", mesh_t{}},
    {"../data/random.json", random_t{}},
    {"../data/github_events.json", githubEvents::events_t{}},
    {"../data/marine_ik.json", marine_ik::marine_ik_t{}},
    {"../data/numbers.json", std::vector<iguana::numeric_str>{}},
    {"../data/instruments.json", instruments_t{}},
};

void test_from_json_file() {
  for (auto &pair : test_map) {
    auto content = iguana::json_file_content(pair.first);
    std::visit(
        [&](auto &&arg) {
          test_from_json(pair.first, arg, content, 10);
        },
        pair.second);
  }
}

void test_dom_parse_file(int iterations = 10, std::string_view filter = {},
                         std::string_view parser_filter = {}) {
  for (auto &pair : test_map) {
    if (!filter.empty() && pair.first.find(filter) == std::string::npos) {
      continue;
    }
    auto content = iguana::json_file_content(pair.first);
    if (should_run_parser("iguana parse", parser_filter)) {
      test_dom_parse(pair.first, content, iterations);
    }
    if (should_run_parser("iguana json::dom parse", parser_filter)) {
      test_iguana_json_dom_parse(pair.first, content, iterations);
    }
    if (should_run_parser("iguana json::dom view parse", parser_filter)) {
      test_iguana_json_dom_view_parse(pair.first, content, iterations);
    }
    if (should_run_parser("iguana json::dom reuse view parse", parser_filter)) {
      test_iguana_json_dom_reuse_view_parse(pair.first, content, iterations);
    }
    if (should_run_parser("iguana json::dom lazy reuse view parse",
                          parser_filter)) {
      test_iguana_json_dom_lazy_reuse_view_parse(pair.first, content,
                                                 iterations);
    }
    if (should_run_parser("iguana json::dom trusted reuse view parse",
                          parser_filter)) {
      test_iguana_json_dom_trusted_reuse_view_parse(pair.first, content,
                                                    iterations);
    }
#ifdef HAS_NLOHMANN_JSON
    if (should_run_parser("nlohmann parse", parser_filter)) {
      test_nlohmann_dom_parse(pair.first, content, iterations);
    }
#endif
#ifdef HAS_YYJSON
    if (should_run_parser("yyjson parse", parser_filter)) {
      test_yyjson_dom_parse(pair.first, content, iterations);
    }
    if (should_run_parser("yyjson insitu parse", parser_filter)) {
      test_yyjson_insitu_dom_parse(pair.first, content, iterations);
    }
#endif
#ifdef HAS_SIMDJSON
    if (should_run_parser("simdjson dom parse", parser_filter)) {
      test_simdjson_dom_parse(pair.first, content, iterations);
    }
#endif
  }
}

int run_dom_parse_benchmark(int rounds = 1, int iterations = 10,
                            std::string_view filter = {},
                            std::string_view parser_filter = {}) {
  for (int i = 0; i < rounds; ++i) {
    test_dom_parse_file(iterations, filter, parser_filter);
    std::cout << "====================\n";
  }
  return 0;
}

int run_benchmark() {
  for (size_t i = 0; i < 5; i++) {
    test_from_json_file();
    std::cout << "====================\n";
  }

  for (size_t i = 0; i < 5; i++) {
    test_dom_parse_file();
    std::cout << "====================\n";
  }

  for (int i = 0; i < 10; ++i) {
    test_to_json();
    std::cout << "====================\n";
  }

  obj_t obj;
  for (int i = 0; i < 10; ++i) {
    test_from_json("obj_t", obj, json0, 100000);
    std::cout << "====================\n";
  }

  iguana::jvalue val;
  for (int i = 0; i < 10; ++i) {
    test_dom_parse("obj_t", json0, 100000);
    std::cout << "====================\n";
  }
  return 0;
}

int main(int argc, char **argv) {
  try {
    if (argc > 1 && std::string_view(argv[1]) == "dom") {
      int rounds = argc > 2 ? std::atoi(argv[2]) : 1;
      int iterations = argc > 3 ? std::atoi(argv[3]) : 10;
      if (rounds <= 0 || iterations <= 0) {
        throw std::runtime_error("rounds and iterations must be positive");
      }
      const std::string_view filter = argc > 4 ? std::string_view(argv[4]) : "";
      const std::string_view parser_filter =
          argc > 5 ? std::string_view(argv[5]) : "";
      return run_dom_parse_benchmark(rounds, iterations, filter, parser_filter);
    }
    return run_benchmark();
  } catch (const std::exception &e) {
    std::cerr << "json_benchmark failed: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "json_benchmark failed: unknown exception\n";
    return 1;
  }
}
