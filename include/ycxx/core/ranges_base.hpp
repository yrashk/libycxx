// libycxx core: the parts of <ranges> that other headers specialise or use before <ranges> exists in a
// translation unit: enable_view, view_base, view_interface's declaration and from_range_t.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace std::ranges {

struct view_base {};

template <class D>
  requires is_class_v<D> && same_as<D, remove_cv_t<D>>
class view_interface;

} // namespace std::ranges

namespace ycxx::detail {
template <class T>
concept derived_from_view_interface = requires(const T& t) { []<class U>(const std::ranges::view_interface<U>&) {}(t); };
} // namespace ycxx::detail

namespace std::ranges {
template <class T>
constexpr bool enable_view = derived_from<T, view_base> || ycxx::detail::derived_from_view_interface<T>;
} // namespace std::ranges

namespace std {
// [ranges.syn]: the tag of the containers' range constructors.
struct from_range_t {
  explicit from_range_t() = default;
};
inline constexpr from_range_t from_range{};
} // namespace std
