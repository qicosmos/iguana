//
// Created by Qiyu on 17-6-5.
//

#ifndef SERIALIZE_JSON_HPP
#define SERIALIZE_JSON_HPP
#include "detail/json_string_sink.hpp"
#include "json_util.hpp"

namespace iguana {

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<ylt_refletable_v<T>, int> = 0>
IGUANA_INLINE void to_json(T &&t, Stream &s);
namespace detail {
template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<optional_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &val);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<fixed_array_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &t);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<pair_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &o);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<tuple_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &s, T &&t);

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<variant_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &s, T &&t);

template <typename Stream, typename InputIt, typename T, typename F>
IGUANA_INLINE void join(Stream &ss, InputIt first, InputIt last, const T &delim,
                        const F &f) {
  if (first == last)
    return;
  f(*first++);
  while (first != last) {
    ss.push_back(delim);
    f(*first++);
  }
}

template <bool Is_writing_escape, typename Stream>
IGUANA_INLINE void to_json_impl(Stream &ss, std::nullptr_t) {
  ss.append("null");
}

template <bool Is_writing_escape, typename Stream>
IGUANA_INLINE void to_json_impl(Stream &ss, bool b) {
  ss.append(b ? "true" : "false");
};

template <bool Is_writing_escape, typename Stream>
IGUANA_INLINE void to_json_impl(Stream &ss, char value) {
  ss.append("\"");
  ss.push_back(value);
  ss.append("\"");
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, T value) {
  if constexpr (json_string_sink_v<Stream> && int_v<T> && sizeof(T) <= 8) {
    ss.write_integer(std::array<char, 0>{}, value);
  }
  else {
    char temp[65];
    auto p = detail::to_chars(temp, value);
    ss.append(temp, p - temp);
  }
}

// template <bool Is_writing_escape, typename Stream, typename T,
//           std::enable_if_t<is_pb_type_v<T>, int> = 0>
// IGUANA_INLINE void to_json_impl(Stream &ss, T value) {
//   to_json_impl<Is_writing_escape>(ss, value.val);
// }

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<numeric_str_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, T v) {
  ss.append(v.value().data(), v.value().size());
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, T &&t) {
  if constexpr (json_string_sink_v<Stream> &&
                std::is_convertible_v<T, std::string_view>) {
    ss.template write_quoted<Is_writing_escape>(std::array<char, 0>{}, t);
  }
  else {
    ss.push_back('"');
    if constexpr (Is_writing_escape) {
      write_string_with_escape(t.data(), t.size(), ss);
    }
    else {
      ss.append(t.data(), t.size());
    }
    ss.push_back('"');
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<num_v<T>, int> = 0>
IGUANA_INLINE void render_key(Stream &ss, const T &t) {
  ss.push_back('"');
  to_json_impl<Is_writing_escape>(ss, t);
  ss.push_back('"');
}

// template <bool Is_writing_escape, typename Stream, typename T,
//           std::enable_if_t<is_pb_type_v<T>, int> = 0>
// IGUANA_INLINE void render_key(Stream &ss, const T &t) {
//   render_key<Is_writing_escape>(ss, t.val);
// }

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<string_container_v<T>, int> = 0>
IGUANA_INLINE void render_key(Stream &ss, T &&t) {
  to_json_impl<Is_writing_escape>(ss, std::forward<T>(t));
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<ylt_refletable_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, T &&t) {
  to_json(std::forward<T>(t), ss);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<enum_v<T>, int> = 0>
IGUANA_INLINE void to_json_impl(Stream &ss, T val) {
  static constexpr auto enum_to_str = get_enum_map<false, std::decay_t<T>>();
  if constexpr (bool_v<decltype(enum_to_str)>) {
    to_json_impl<Is_writing_escape>(
        ss, static_cast<std::underlying_type_t<T>>(val));
  }
  else {
    auto it = enum_to_str.find(val);
    if (it != enum_to_str.end())
      IGUANA_LIKELY {
        auto str = it->second;
        to_json_impl<Is_writing_escape>(
            ss, std::string_view(str.data(), str.size()));
      }
    else {
      throw std::runtime_error(
          std::to_string(static_cast<std::underlying_type_t<T>>(val)) +
          " is a missing value in enum_value");
    }
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<optional_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &val) {
  if (!val) {
    ss.append("null");
  }
  else {
    to_json_impl<Is_writing_escape>(ss, *val);
  }
}

template <bool Is_writing_escape, typename Stream, typename T>
IGUANA_INLINE void render_array(Stream &ss, const T &v) {
  ss.push_back('[');
  join(ss, std::begin(v), std::end(v), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         to_json_impl<Is_writing_escape>(ss, jsv);
       });
  ss.push_back(']');
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<fixed_array_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &t) {
  if constexpr (std::is_same_v<char, std::remove_reference_t<
                                         decltype(std::declval<T>()[0])>>) {
    constexpr size_t n = sizeof(T) / sizeof(decltype(std::declval<T>()[0]));
    ss.push_back('"');
    auto get_length = [&t](int n) constexpr {
      for (int i = 0; i < n; ++i) {
        if (t[i] == '\0')
          return i;
      }
      return n;
    };
    size_t len = get_length(n);
    ss.append(std::data(t), len);
    ss.push_back('"');
  }
  else {
    render_array<Is_writing_escape>(ss, t);
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<map_container_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &o) {
  ss.push_back('{');
  join(ss, o.cbegin(), o.cend(), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         render_key<Is_writing_escape>(ss, jsv.first);
         ss.push_back(':');
         to_json_impl<Is_writing_escape>(ss, jsv.second);
       });
  ss.push_back('}');
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<sequence_container_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v) {
  constexpr char pre = pair_v<typename T::value_type> ? '{' : '[';
  ss.push_back(pre);
  join(ss, v.cbegin(), v.cend(), ',',
       [&ss](const auto &jsv) IGUANA__INLINE_LAMBDA {
         to_json_impl<Is_writing_escape>(ss, jsv);
       });
  constexpr char post = pair_v<typename T::value_type> ? '}' : ']';
  ss.push_back(post);
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<pair_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v) {
  render_key<Is_writing_escape>(ss, v.first);
  ss.push_back(':');
  to_json_impl<Is_writing_escape>(ss, v.second);
}

template <typename T, size_t Index, bool IncludeOpen = false>
constexpr auto make_json_key() {
  constexpr auto name = ylt::reflection::get_member_names<T>()[Index];
  constexpr size_t prefix = (Index != 0 || IncludeOpen) ? 1 : 0;
  std::array<char, name.size() + prefix + 3> key{};
  if constexpr (prefix != 0) {
    key[0] = Index == 0 ? '{' : ',';
  }
  key[prefix] = '"';
  for (size_t i = 0; i < name.size(); ++i) {
    key[prefix + i + 1] = name[i];
  }
  key[key.size() - 2] = '"';
  key[key.size() - 1] = ':';
  return key;
}

template <typename T, size_t Index>
inline constexpr auto json_key = make_json_key<T, Index>();

template <typename T, size_t... Indices>
constexpr auto make_json_keys(std::index_sequence<Indices...>) {
  return std::array<std::string_view, sizeof...(Indices)>{std::string_view(
      json_key<T, Indices>.data(), json_key<T, Indices>.size())...};
}

template <typename T>
inline constexpr auto json_keys = make_json_keys<T>(
    std::make_index_sequence<ylt::reflection::members_count_v<T>>{});

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<smart_ptr_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &ss, const T &v) {
  if (v) {
    to_json_impl<Is_writing_escape>(ss, *v);
  }
  else {
    ss.append("null");
  }
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<tuple_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &s, T &&t) {
  using U = typename std::decay_t<T>;
  s.push_back('[');
  constexpr size_t size = std::tuple_size_v<U>;
  foreach_tuple(
      [&s, size](auto &v, auto i) IGUANA__INLINE_LAMBDA {
        to_json_impl<Is_writing_escape>(s, v);

        if (i != size - 1)
          IGUANA_LIKELY { s.push_back(','); }
      },
      std::forward<T>(t));
  s.push_back(']');
}

template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<variant_v<T>, int>>
IGUANA_INLINE void to_json_impl(Stream &s, T &&t) {
  static_assert(!has_duplicate_type_v<std::remove_reference_t<T>>,
                "don't allow same type in std::variant");
  std::visit(
      [&s](auto value) {
        to_json_impl<Is_writing_escape>(s, value);
      },
      t);
}
template <bool Is_writing_escape, typename Field, size_t N>
IGUANA_INLINE void write_buffered_field(json_string_sink &ss,
                                        const std::array<char, N> &key,
                                        const Field &field) {
  if constexpr (string_container_v<Field> &&
                std::is_convertible_v<const Field &, std::string_view>) {
    ss.template write_quoted<Is_writing_escape>(key, field);
  }
  else if constexpr (int_v<Field> && sizeof(Field) <= 8) {
    ss.write_integer(key, field);
  }
  else {
    ss.append(key.data(), key.size());
    to_json_impl<Is_writing_escape>(ss, field);
  }
}
}  // namespace detail
template <bool Is_writing_escape, typename Stream, typename T,
          std::enable_if_t<ylt_refletable_v<T>, int>>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  using namespace detail;
  using U = ylt::reflection::remove_cvref_t<T>;
  constexpr auto Count = ylt::reflection::members_count_v<U>;
  if constexpr (json_string_sink_v<Stream> && Count > 0) {
    auto fields = ylt::reflection::object_to_tuple(t);
    foreach_tuple(
        [&](auto &field, auto index) {
          static_assert(
              !std::is_pointer_v<std::remove_reference_t<decltype(field)>>,
              "don't use raw pointer, please use smart pointer");
          static constexpr auto key =
              make_json_key<U, decltype(index)::value, true>();
          write_buffered_field<Is_writing_escape>(s, key, field);
        },
        fields);
  }
  else {
    s.push_back('{');
    if constexpr (Count > 0) {
      ylt::reflection::for_each(t, [&](auto &field, auto, auto index) {
        static_assert(
            !std::is_pointer_v<std::remove_reference_t<decltype(field)>>,
            "don't use raw pointer, please use smart pointer");
        const auto &key = json_keys<U>[index];
        s.append(key.data(), key.size());
        to_json_impl<Is_writing_escape>(s, field);
      });
    }
  }

  s.push_back('}');
}

template <bool Is_writing_escape = true, typename Stream, typename T,
          std::enable_if_t<non_ylt_refletable_v<T>, int> = 0>
IGUANA_INLINE void to_json(T &&t, Stream &s) {
  using namespace detail;
  to_json_impl<Is_writing_escape>(s, t);
}

// Append using contiguous storage. The value must not borrow from output.
// Custom serializers used here must accept a generic output stream.
template <bool Is_writing_escape = true, typename T>
IGUANA_INLINE void to_json_buffered(T &&value, std::string &output) {
  detail::json_string_sink stream(output);
  to_json<Is_writing_escape>(std::forward<T>(value), stream);
}

template <typename T>
IGUANA_INLINE void to_json_adl(iguana_adl_t *p, T &t, std::string &pb_str) {
  to_json(t, pb_str);
}

}  // namespace iguana
#endif  // SERIALIZE_JSON_HPP
