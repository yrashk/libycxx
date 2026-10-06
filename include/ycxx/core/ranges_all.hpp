// libycxx core: views::all, ref_view and owning_view ([range.all]).
#pragma once

#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/ranges_adaptor.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// FUN of [range.ref.view]: non-template functions, so binding through two equally good
// conversion functions is ambiguous (function-template partial ordering would prefer R&).
template <class _Rp>
struct __ref_view_fun {
  static void fun(_Rp&) noexcept;
  static void fun(_Rp&&) = delete;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <range _Rp>
  requires is_object_v<_Rp>
class ref_view : public view_interface<ref_view<_Rp>> {
  _Rp* __r_;

public:
  template <__ycxx::__detail::__different_from<ref_view> _Tp>
    requires convertible_to<_Tp, _Rp&> && requires { __ycxx::__detail::__ref_view_fun<_Rp>::fun(declval<_Tp>()); }
  constexpr ref_view(_Tp&& t) : __r_(__builtin_addressof(static_cast<_Rp&>(static_cast<_Tp&&>(t)))) {}

  constexpr _Rp& base() const { return *__r_; }
  constexpr iterator_t<_Rp> begin() const { return ranges::begin(*__r_); }
  constexpr sentinel_t<_Rp> end() const { return ranges::end(*__r_); }
  constexpr bool empty() const
    requires requires { ranges::empty(*__r_); }
  {
    return ranges::empty(*__r_);
  }
  constexpr auto size() const
    requires sized_range<_Rp>
  {
    return ranges::size(*__r_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<_Rp>
  {
    return ranges::reserve_hint(*__r_);
  }
  constexpr auto data() const
    requires contiguous_range<_Rp>
  {
    return ranges::data(*__r_);
  }
};
template <class _Rp>
ref_view(_Rp&) -> ref_view<_Rp>;
template <class _Tp>
constexpr bool enable_borrowed_range<ref_view<_Tp>> = true;

template <range _Rp>
  requires movable<_Rp> && (!__ycxx::__detail::__is_init_list_v<remove_cvref_t<_Rp>>)
class owning_view : public view_interface<owning_view<_Rp>> {
  _Rp __r_ = _Rp();

public:
  owning_view()
    requires default_initializable<_Rp>
  = default;
  constexpr owning_view(_Rp&& t) : __r_(std::move(t)) {}
  owning_view(owning_view&&) = default;
  owning_view& operator=(owning_view&&) = default;

  constexpr _Rp& base() & noexcept { return __r_; }
  constexpr const _Rp& base() const& noexcept { return __r_; }
  constexpr _Rp&& base() && noexcept { return std::move(__r_); }
  constexpr const _Rp&& base() const&& noexcept { return std::move(__r_); }

  constexpr iterator_t<_Rp> begin() { return ranges::begin(__r_); }
  constexpr sentinel_t<_Rp> end() { return ranges::end(__r_); }
  constexpr auto begin() const
    requires range<const _Rp>
  {
    return ranges::begin(__r_);
  }
  constexpr auto end() const
    requires range<const _Rp>
  {
    return ranges::end(__r_);
  }
  constexpr bool empty()
    requires requires { ranges::empty(__r_); }
  {
    return ranges::empty(__r_);
  }
  constexpr bool empty() const
    requires requires { ranges::empty(__r_); }
  {
    return ranges::empty(__r_);
  }
  constexpr auto size()
    requires sized_range<_Rp>
  {
    return ranges::size(__r_);
  }
  constexpr auto size() const
    requires sized_range<const _Rp>
  {
    return ranges::size(__r_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Rp>
  {
    return ranges::reserve_hint(__r_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Rp>
  {
    return ranges::reserve_hint(__r_);
  }
  constexpr auto data()
    requires contiguous_range<_Rp>
  {
    return ranges::data(__r_);
  }
  constexpr auto data() const
    requires contiguous_range<const _Rp>
  {
    return ranges::data(__r_);
  }
};
template <class _Tp>
constexpr bool enable_borrowed_range<owning_view<_Tp>> = enable_borrowed_range<_Tp>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_all {
struct __fn : std::ranges::range_adaptor_closure<__fn> {
  template <class _Rp>
  static consteval bool nothrow() {
    if constexpr (std::ranges::view<std::decay_t<_Rp>>)
      return std::is_nothrow_convertible_v<_Rp, std::decay_t<_Rp>>;
    else if constexpr (requires { std::ranges::ref_view{std::declval<_Rp>()}; })
      return noexcept(std::ranges::ref_view{std::declval<_Rp>()});
    else
      return noexcept(std::ranges::owning_view{std::declval<_Rp>()});
  }
  template <std::ranges::viewable_range _Rp>
  [[nodiscard]] constexpr auto operator()(_Rp&& r) const noexcept(nothrow<_Rp>()) {
    if constexpr (std::ranges::view<std::decay_t<_Rp>>)
      return ::__ycxx::__detail::__decay_copy(static_cast<_Rp&&>(r));
    else if constexpr (requires { std::ranges::ref_view{static_cast<_Rp&&>(r)}; })
      return std::ranges::ref_view{static_cast<_Rp&&>(r)};
    else
      return std::ranges::owning_view{static_cast<_Rp&&>(r)};
  }
};
}} // namespace __ycxx::__detail::__range_all

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges::views {
inline constexpr __ycxx::__detail::__range_all::__fn all{};
template <viewable_range _Rp>
using all_t = decltype(all(declval<_Rp>()));
}} // namespace std::ranges::views

namespace [[__gnu__::__visibility__("hidden")]] std {
namespace views = ranges::views;
} // namespace std
