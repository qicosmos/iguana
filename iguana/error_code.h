#pragma once

#include <cassert>
#include <map>
#include <system_error>

namespace iguana {

class iguana_category : public std::error_category {
public:
  virtual const char *name() const noexcept override {
    return "iguna::category";
  }
  virtual std::string message(int err_val) const override {
    for (auto &pair : err_map_) {
      if (pair.second == err_val) {
        return pair.first;
      }
    }

    return "unrecognized error";
  }

  uint32_t add_message(const std::string &msg) {
    if (auto it = err_map_.find(msg); it != err_map_.end()) {
      return it->second;
    } else {
      err_++;
      err_map_.emplace(msg, err_);
      return err_;
    }
  }

  uint32_t err_ = 0;
  std::map<std::string, uint32_t> err_map_;
};

iguana::iguana_category &category() {
  static iguana::iguana_category instance;
  return instance;
}

inline std::error_code make_error_code(const std::string data = "") {
  assert(!data.empty());

  auto err = iguana::category().add_message(data);
  return std::error_code(err, iguana::category());
}
} // namespace iguana
