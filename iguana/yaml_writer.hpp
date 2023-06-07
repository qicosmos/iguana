#pragma once

#include "detail/charconv.h"
#include "yaml_util.hpp"

















template <typename Stream, refletable T>
IGUANA_INLINE void to_json(T &&t, Stream &s, size_t min_spaces = 0) {
  for_each(std::forward<T>(t),
           [&t, &s](const auto &v, auto i) IGUANA__INLINE_LAMBDA {
             using M = decltype(iguana_reflect_members(std::forward<T>(t)));
             constexpr auto Idx = decltype(i)::value;
             constexpr auto Count = M::value();
             static_assert(Idx < Count);

             write_json_key(s, i, t);
             s.push_back(':');

             if constexpr (!is_reflection<decltype(v)>::value) {
               render_json_value(s, t.*v);
             } else {
               to_json(t.*v, s);
             }

             if (Idx < Count - 1)
               s.push_back(',');
           });
}