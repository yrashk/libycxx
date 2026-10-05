// libycxx core: the parts of <ranges> that other headers specialise or use before <ranges> exists in a
// translation unit: enable_view, view_base, view_interface's declaration and from_range_t.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

struct view_base {};

template <class D>
  requires is_class_v<D> && same_as<D, remove_cv_t<D>>
class view_interface;

}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// is-derived-from-view-interface: a class (possibly cv-qualified) with exactly one public base
// view_interface<U>; deduction from a pointer fails for an ambiguous or inaccessible base.
template <class T>
inline constexpr bool is_view_interface = false;
template <class D>
inline constexpr bool is_view_interface<std::ranges::view_interface<D>> = true;
template <class T>
concept derived_from_view_interface =
    std::is_class_v<T> && !is_view_interface<std::remove_cv_t<T>> &&
    requires(T* p) { []<class U>(const volatile std::ranges::view_interface<U>*) {}(p); };
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace ranges {
template <class T>
constexpr bool enable_view = derived_from<T, view_base> || ycxx::detail::derived_from_view_interface<T>;
}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] std {
// [ranges.syn]: the tag of the containers' range constructors.
struct from_range_t {
  explicit from_range_t() = default;
};
inline constexpr from_range_t from_range{};
} // namespace std
