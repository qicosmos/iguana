#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "iguana/json_dom.hpp"
#include "iguana/json_reader.hpp"

#ifdef IGUANA_DOM_BENCHMARK_REFERENCES
#include <simdjson.h>
#include <yyjson.h>

#include <nlohmann/json.hpp>
#endif

namespace {
volatile size_t result_size;

template <typename Parse>
void measure(const char* name, size_t bytes, Parse parse) {
  using clock = std::chrono::steady_clock;
  const auto iterations = std::clamp<size_t>(16 * 1024 * 1024 / bytes, 8, 5000);
  std::vector<double> timings;
  result_size = parse();
  for (int round = 0; round < 5; ++round) {
    size_t total = 0;
    auto start = clock::now();
    for (size_t i = 0; i < iterations; ++i) {
      total += parse();
    }
    auto elapsed = std::chrono::duration<double>(clock::now() - start);
    result_size = total;
    timings.push_back(bytes * iterations / (elapsed.count() * 1e6));
  }
  std::sort(timings.begin(), timings.end());
  std::cout << std::left << std::setw(18) << name << " " << std::right
            << std::fixed << std::setprecision(1) << std::setw(8) << timings[2]
            << " MB/s\n";
}

void compare(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot open " + filename);
  }
  std::string input((std::istreambuf_iterator<char>(file)), {});
  if (input.empty()) {
    throw std::runtime_error("empty input " + filename);
  }

  std::cout << filename << " (" << input.size() << " bytes)\n";
  measure("iguana json", input.size(), [&] {
    return iguana::json::parse(input).size();
  });
  iguana::json_parser iguana_parser;
  measure("iguana reused", input.size(), [&] {
    return iguana_parser.parse(input).size();
  });
  measure("iguana jvalue", input.size(), [&] {
    iguana::jvalue value;
    iguana::parse(value, input);
    if (auto array = std::get_if<iguana::jarray>(&value.base())) {
      return array->size();
    }
    if (auto object = std::get_if<iguana::jobject>(&value.base())) {
      return object->size();
    }
    return size_t{1};
  });

#ifdef IGUANA_DOM_BENCHMARK_REFERENCES
  measure("nlohmann/json", input.size(), [&] {
    return nlohmann::json::parse(input).size();
  });
  simdjson::padded_string padded(input);
  simdjson::dom::parser parser;
  auto simd_size = [](simdjson::dom::element value) {
    if (value.is_array()) {
      return value.get_array().value().size();
    }
    if (value.is_object()) {
      return value.get_object().value().size();
    }
    return size_t{1};
  };
  measure("simdjson fresh", input.size(), [&] {
    simdjson::dom::parser fresh;
    return simd_size(fresh.parse(padded).value());
  });
  measure("simdjson reused", input.size(), [&] {
    return simd_size(parser.parse(padded).value());
  });
  auto parse_yyjson = [&](const yyjson_alc* allocator) {
    auto* document =
        yyjson_read_opts(input.data(), input.size(), 0, allocator, nullptr);
    if (!document) {
      throw std::runtime_error("yyjson parse error");
    }
    auto* root = yyjson_doc_get_root(document);
    auto size = yyjson_is_arr(root)   ? yyjson_arr_size(root)
                : yyjson_is_obj(root) ? yyjson_obj_size(root)
                                      : size_t{1};
    yyjson_doc_free(document);
    return size;
  };
  measure("yyjson", input.size(), [&] {
    return parse_yyjson(nullptr);
  });
  const auto pool_size = yyjson_read_max_memory_usage(input.size(), 0);
  std::unique_ptr<char[]> pool_buffer(new char[pool_size]);
  yyjson_alc pool;
  if (!yyjson_alc_pool_init(&pool, pool_buffer.get(), pool_size)) {
    throw std::runtime_error("cannot initialize yyjson memory pool");
  }
  measure("yyjson pool", input.size(), [&] {
    return parse_yyjson(&pool);
  });
#endif
  std::cout << '\n';
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: json_dom_benchmark FILE [FILE ...]\n";
    return 1;
  }
  try {
    for (int i = 1; i < argc; ++i) {
      compare(argv[i]);
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
