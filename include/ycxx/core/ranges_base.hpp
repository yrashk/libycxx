// libycxx core: the parts of <ranges> that other headers specialise or use before <ranges> exists in a
// translation unit: enable_view, view_base, view_interface's declaration and from_range_t.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

struct view_base {};

template <class _Dp>
  requires is_class_v<_Dp> && same_as<_Dp, remove_cv_t<_Dp>>
class view_interface;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// is-derived-from-view-interface: a class (possibly cv-qualified) with exactly one public base
// view_interface<U>; deduction from a pointer fails for an ambiguous or inaccessible base.
template <class _Tp>
inline constexpr bool __is_view_interface = false;
template <class _Dp>
inline constexpr bool __is_view_interface<std::ranges::view_interface<_Dp>> = true;
template <class _Tp>
concept __derived_from_view_interface =
    std::is_class_v<_Tp> && !__is_view_interface<std::remove_cv_t<_Tp>> &&
    requires(_Tp* p) { []<class _Up>(const volatile std::ranges::view_interface<_Up>*) {}(p); };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _Tp>
constexpr bool enable_view = derived_from<_Tp, view_base> || __ycxx::__detail::__derived_from_view_interface<_Tp>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {
// [ranges.syn]: the tag of the containers' range constructors.
struct from_range_t {
  explicit from_range_t() = default;
};
inline constexpr from_range_t from_range{};
} // namespace std
