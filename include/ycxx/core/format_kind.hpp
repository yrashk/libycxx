// libycxx core: range_format and format_kind ([format.range.fmtkind]). Separate from the
// formatters (format_ranges.hpp) because <optional> declares format_kind<optional<T>>
// ([optional.syn]) and so needs the primary template, but not the formatting library.
#pragma once

#include <ycxx/core/range_access.hpp>
#include <ycxx/core/tuple_like.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __fmt_is_pair_or_2tuple = false;
template <class _Tp, class _Up>
inline constexpr bool __fmt_is_pair_or_2tuple<std::pair<_Tp, _Up>> = true;
template <class _Tp, class _Up>
inline constexpr bool __fmt_is_pair_or_2tuple<std::tuple<_Tp, _Up>> = true;

template <class _Rp>
inline constexpr bool __fmt_dependent_false = false;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [format.range.fmtkind]
enum class range_format { disabled, map, set, sequence, string, debug_string };

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Rp>
consteval std::range_format __fmt_kind_primary() {
  static_assert(__fmt_dependent_false<_Rp>, "std::format_kind: the primary template is instantiated ([format.range.fmtkind]/1)");
  return std::range_format::disabled;
}
template <class _Rp>
consteval std::range_format __fmt_default_kind() {
  using _Up = std::remove_cvref_t<std::ranges::range_reference_t<_Rp>>;
  if constexpr (__is_same(_Up, _Rp))
    return std::range_format::disabled;
  else if constexpr (requires { typename _Rp::key_type; }) {
    if constexpr (requires { typename _Rp::mapped_type; } && __fmt_is_pair_or_2tuple<_Up>)
      return std::range_format::map;
    else
      return std::range_format::set;
  } else
    return std::range_format::sequence;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Rp>
inline constexpr range_format format_kind = __ycxx::__detail::__fmt_kind_primary<_Rp>();
template <ranges::input_range _Rp>
  requires same_as<_Rp, remove_cvref_t<_Rp>>
inline constexpr range_format format_kind<_Rp> = __ycxx::__detail::__fmt_default_kind<_Rp>();

}} // namespace std
