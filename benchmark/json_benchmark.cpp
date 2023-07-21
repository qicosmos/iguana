#define SEQUENTIAL_PARSE
#include "json_benchmark.h"

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
  template <typename Writer> void Serialize(Writer &writer) const {
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
REFLECTION(fixed_name_object_t, name0, name1, name2, name3, name4);

struct nested_object_t {
  std::vector<std::array<double, 3>> v3s{};
  std::string id{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
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
REFLECTION(nested_object_t, v3s, id);

struct another_object_t {
  std::string string{};
  std::string another_string{};
  bool boolean{};
  nested_object_t nested_object{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
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
REFLECTION(another_object_t, string, another_string, boolean, nested_object);

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
  template <typename Writer> void Serialize(Writer &writer) const {
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
REFLECTION(obj_t, fixed_name_object, another_object, string_array, string,
           number, boolean, another_bool);

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

void test_from_json(std::string filename, auto &obj, const auto &json_str,
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

void test_dom_parse(std::string filename, const auto &json_str,
                    const int size) {
  std::string iguana_str = "iguana parse " + filename;
  iguana::jvalue val;
  {
    ScopedTimer timer(iguana_str.data());
    for (int i = 0; i < size; ++i) {
      iguana::parse(val, json_str);
    }
  }
}

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

using variant =
    std::variant<FeatureCollection, apache_builds, citm_object_t, gsoc_object_t,
                 mesh_t, random_t, githubEvents::events_t,
                 marine_ik::marine_ik_t, std::vector<double>, instruments_t>;

static std::map<std::string, variant> test_map{
    {"../data/canada.json", FeatureCollection{}},
    {"../data/apache_builds.json", apache_builds{}},
    {"../data/citm_catalog.json", citm_object_t{}},
    {"../data/gsoc-2018.json", gsoc_object_t{}},
    {"../data/mesh.pretty.json", mesh_t{}},
    {"../data/random.json", random_t{}},
    {"../data/github_events.json", githubEvents::events_t{}},
    {"../data/marine_ik.json", marine_ik::marine_ik_t{}},
    {"../data/numbers.json", std::vector<double>{}},
    {"../data/instruments.json", instruments_t{}},
};

void test_from_json_file() {
  for (auto &pair : test_map) {
    auto content = iguana::json_file_content(pair.first);
    std::visit(
        [&](auto &&arg) { test_from_json(pair.first, arg, content, 10); },
        pair.second);
  }
}

void test_dom_parse_file() {
  for (auto &pair : test_map) {
    auto content = iguana::json_file_content(pair.first);
    test_dom_parse(pair.first, content, 10);
  }
}

int main() {
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
}