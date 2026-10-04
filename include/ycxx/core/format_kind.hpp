// libycxx core: range_format and format_kind ([format.range.fmtkind]). Separate from the
// formatters (format_ranges.hpp) because <optional> declares format_kind<optional<T>>
// ([optional.syn]) and so needs the primary template, but not the formatting library.
#pragma once

#include <ycxx/core/range_access.hpp>
#include <ycxx/core/tuple_like.hpp>

namespace ycxx::detail {
template <class T>
inline constexpr bool fmt_is_pair_or_2tuple = false;
template <class T, class U>
inline constexpr bool fmt_is_pair_or_2tuple<std::pair<T, U>> = true;
template <class T, class U>
inline constexpr bool fmt_is_pair_or_2tuple<std::tuple<T, U>> = true;

template <class R>
inline constexpr bool fmt_dependent_false = false;
} // namespace ycxx::detail

namespace std {

// [format.range.fmtkind]
enum class range_format { disabled, map, set, sequence, string, debug_string };

} // namespace std

namespace ycxx::detail {
template <class R>
consteval std::range_format fmt_kind_primary() {
  static_assert(fmt_dependent_false<R>, "std::format_kind: the primary template is instantiated ([format.range.fmtkind]/1)");
  return std::range_format::disabled;
}
template <class R>
consteval std::range_format fmt_default_kind() {
  using U = std::remove_cvref_t<std::ranges::range_reference_t<R>>;
  if constexpr (__is_same(U, R))
    return std::range_format::disabled;
  else if constexpr (requires { typename R::key_type; }) {
    if constexpr (requires { typename R::mapped_type; } && fmt_is_pair_or_2tuple<U>)
      return std::range_format::map;
    else
      return std::range_format::set;
  } else
    return std::range_format::sequence;
}
} // namespace ycxx::detail

namespace std {

template <class R>
inline constexpr range_format format_kind = ycxx::detail::fmt_kind_primary<R>();
template <ranges::input_range R>
  requires same_as<R, remove_cvref_t<R>>
inline constexpr range_format format_kind<R> = ycxx::detail::fmt_default_kind<R>();

} // namespace std
