// libycxx core: views::all, ref_view and owning_view ([range.all]).
#pragma once

#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/ranges_adaptor.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// FUN of [range.ref.view]: non-template functions, so binding through two equally good
// conversion functions is ambiguous (function-template partial ordering would prefer R&).
template <class R>
struct ref_view_fun {
  static void fun(R&) noexcept;
  static void fun(R&&) = delete;
};
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

template <range R>
  requires is_object_v<R>
class ref_view : public view_interface<ref_view<R>> {
  R* r_;

public:
  template <ycxx::detail::different_from<ref_view> T>
    requires convertible_to<T, R&> && requires { ycxx::detail::ref_view_fun<R>::fun(declval<T>()); }
  constexpr ref_view(T&& t) : r_(__builtin_addressof(static_cast<R&>(static_cast<T&&>(t)))) {}

  constexpr R& base() const { return *r_; }
  constexpr iterator_t<R> begin() const { return ranges::begin(*r_); }
  constexpr sentinel_t<R> end() const { return ranges::end(*r_); }
  constexpr bool empty() const
    requires requires { ranges::empty(*r_); }
  {
    return ranges::empty(*r_);
  }
  constexpr auto size() const
    requires sized_range<R>
  {
    return ranges::size(*r_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<R>
  {
    return ranges::reserve_hint(*r_);
  }
  constexpr auto data() const
    requires contiguous_range<R>
  {
    return ranges::data(*r_);
  }
};
template <class R>
ref_view(R&) -> ref_view<R>;
template <class T>
constexpr bool enable_borrowed_range<ref_view<T>> = true;

template <range R>
  requires movable<R> && (!ycxx::detail::is_init_list_v<remove_cvref_t<R>>)
class owning_view : public view_interface<owning_view<R>> {
  R r_ = R();

public:
  owning_view()
    requires default_initializable<R>
  = default;
  constexpr owning_view(R&& t) : r_(std::move(t)) {}
  owning_view(owning_view&&) = default;
  owning_view& operator=(owning_view&&) = default;

  constexpr R& base() & noexcept { return r_; }
  constexpr const R& base() const& noexcept { return r_; }
  constexpr R&& base() && noexcept { return std::move(r_); }
  constexpr const R&& base() const&& noexcept { return std::move(r_); }

  constexpr iterator_t<R> begin() { return ranges::begin(r_); }
  constexpr sentinel_t<R> end() { return ranges::end(r_); }
  constexpr auto begin() const
    requires range<const R>
  {
    return ranges::begin(r_);
  }
  constexpr auto end() const
    requires range<const R>
  {
    return ranges::end(r_);
  }
  constexpr bool empty()
    requires requires { ranges::empty(r_); }
  {
    return ranges::empty(r_);
  }
  constexpr bool empty() const
    requires requires { ranges::empty(r_); }
  {
    return ranges::empty(r_);
  }
  constexpr auto size()
    requires sized_range<R>
  {
    return ranges::size(r_);
  }
  constexpr auto size() const
    requires sized_range<const R>
  {
    return ranges::size(r_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<R>
  {
    return ranges::reserve_hint(r_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const R>
  {
    return ranges::reserve_hint(r_);
  }
  constexpr auto data()
    requires contiguous_range<R>
  {
    return ranges::data(r_);
  }
  constexpr auto data() const
    requires contiguous_range<const R>
  {
    return ranges::data(r_);
  }
};
template <class T>
constexpr bool enable_borrowed_range<owning_view<T>> = enable_borrowed_range<T>;

}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::range_all {
struct fn : std::ranges::range_adaptor_closure<fn> {
  template <class R>
  static consteval bool nothrow() {
    if constexpr (std::ranges::view<std::decay_t<R>>)
      return std::is_nothrow_convertible_v<R, std::decay_t<R>>;
    else if constexpr (requires { std::ranges::ref_view{std::declval<R>()}; })
      return noexcept(std::ranges::ref_view{std::declval<R>()});
    else
      return noexcept(std::ranges::owning_view{std::declval<R>()});
  }
  template <std::ranges::viewable_range R>
  [[nodiscard]] constexpr auto operator()(R&& r) const noexcept(nothrow<R>()) {
    if constexpr (std::ranges::view<std::decay_t<R>>)
      return ::ycxx::detail::decay_copy(static_cast<R&&>(r));
    else if constexpr (requires { std::ranges::ref_view{static_cast<R&&>(r)}; })
      return std::ranges::ref_view{static_cast<R&&>(r)};
    else
      return std::ranges::owning_view{static_cast<R&&>(r)};
  }
};
}} // namespace ycxx::detail::range_all

namespace [[gnu::visibility("hidden")]] std { namespace ranges::views {
inline constexpr ycxx::detail::range_all::fn all{};
template <viewable_range R>
using all_t = decltype(all(declval<R>()));
}} // namespace std::ranges::views

namespace [[gnu::visibility("hidden")]] std {
namespace views = ranges::views;
} // namespace std
