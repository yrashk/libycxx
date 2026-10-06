// libycxx core: the range utilities the algorithms need ([range.utility]): the view concept,
// view_interface, subrange and dangling / borrowed_iterator_t / borrowed_subrange_t.
// Reachable through <ranges> and <algorithm>; the views themselves live elsewhere.
#pragma once

#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_init_list_v = false;
template <class _Tp>
inline constexpr bool __is_init_list_v<std::initializer_list<_Tp>> = true;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// [range.view]
template <class _Tp>
concept view = range<_Tp> && movable<_Tp> && enable_view<_Tp>;

template <class _Tp>
concept viewable_range =
    range<_Tp> && ((view<remove_cvref_t<_Tp>> && constructible_from<remove_cvref_t<_Tp>, _Tp>) ||
                 (!view<remove_cvref_t<_Tp>> &&
                  (is_lvalue_reference_v<_Tp> || (movable<remove_reference_t<_Tp>> && !__ycxx::__detail::__is_init_list_v<remove_cvref_t<_Tp>>))));

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Rp>
concept __simple_view = std::ranges::view<_Rp> && std::ranges::range<const _Rp> &&
                      std::same_as<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<const _Rp>> &&
                      std::same_as<std::ranges::sentinel_t<_Rp>, std::ranges::sentinel_t<const _Rp>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// [view.interface]
template <class _Dp>
  requires is_class_v<_Dp> && same_as<_Dp, remove_cv_t<_Dp>>
class view_interface {
  constexpr _Dp& __derived() noexcept { return static_cast<_Dp&>(*this); }
  constexpr const _Dp& __derived() const noexcept { return static_cast<const _Dp&>(*this); }

public:
  constexpr bool empty()
    requires sized_range<_Dp> || forward_range<_Dp>
  {
    if constexpr (sized_range<_Dp>)
      return ranges::size(__derived()) == 0;
    else
      return ranges::begin(__derived()) == ranges::end(__derived());
  }
  constexpr bool empty() const
    requires sized_range<const _Dp> || forward_range<const _Dp>
  {
    if constexpr (sized_range<const _Dp>)
      return ranges::size(__derived()) == 0;
    else
      return ranges::begin(__derived()) == ranges::end(__derived());
  }
  constexpr auto cbegin()
    requires input_range<_Dp>
  {
    return ranges::cbegin(__derived());
  }
  constexpr auto cbegin() const
    requires input_range<const _Dp>
  {
    return ranges::cbegin(__derived());
  }
  constexpr auto cend()
    requires input_range<_Dp>
  {
    return ranges::cend(__derived());
  }
  constexpr auto cend() const
    requires input_range<const _Dp>
  {
    return ranges::cend(__derived());
  }
  constexpr explicit operator bool()
    requires requires { ranges::empty(__derived()); }
  {
    return !ranges::empty(__derived());
  }
  constexpr explicit operator bool() const
    requires requires { ranges::empty(__derived()); }
  {
    return !ranges::empty(__derived());
  }
  constexpr auto data()
    requires contiguous_iterator<iterator_t<_Dp>>
  {
    return std::to_address(ranges::begin(__derived()));
  }
  constexpr auto data() const
    requires range<const _Dp> && contiguous_iterator<iterator_t<const _Dp>>
  {
    return std::to_address(ranges::begin(__derived()));
  }
  constexpr auto size()
    requires forward_range<_Dp> && sized_sentinel_for<sentinel_t<_Dp>, iterator_t<_Dp>>
  {
    return ::__ycxx::__detail::__to_unsigned_like(ranges::end(__derived()) - ranges::begin(__derived()));
  }
  constexpr auto size() const
    requires forward_range<const _Dp> && sized_sentinel_for<sentinel_t<const _Dp>, iterator_t<const _Dp>>
  {
    return ::__ycxx::__detail::__to_unsigned_like(ranges::end(__derived()) - ranges::begin(__derived()));
  }
  constexpr decltype(auto) front()
    requires forward_range<_Dp>
  {
    ::__ycxx::__detail::__precondition(!empty(), "view_interface::front: empty view");
    return *ranges::begin(__derived());
  }
  constexpr decltype(auto) front() const
    requires forward_range<const _Dp>
  {
    ::__ycxx::__detail::__precondition(!empty(), "view_interface::front: empty view");
    return *ranges::begin(__derived());
  }
  constexpr decltype(auto) back()
    requires bidirectional_range<_Dp> && common_range<_Dp>
  {
    ::__ycxx::__detail::__precondition(!empty(), "view_interface::back: empty view");
    return *ranges::prev(ranges::end(__derived()));
  }
  constexpr decltype(auto) back() const
    requires bidirectional_range<const _Dp> && common_range<const _Dp>
  {
    ::__ycxx::__detail::__precondition(!empty(), "view_interface::back: empty view");
    return *ranges::prev(ranges::end(__derived()));
  }
  template <random_access_range _Rp = _Dp>
  constexpr decltype(auto) operator[](range_difference_t<_Rp> n) {
    return ranges::begin(__derived())[n];
  }
  template <random_access_range _Rp = const _Dp>
  constexpr decltype(auto) operator[](range_difference_t<_Rp> n) const {
    return ranges::begin(__derived())[n];
  }
  template <random_access_range _Rp = _Dp>
    requires sized_range<_Rp>
  constexpr decltype(auto) at(range_difference_t<_Rp> n) {
    if (n < 0 || n >= ranges::distance(__derived()))
      ::__ycxx::__detail::__throw_out_of_range("view_interface::at: index out of range");
    return (*this)[n];
  }
  template <random_access_range _Rp = const _Dp>
    requires sized_range<_Rp>
  constexpr decltype(auto) at(range_difference_t<_Rp> n) const {
    if (n < 0 || n >= ranges::distance(__derived()))
      ::__ycxx::__detail::__throw_out_of_range("view_interface::at: index out of range");
    return (*this)[n];
  }
};

// [range.subrange]
enum class subrange_kind : bool { unsized, sized };

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _From, class _To>
concept __uses_nonqualification_pointer_conversion =
    std::is_pointer_v<_From> && std::is_pointer_v<_To> &&
    !std::convertible_to<std::remove_pointer_t<_From> (*)[], std::remove_pointer_t<_To> (*)[]>;
template <class _From, class _To>
concept __convertible_to_non_slicing =
    std::convertible_to<_From, _To> && !__uses_nonqualification_pointer_conversion<std::decay_t<_From>, std::decay_t<_To>>;
template <class _Tp, class _Up, class _Vp>
concept __pair_like_convertible_from = !std::ranges::range<_Tp> && !std::is_reference_v<_Tp> && __pair_like<_Tp> &&
                                     std::constructible_from<_Tp, _Up, _Vp> &&
                                     __convertible_to_non_slicing<_Up, std::tuple_element_t<0, _Tp>> &&
                                     std::convertible_to<_Vp, std::tuple_element_t<1, _Tp>>;

// The stored size of a subrange that is sized only through its constructor argument.
template <class _Dp, bool _Store>
struct __subrange_size {
  constexpr __subrange_size() = default;
  constexpr __subrange_size(_Dp) noexcept {}
};
template <class _Dp>
struct __subrange_size<_Dp, true> {
  _Dp value = 0;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <input_or_output_iterator _Ip, sentinel_for<_Ip> _Sp = _Ip,
          subrange_kind _Kp = sized_sentinel_for<_Sp, _Ip> ? subrange_kind::sized : subrange_kind::unsized>
  requires(_Kp == subrange_kind::sized || !sized_sentinel_for<_Sp, _Ip>)
class subrange : public view_interface<subrange<_Ip, _Sp, _Kp>> {
  static constexpr bool _StoreSize = _Kp == subrange_kind::sized && !sized_sentinel_for<_Sp, _Ip>;
  using size_type = make_unsigned_t<iter_difference_t<_Ip>>;

  [[no_unique_address]] _Ip __begin_ = _Ip();
  [[no_unique_address]] _Sp __end_ = _Sp();
  [[no_unique_address]] __ycxx::__detail::__subrange_size<size_type, _StoreSize> __size_{};

public:
  subrange()
    requires default_initializable<_Ip>
  = default;

  template <__ycxx::__detail::__convertible_to_non_slicing<_Ip> _It>
  constexpr subrange(_It i, _Sp s)
    requires(!_StoreSize)
      : __begin_(std::move(i)), __end_(std::move(s)) {}

  template <__ycxx::__detail::__convertible_to_non_slicing<_Ip> _It>
  constexpr subrange(_It i, _Sp s, size_type n)
    requires(_Kp == subrange_kind::sized)
      : __begin_(std::move(i)), __end_(std::move(s)) {
    if constexpr (_StoreSize)
      __size_.value = n;
  }

  template <__ycxx::__detail::__different_from<subrange> _Rp>
    requires borrowed_range<_Rp> && __ycxx::__detail::__convertible_to_non_slicing<iterator_t<_Rp>, _Ip> &&
             convertible_to<sentinel_t<_Rp>, _Sp>
  constexpr subrange(_Rp&& r)
    requires(!_StoreSize)
      : __begin_(ranges::begin(r)), __end_(ranges::end(r)) {}
  // [range.subrange.ctor]/6: subrange(r, ranges::size(r)), so the size is taken before
  // ranges::begin(r) (LWG 3286: size may not be valid after begin on an input range).
  template <__ycxx::__detail::__different_from<subrange> _Rp>
    requires borrowed_range<_Rp> && __ycxx::__detail::__convertible_to_non_slicing<iterator_t<_Rp>, _Ip> &&
             convertible_to<sentinel_t<_Rp>, _Sp>
  constexpr subrange(_Rp&& r)
    requires(_StoreSize && sized_range<_Rp>)
      : subrange(r, static_cast<size_type>(ranges::size(r))) {}

  template <borrowed_range _Rp>
    requires __ycxx::__detail::__convertible_to_non_slicing<iterator_t<_Rp>, _Ip> && convertible_to<sentinel_t<_Rp>, _Sp>
  constexpr subrange(_Rp&& r, size_type n)
    requires(_Kp == subrange_kind::sized)
      : subrange{ranges::begin(r), ranges::end(r), n} {}

  template <__ycxx::__detail::__different_from<subrange> _PairLike>
    requires __ycxx::__detail::__pair_like_convertible_from<_PairLike, const _Ip&, const _Sp&>
  constexpr operator _PairLike() const {
    return _PairLike(__begin_, __end_);
  }

  constexpr _Ip begin() const
    requires copyable<_Ip>
  {
    return __begin_;
  }
  [[nodiscard]] constexpr _Ip begin()
    requires(!copyable<_Ip>)
  {
    return std::move(__begin_);
  }
  constexpr _Sp end() const { return __end_; }
  constexpr bool empty() const { return __begin_ == __end_; }
  constexpr size_type size() const
    requires(_Kp == subrange_kind::sized)
  {
    if constexpr (_StoreSize)
      return __size_.value;
    else
      return ::__ycxx::__detail::__to_unsigned_like(__end_ - __begin_);
  }

  [[nodiscard]] constexpr subrange next(iter_difference_t<_Ip> n = 1) const&
    requires forward_iterator<_Ip>
  {
    auto __tmp = *this;
    __tmp.advance(n);
    return __tmp;
  }
  [[nodiscard]] constexpr subrange next(iter_difference_t<_Ip> n = 1) && {
    advance(n);
    return std::move(*this);
  }
  [[nodiscard]] constexpr subrange prev(iter_difference_t<_Ip> n = 1) const
    requires bidirectional_iterator<_Ip>
  {
    auto __tmp = *this;
    __tmp.advance(-n);
    return __tmp;
  }
  constexpr subrange& advance(iter_difference_t<_Ip> n) {
    if constexpr (bidirectional_iterator<_Ip>) {
      if (n < 0) {
        ranges::advance(__begin_, n);
        if constexpr (_StoreSize)
          __size_.value += ::__ycxx::__detail::__to_unsigned_like(-n);
        return *this;
      }
    }
    auto d = n - ranges::advance(__begin_, n, __end_);
    if constexpr (_StoreSize)
      __size_.value -= ::__ycxx::__detail::__to_unsigned_like(d);
    return *this;
  }
};

template <input_or_output_iterator _Ip, sentinel_for<_Ip> _Sp>
subrange(_Ip, _Sp) -> subrange<_Ip, _Sp>;
template <input_or_output_iterator _Ip, sentinel_for<_Ip> _Sp>
subrange(_Ip, _Sp, make_unsigned_t<iter_difference_t<_Ip>>) -> subrange<_Ip, _Sp, subrange_kind::sized>;
template <borrowed_range _Rp>
subrange(_Rp&&) -> subrange<iterator_t<_Rp>, sentinel_t<_Rp>,
                          (sized_range<_Rp> || sized_sentinel_for<sentinel_t<_Rp>, iterator_t<_Rp>>) ? subrange_kind::sized
                                                                                               : subrange_kind::unsized>;
template <borrowed_range _Rp>
subrange(_Rp&&, make_unsigned_t<range_difference_t<_Rp>>) -> subrange<iterator_t<_Rp>, sentinel_t<_Rp>, subrange_kind::sized>;

template <size_t _Np, class _Ip, class _Sp, subrange_kind _Kp>
  requires((_Np == 0 && copyable<_Ip>) || _Np == 1)
constexpr auto get(const subrange<_Ip, _Sp, _Kp>& r) {
  if constexpr (_Np == 0)
    return r.begin();
  else
    return r.end();
}
template <size_t _Np, class _Ip, class _Sp, subrange_kind _Kp>
  requires(_Np < 2)
constexpr auto get(subrange<_Ip, _Sp, _Kp>&& r) {
  if constexpr (_Np == 0)
    return r.begin();
  else
    return r.end();
}

template <class _Ip, class _Sp, subrange_kind _Kp>
constexpr bool enable_borrowed_range<subrange<_Ip, _Sp, _Kp>> = true;

// [range.dangling]
struct dangling {
  constexpr dangling() noexcept = default;
  template <class... _Args>
  constexpr dangling(_Args&&...) noexcept {}
};

template <range _Rp>
using borrowed_iterator_t = conditional_t<borrowed_range<_Rp>, iterator_t<_Rp>, dangling>;
template <range _Rp>
using borrowed_subrange_t = conditional_t<borrowed_range<_Rp>, subrange<iterator_t<_Rp>>, dangling>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {
using ranges::get;

template <class _Ip, class _Sp, ranges::subrange_kind _Kp>
struct tuple_size<ranges::subrange<_Ip, _Sp, _Kp>> : integral_constant<size_t, 2> {};
template <class _Ip, class _Sp, ranges::subrange_kind _Kp>
struct tuple_element<0, ranges::subrange<_Ip, _Sp, _Kp>> {
  using type = _Ip;
};
template <class _Ip, class _Sp, ranges::subrange_kind _Kp>
struct tuple_element<1, ranges::subrange<_Ip, _Sp, _Kp>> {
  using type = _Sp;
};
template <class _Ip, class _Sp, ranges::subrange_kind _Kp>
struct tuple_element<0, const ranges::subrange<_Ip, _Sp, _Kp>> {
  using type = _Ip;
};
template <class _Ip, class _Sp, ranges::subrange_kind _Kp>
struct tuple_element<1, const ranges::subrange<_Ip, _Sp, _Kp>> {
  using type = _Sp;
};
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Ip, class _Sp, std::ranges::subrange_kind _Kp>
inline constexpr bool __is_tuple_like_impl<std::ranges::subrange<_Ip, _Sp, _Kp>> = true;
// Excluded from pair's and tuple's pair-like/tuple-like constructors and from the pair-like
// uses_allocator_construction_args overload (pair.hpp).
template <class _Ip, class _Sp, std::ranges::subrange_kind _Kp>
inline constexpr bool __is_subrange<std::ranges::subrange<_Ip, _Sp, _Kp>> = true;
}} // namespace __ycxx::__detail
