#pragma once
#include "reflection.hpp"

namespace iguana {
using base = detail::base;

template <typename T, typename U>
IGUANA_INLINE constexpr size_t member_offset(T* t, U T::*member) {
  return (char*)&(t->*member) - (char*)t;
}

template <typename T>
struct base_impl : public base {
  void to_pb(std::string& str) override {
    to_pb_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void from_pb(std::string_view str) override {
    from_pb_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void to_json(std::string& str) override {
    to_json_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void from_json(std::string_view str) override {
    from_json_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void to_xml(std::string& str) override {
    to_xml_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void from_xml(std::string_view str) override {
    from_xml_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void to_yaml(std::string& str) override {
    to_yaml_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  void from_yaml(std::string_view str) override {
    from_yaml_adl((iguana_adl_t*)nullptr, *(static_cast<T*>(this)), str);
  }

  iguana::detail::field_info get_field_info(std::string_view name) override {
    static constexpr auto map = iguana::get_members<T>();
    iguana::detail::field_info info{};
    for (auto& [no, field] : map) {
      if (info.offset > 0) {
        break;
      }
      std::visit(
          [&](auto val) {
            if (val.field_name == name) {
              info.offset = member_offset((T*)this, val.member_ptr);
              using value_type = typename decltype(val)::value_type;
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 8)
              info.type_name = type_string<value_type>();
#endif
            }
          },
          field);
    }

    return info;
  }

  std::vector<std::string_view> get_fields_name() override {
    static constexpr auto map = iguana::get_members<T>();
    std::vector<std::string_view> vec;
    for (auto [no, val] : map) {
      std::visit(
          [&](auto& field) {
            vec.push_back(std::string_view(field.field_name.data(),
                                           field.field_name.size()));
          },
          val);
    }
    return vec;
  }

  virtual ~base_impl() {}

  size_t cache_size = 0;
};

IGUANA_INLINE std::shared_ptr<base> create_instance(std::string_view name) {
  auto it = iguana::detail::g_pb_map.find(name);
  if (it == iguana::detail::g_pb_map.end()) {
    throw std::invalid_argument(std::string(name) +
                                "not inheried from iguana::base_impl");
  }
  return it->second();
}
}  // namespace iguana