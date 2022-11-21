//
// Created by Qiyu on 17-6-5.
//

#ifndef SERIALIZE_JSON_HPP
#define SERIALIZE_JSON_HPP
#include "reflection.hpp"
#include <dragonbox/dragonbox_to_chars.h>
#include <math.h>

namespace iguana {
template <typename InputIt, typename T, typename F>
T join(InputIt first, InputIt last, const T &delim, const F &f) {
  if (first == last)
    return T();

  T t = f(*first++);
  while (first != last) {
    t += delim;
    t += f(*first++);
  }
  return t;
}

template <typename Stream, typename InputIt, typename T, typename F>
void join(Stream &ss, InputIt first, InputIt last, const T &delim, const F &f) {
  if (first == last)
    return;

  f(*first++);
  while (first != last) {
    ss.put(delim);
    f(*first++);
  }
}

template <typename Stream> void render_json_value(Stream &ss, std::nullptr_t) {
  ss.write("null");
}

template <typename Stream> void render_json_value(Stream &ss, bool b) {
  ss.write(b ? "true" : "false");
};

template <typename Stream, typename T>
std::enable_if_t<!std::is_floating_point<T>::value &&
                 (std::is_integral<T>::value || std::is_unsigned<T>::value ||
                  std::is_signed<T>::value)>
render_json_value(Stream &ss, T value) {
  char temp[20];
  auto p = itoa_fwd(value, temp);
  ss.write(temp, p - temp);
}

template <typename Stream> void render_json_value(Stream &ss, int64_t value) {
  char temp[65];
  auto p = xtoa(value, temp, 10, 1);
  ss.write(temp, p - temp);
}

template <typename Stream> void render_json_value(Stream &ss, uint64_t value) {
  char temp[65];
  auto p = xtoa(value, temp, 10, 0);
  ss.write(temp, p - temp);
}

template <typename Stream, typename T>
std::enable_if_t<std::is_floating_point<T>::value> render_json_value(Stream &ss,
                                                                     T value) {
  char temp[40];
  const auto end = jkj::dragonbox::to_chars(value, temp);
  const auto n = std::distance(temp, end);
  ss.write(temp, n);
}

template <typename Stream>
void render_json_value(Stream &ss, const std::string &s) {
  ss.write_str(s.c_str(), s.size());
}

template <typename Stream>
void render_json_value(Stream &ss, const char *s, size_t size) {
  ss.write_str(s, size);
}

template <typename Stream, typename T>
std::enable_if_t<std::is_arithmetic<T>::value> render_key(Stream &ss, T t) {
  ss.put('"');
  render_json_value(ss, t);
  ss.put('"');
}

template <typename Stream> void render_key(Stream &ss, const std::string &s) {
  render_json_value(ss, s);
}

template <typename Stream, typename T>
constexpr auto to_json(Stream &ss, T &&t)
    -> std::enable_if_t<is_reflection<T>::value>;

template <typename Stream, typename T>
auto render_json_value(Stream &ss, T &&t)
    -> std::enable_if_t<is_reflection<T>::value> {
  to_json(ss, std::forward<T>(t));
}

template <typename Stream, typename T>
std::enable_if_t<std::is_enum<T>::value> render_json_value(Stream &ss, T val) {
  render_json_value(ss, (std::underlying_type_t<T> &)val);
}

template <typename Stream, typename T>
void render_array(Stream &ss, const T &v) {
  ss.put('[');
  join(ss, std::begin(v), std::end(v), ',',
       [&ss](const auto &jsv) { render_json_value(ss, jsv); });
  ss.put(']');
}

template <typename Stream, typename T, size_t N>
void render_json_value(Stream &ss, const T (&v)[N]) {
  render_array(ss, v);
}

template <typename Stream, typename T, size_t N>
void render_json_value(Stream &ss, const std::array<T, N> &v) {
  render_array(ss, v);
}

template <typename Stream, typename T>
std::enable_if_t<is_associat_container<T>::value>
render_json_value(Stream &ss, const T &o) {
  ss.put('{');
  join(ss, o.cbegin(), o.cend(), ',', [&ss](const auto &jsv) {
    render_key(ss, jsv.first);
    ss.put(':');
    render_json_value(ss, jsv.second);
  });
  ss.put('}');
}

template <typename Stream, typename T>
std::enable_if_t<is_sequence_container<T>::value>
render_json_value(Stream &ss, const T &v) {
  ss.put('[');
  join(ss, v.cbegin(), v.cend(), ',',
       [&ss](const auto &jsv) { render_json_value(ss, jsv); });
  ss.put(']');
}

constexpr auto write_json_key = [](auto &s, auto i, auto &t) {
  s.put('"');
  auto name =
      get_name<decltype(t),
               decltype(i)::value>(); // will be replaced by string_view later
  s.write(name.data(), name.size());
  s.put('"');
};

template <typename Stream, typename T>
constexpr auto to_json(Stream &s, T &&v)
    -> std::enable_if_t<is_sequence_container<std::decay_t<T>>::value> {
  using U = typename std::decay_t<T>::value_type;
  s.put('[');
  const size_t size = v.size();
  for (size_t i = 0; i < size; i++) {
    if constexpr (is_reflection_v<U>) {
      to_json(s, v[i]);
    } else {
      render_json_value(s, v[i]);
    }

    if (i != size - 1)
      s.put(',');
  }
  s.put(']');
}

template <typename Stream, typename T>
constexpr auto to_json(Stream &s, T &&t)
    -> std::enable_if_t<is_tuple<std::decay_t<T>>::value> {
  using U = typename std::decay_t<T>;
  s.put('[');
  const size_t size = std::tuple_size_v<U>;
  for_each(std::forward<T>(t), [&s, size](auto &v, auto i) {
    render_json_value(s, v);

    if (i != size - 1)
      s.put(',');
  });
  s.put(']');
}

template <typename Stream, typename T>
std::enable_if_t<is_tuple<std::decay_t<T>>::value>
render_json_value(Stream &ss, const T &v) {
  to_json(ss, v);
}

template <typename Stream, typename T>
constexpr auto to_json(Stream &s, T &&t)
    -> std::enable_if_t<is_reflection<T>::value> {
  s.put('{');
  for_each(std::forward<T>(t), [&t, &s](const auto &v, auto i) {
    using M = decltype(iguana_reflect_members(std::forward<T>(t)));
    constexpr auto Idx = decltype(i)::value;
    constexpr auto Count = M::value();
    static_assert(Idx < Count);

    write_json_key(s, i, t);
    s.put(':');

    if constexpr (!is_reflection<decltype(v)>::value) {
      render_json_value(s, t.*v);
    } else {
      to_json(s, t.*v);
    }

    if (Idx < Count - 1)
      s.put(',');
  });
  s.put('}');
}

} // namespace iguana
#endif // SERIALIZE_JSON_HPP
