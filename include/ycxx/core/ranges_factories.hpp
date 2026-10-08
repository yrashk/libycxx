// libycxx core: the range factories ([range.factories]): empty_view, single_view, iota_view
// (views::iota, views::indices), repeat_view and basic_istream_view.
#pragma once

#include <ycxx/core/ranges_adaptor.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/char_traits.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

// [range.empty]
template <class _Tp>
  requires is_object_v<_Tp>
class empty_view : public view_interface<empty_view<_Tp>> {
public:
  static constexpr _Tp* begin() noexcept { return nullptr; }
  static constexpr _Tp* end() noexcept { return nullptr; }
  static constexpr _Tp* data() noexcept { return nullptr; }
  static constexpr size_t size() noexcept { return 0; }
  static constexpr bool empty() noexcept { return true; }
};
template <class _Tp>
constexpr bool enable_borrowed_range<empty_view<_Tp>> = true;

namespace views {
template <class _Tp>
constexpr empty_view<_Tp> empty{};
} // namespace views

// [range.single]
template <move_constructible _Tp>
  requires is_object_v<_Tp>
class single_view : public view_interface<single_view<_Tp>> {
  __ycxx::__detail::__movable_box<_Tp> __value_;

public:
  // The constructors are noexcept when constructing T is (a permitted strengthening).
  single_view()
    requires default_initializable<_Tp>
  = default;
  constexpr explicit single_view(const _Tp& t) noexcept(is_nothrow_copy_constructible_v<_Tp>)
    requires copy_constructible<_Tp>
      : __value_(in_place, t) {}
  constexpr explicit single_view(_Tp&& t) noexcept(is_nothrow_move_constructible_v<_Tp>) : __value_(in_place, std::move(t)) {}
  template <class... _Args>
    requires constructible_from<_Tp, _Args...>
  constexpr explicit single_view(in_place_t, _Args&&... __args) noexcept(is_nothrow_constructible_v<_Tp, _Args...>)
      : __value_(in_place, static_cast<_Args&&>(__args)...) {}

  constexpr _Tp* begin() noexcept { return data(); }
  constexpr const _Tp* begin() const noexcept { return data(); }
  constexpr _Tp* end() noexcept { return data() + 1; }
  constexpr const _Tp* end() const noexcept { return data() + 1; }
  static constexpr bool empty() noexcept { return false; }
  static constexpr size_t size() noexcept { return 1; }
  constexpr _Tp* data() noexcept { return __value_.operator->(); }
  constexpr const _Tp* data() const noexcept { return __value_.operator->(); }
};
template <class _Tp>
single_view(_Tp) -> single_view<_Tp>;

}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// ---- [range.iota.view] -------------------------------------------------------------------------
// IOTA-DIFF-T(W): a signed type wider than an integral W; int128 serves the 64-bit types (and
// itself, as the signed-integer-like type of width not less than W).
template <class _Wp>
consteval auto __iota_diff() {
  if constexpr (!std::is_integral_v<_Wp> || sizeof(std::iter_difference_t<_Wp>) > sizeof(_Wp))
    return std::type_identity<std::iter_difference_t<_Wp>>{};
  else if constexpr (sizeof(signed char) > sizeof(_Wp))
    return std::type_identity<signed char>{};
  else if constexpr (sizeof(short) > sizeof(_Wp))
    return std::type_identity<short>{};
  else if constexpr (sizeof(int) > sizeof(_Wp))
    return std::type_identity<int>{};
  else if constexpr (sizeof(long long) > sizeof(_Wp))
    return std::type_identity<long long>{};
  else if constexpr (__cfg::__has_int128)
    return std::type_identity<__y_int128>{};
  else
    return std::type_identity<long long>{};
}
template <class _Wp>
using __iota_diff_t = typename decltype(::__ycxx::__detail::__iota_diff<_Wp>())::type;

template <class _Ip>
concept __decrementable = std::incrementable<_Ip> && requires(_Ip i) {
  { --i } -> std::same_as<_Ip&>;
  { i-- } -> std::same_as<_Ip>;
};

template <class _Ip>
concept __advanceable = __decrementable<_Ip> && std::totally_ordered<_Ip> && requires(_Ip i, const _Ip __j, const __iota_diff_t<_Ip> n) {
  { i += n } -> std::same_as<_Ip&>;
  { i -= n } -> std::same_as<_Ip&>;
  _Ip(__j + n);
  _Ip(n + __j);
  _Ip(__j - n);
  { __j - __j } -> std::convertible_to<__iota_diff_t<_Ip>>;
};

template <class _Wp>
consteval auto __iota_concept() {
  if constexpr (__advanceable<_Wp>)
    return std::random_access_iterator_tag{};
  else if constexpr (__decrementable<_Wp>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (std::incrementable<_Wp>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}

template <class _Wp>
struct __iota_category {};
template <class _Wp>
  requires std::incrementable<_Wp> && std::is_integral_v<__iota_diff_t<_Wp>>
struct __iota_category<_Wp> {
  using iterator_category = std::input_iterator_tag;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

template <weakly_incrementable _Wp, semiregular _Bound = unreachable_sentinel_t>
  requires __ycxx::__detail::__weakly_equality_comparable_with<_Wp, _Bound> && copyable<_Wp>
class iota_view : public view_interface<iota_view<_Wp, _Bound>> {
  struct sentinel;

  struct iterator : __ycxx::__detail::__iota_category<_Wp> {
  private:
    friend iota_view;
    _Wp __value_ = _Wp();
    constexpr explicit iterator(_Wp value) : __value_(value) {}

  public:
    using iterator_concept = decltype(__ycxx::__detail::__iota_concept<_Wp>());
    using value_type = _Wp;
    using difference_type = __ycxx::__detail::__iota_diff_t<_Wp>;

    iterator()
      requires default_initializable<_Wp>
    = default;

    constexpr _Wp operator*() const noexcept(is_nothrow_copy_constructible_v<_Wp>) { return __value_; }
    constexpr iterator& operator++() {
      ++__value_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires incrementable<_Wp>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires __ycxx::__detail::__decrementable<_Wp>
    {
      --__value_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ycxx::__detail::__decrementable<_Wp>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      if constexpr (__ycxx::__detail::__integer_like<_Wp> && !__ycxx::__detail::__signed_integer_like<_Wp>) {
        if (n >= difference_type(0))
          __value_ += static_cast<_Wp>(n);
        else
          __value_ -= static_cast<_Wp>(-n);
      } else {
        __value_ += n;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      if constexpr (__ycxx::__detail::__integer_like<_Wp> && !__ycxx::__detail::__signed_integer_like<_Wp>) {
        if (n >= difference_type(0))
          __value_ -= static_cast<_Wp>(n);
        else
          __value_ += static_cast<_Wp>(-n);
      } else {
        __value_ -= n;
      }
      return *this;
    }
    constexpr _Wp operator[](difference_type n) const
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      return _Wp(__value_ + n);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires equality_comparable<_Wp>
    {
      return __x.__value_ == y.__value_;
    }
    friend constexpr bool operator<(const iterator& __x, const iterator& y)
      requires totally_ordered<_Wp>
    {
      return __x.__value_ < y.__value_;
    }
    friend constexpr bool operator>(const iterator& __x, const iterator& y)
      requires totally_ordered<_Wp>
    {
      return y < __x;
    }
    friend constexpr bool operator<=(const iterator& __x, const iterator& y)
      requires totally_ordered<_Wp>
    {
      return !(y < __x);
    }
    friend constexpr bool operator>=(const iterator& __x, const iterator& y)
      requires totally_ordered<_Wp>
    {
      return !(__x < y);
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires totally_ordered<_Wp> && three_way_comparable<_Wp>
    {
      return __x.__value_ <=> y.__value_;
    }
    friend constexpr iterator operator+(iterator i, difference_type n)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      i += n;
      return i;
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      return i + n;
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      i -= n;
      return i;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__advanceable<_Wp>
    {
      using _Dp = difference_type;
      if constexpr (__ycxx::__detail::__integer_like<_Wp>) {
        if constexpr (__ycxx::__detail::__signed_integer_like<_Wp>)
          return _Dp(_Dp(__x.__value_) - _Dp(y.__value_));
        else
          return (y.__value_ > __x.__value_) ? _Dp(-_Dp(y.__value_ - __x.__value_)) : _Dp(__x.__value_ - y.__value_);
      } else {
        return __x.__value_ - y.__value_;
      }
    }
  };

private:
  struct sentinel {
  private:
    friend iota_view;
    _Bound __bound_ = _Bound();
    constexpr explicit sentinel(_Bound __y_bound) : __bound_(__y_bound) {}

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator& __x, const sentinel& y) { return __x.__value_ == y.__bound_; }
    friend constexpr iter_difference_t<_Wp> operator-(const iterator& __x, const sentinel& y)
      requires sized_sentinel_for<_Bound, _Wp>
    {
      return __x.__value_ - y.__bound_;
    }
    friend constexpr iter_difference_t<_Wp> operator-(const sentinel& __x, const iterator& y)
      requires sized_sentinel_for<_Bound, _Wp>
    {
      return -(y - __x);
    }
  };

  using __last_type =
      conditional_t<same_as<_Wp, _Bound>, iterator, conditional_t<same_as<_Bound, unreachable_sentinel_t>, _Bound, sentinel>>;

  [[no_unique_address]] _Wp __value_ = _Wp();
  [[no_unique_address]] _Bound __bound_ = _Bound();

public:
  iota_view()
    requires default_initializable<_Wp>
  = default;
  // The constructors are noexcept when copying W and Bound is (a permitted strengthening).
  constexpr explicit iota_view(_Wp value) noexcept(is_nothrow_copy_constructible_v<_Wp>) : __value_(value) {
    if constexpr (totally_ordered_with<_Wp, _Bound>)
      ::__ycxx::__detail::__precondition(bool(__value_ <= __bound_), "iota_view: the bound is not reachable from the value");
  }
  constexpr explicit iota_view(type_identity_t<_Wp> value, type_identity_t<_Bound> __y_bound) noexcept(
      is_nothrow_copy_constructible_v<_Wp> && is_nothrow_copy_constructible_v<_Bound>)
      : __value_(value), __bound_(__y_bound) {
    if constexpr (totally_ordered_with<_Wp, _Bound>)
      ::__ycxx::__detail::__precondition(bool(__value_ <= __bound_), "iota_view: the bound is not reachable from the value");
  }
  constexpr explicit iota_view(iterator first, __last_type last)
      : iota_view(first.__value_, [&]() -> _Bound {
          if constexpr (same_as<_Wp, _Bound>)
            return last.__value_;
          else if constexpr (same_as<_Bound, unreachable_sentinel_t>)
            return last;
          else
            return last.__bound_;
        }()) {}

  constexpr iterator begin() const { return iterator{__value_}; }
  constexpr auto end() const {
    if constexpr (same_as<_Bound, unreachable_sentinel_t>)
      return unreachable_sentinel;
    else
      return sentinel{__bound_};
  }
  constexpr iterator end() const
    requires same_as<_Wp, _Bound>
  {
    return iterator{__bound_};
  }
  constexpr bool empty() const { return __value_ == __bound_; }
  constexpr auto size() const
    requires(same_as<_Wp, _Bound> && __ycxx::__detail::__advanceable<_Wp>) ||
            (__ycxx::__detail::__integer_like<_Wp> && __ycxx::__detail::__integer_like<_Bound>) || sized_sentinel_for<_Bound, _Wp>
  {
    using __ycxx::__detail::__to_unsigned_like;
    if constexpr (__ycxx::__detail::__integer_like<_Wp> && __ycxx::__detail::__integer_like<_Bound>) {
      // The value of the specified expression, computed without negating a minimum value: both
      // operands converted (sign-extended) to a common unsigned type, whose modular difference
      // is the exact size. The result type is the specified one, made unsigned where integral
      // promotion turned it signed (narrow W).
      using _R0 = decltype(__to_unsigned_like(__bound_) - __to_unsigned_like(__value_));
      using _Rp = conditional_t<signed_integral<_R0>, make_unsigned_t<_R0>, _R0>;
      using _UC = make_unsigned_t<common_type_t<_Wp, _Bound>>;
      return static_cast<_Rp>(static_cast<_UC>(static_cast<_UC>(__bound_) - static_cast<_UC>(__value_)));
    } else {
      return __to_unsigned_like(__bound_ - __value_);
    }
  }
};

template <class _Wp, class _Bound>
  requires(!__ycxx::__detail::__integer_like<_Wp> || !__ycxx::__detail::__integer_like<_Bound> ||
           (__ycxx::__detail::__signed_integer_like<_Wp> == __ycxx::__detail::__signed_integer_like<_Bound>))
iota_view(_Wp, _Bound) -> iota_view<_Wp, _Bound>;

template <class _Wp, class _Bound>
constexpr bool enable_borrowed_range<iota_view<_Wp, _Bound>> = true;

}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// ---- [range.repeat.view] -----------------------------------------------------------------------
template <class _Tp>
concept __integer_like_with_usable_difference_type =
    __signed_integer_like<_Tp> || (__integer_like<_Tp> && std::weakly_incrementable<_Tp>);

template <class _Tp>
inline constexpr bool __is_iota_view = false;
struct __repeat_access;
template <class _Wp, class _Bp>
inline constexpr bool __is_iota_view<std::ranges::iota_view<_Wp, _Bp>> = true;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

template <move_constructible _Tp, semiregular _Bound = unreachable_sentinel_t>
  requires(is_object_v<_Tp> && same_as<_Tp, remove_cv_t<_Tp>> &&
           (__ycxx::__detail::__integer_like_with_usable_difference_type<_Bound> || same_as<_Bound, unreachable_sentinel_t>))
class repeat_view : public view_interface<repeat_view<_Tp, _Bound>> {
  using index_type = conditional_t<same_as<_Bound, unreachable_sentinel_t>, ptrdiff_t, _Bound>;
  static constexpr bool __bounded = !same_as<_Bound, unreachable_sentinel_t>;

  // views::take / views::drop read the value ([range.take.overview]/2.5).
  friend struct __ycxx::__detail::__repeat_access;

  [[no_unique_address]] __ycxx::__detail::__movable_box<_Tp> __value_;
  [[no_unique_address]] _Bound __bound_ = _Bound();

  class iterator {
    friend repeat_view;
    const _Tp* __value_ = nullptr;
    index_type __current_ = index_type();

    constexpr explicit iterator(const _Tp* value, index_type b = index_type()) : __value_(value), __current_(b) {
      if constexpr (__bounded)
        ::__ycxx::__detail::__precondition(b >= 0, "repeat_view: negative bound");
    }

  public:
    using iterator_concept = random_access_iterator_tag;
    using iterator_category = random_access_iterator_tag;
    using value_type = _Tp;
    using difference_type =
        conditional_t<__ycxx::__detail::__signed_integer_like<index_type>, index_type, __ycxx::__detail::__iota_diff_t<index_type>>;

    iterator() = default;
    constexpr const _Tp& operator*() const noexcept { return *__value_; }
    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr iterator operator++(int) {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--() {
      if constexpr (__bounded)
        ::__ycxx::__detail::__precondition(__current_ > 0, "repeat_view::iterator: decrement before the start");
      --__current_;
      return *this;
    }
    constexpr iterator operator--(int) {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type n) {
      if constexpr (__bounded)
        ::__ycxx::__detail::__precondition(__current_ + n >= 0, "repeat_view::iterator: advance before the start");
      __current_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n) {
      if constexpr (__bounded)
        ::__ycxx::__detail::__precondition(__current_ - n >= 0, "repeat_view::iterator: advance before the start");
      __current_ -= n;
      return *this;
    }
    constexpr const _Tp& operator[](difference_type n) const noexcept { return *(*this + n); }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) { return __x.__current_ == y.__current_; }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y) { return __x.__current_ <=> y.__current_; }
    friend constexpr iterator operator+(iterator i, difference_type n) {
      i += n;
      return i;
    }
    friend constexpr iterator operator+(difference_type n, iterator i) {
      i += n;
      return i;
    }
    friend constexpr iterator operator-(iterator i, difference_type n) {
      i -= n;
      return i;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y) {
      return static_cast<difference_type>(__x.__current_) - static_cast<difference_type>(y.__current_);
    }
  };

public:
  repeat_view()
    requires default_initializable<_Tp>
  = default;
  constexpr explicit repeat_view(const _Tp& value, _Bound __y_bound = _Bound())
    requires copy_constructible<_Tp>
      : __value_(in_place, value), __bound_(__y_bound) {
    if constexpr (__bounded)
      ::__ycxx::__detail::__precondition(__y_bound >= 0, "repeat_view: negative bound");
  }
  constexpr explicit repeat_view(_Tp&& value, _Bound __y_bound = _Bound()) : __value_(in_place, std::move(value)), __bound_(__y_bound) {
    if constexpr (__bounded)
      ::__ycxx::__detail::__precondition(__y_bound >= 0, "repeat_view: negative bound");
  }
  template <class... _TArgs, class... _BoundArgs>
    requires constructible_from<_Tp, _TArgs...> && constructible_from<_Bound, _BoundArgs...>
  constexpr explicit repeat_view(piecewise_construct_t, tuple<_TArgs...> __value_args,
                                 tuple<_BoundArgs...> __bound_args = tuple<>{})
      : __value_(in_place, std::make_from_tuple<_Tp>(std::move(__value_args))),
        __bound_(std::make_from_tuple<_Bound>(std::move(__bound_args))) {
    if constexpr (__bounded)
      ::__ycxx::__detail::__precondition(__bound_ >= 0, "repeat_view: negative bound");
  }

  constexpr iterator begin() const { return iterator(__builtin_addressof(*__value_)); }
  constexpr iterator end() const
    requires(!same_as<_Bound, unreachable_sentinel_t>)
  {
    return iterator(__builtin_addressof(*__value_), __bound_);
  }
  constexpr unreachable_sentinel_t end() const noexcept { return unreachable_sentinel; }
  constexpr auto size() const
    requires(!same_as<_Bound, unreachable_sentinel_t>)
  {
    return ::__ycxx::__detail::__to_unsigned_like(__bound_);
  }
};

template <class _Tp, class _Bound = unreachable_sentinel_t>
repeat_view(_Tp, _Bound = _Bound()) -> repeat_view<_Tp, _Bound>;

}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_repeat_view = false;
template <class _Tp, class _Bp>
inline constexpr bool __is_repeat_view<std::ranges::repeat_view<_Tp, _Bp>> = true;

// The stored value of a repeat_view, for views::take and views::drop (*E.value_).
struct __repeat_access {
  template <class _Rp>
  static constexpr decltype(auto) value(_Rp&& r) noexcept {
    return *static_cast<_Rp&&>(r).__value_;
  }
};

namespace __view_fn {

struct __single_fn {
  template <class _Tp>
    requires requires { std::ranges::single_view<std::decay_t<_Tp>>(std::declval<_Tp>()); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const
      noexcept(noexcept(std::ranges::single_view<std::decay_t<_Tp>>(static_cast<_Tp&&>(t)))) {
    return std::ranges::single_view<std::decay_t<_Tp>>(static_cast<_Tp&&>(t));
  }
};

struct __iota_fn {
  template <class _Wp>
    requires requires { std::ranges::iota_view<std::decay_t<_Wp>>(std::declval<_Wp>()); }
  [[nodiscard]] constexpr auto operator()(_Wp&& value) const
      noexcept(noexcept(std::ranges::iota_view<std::decay_t<_Wp>>(static_cast<_Wp&&>(value)))) {
    return std::ranges::iota_view<std::decay_t<_Wp>>(static_cast<_Wp&&>(value));
  }
  template <class _Wp, class _Bp>
    requires requires { std::ranges::iota_view(std::declval<_Wp>(), std::declval<_Bp>()); }
  [[nodiscard]] constexpr auto operator()(_Wp&& value, _Bp&& __y_bound) const
      noexcept(noexcept(std::ranges::iota_view(static_cast<_Wp&&>(value), static_cast<_Bp&&>(__y_bound)))) {
    return std::ranges::iota_view(static_cast<_Wp&&>(value), static_cast<_Bp&&>(__y_bound));
  }
};

struct __indices_fn {
  template <class _Ep>
    requires __integer_like<std::remove_cvref_t<_Ep>> &&
             requires { __iota_fn{}(std::remove_cvref_t<_Ep>(0), std::declval<_Ep>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const
      noexcept(noexcept(__iota_fn{}(std::remove_cvref_t<_Ep>(0), static_cast<_Ep&&>(e)))) {
    return __iota_fn{}(std::remove_cvref_t<_Ep>(0), static_cast<_Ep&&>(e));
  }
};

struct __repeat_fn {
  template <class _Tp>
    requires requires { std::ranges::repeat_view<std::decay_t<_Tp>>(std::declval<_Tp>()); }
  [[nodiscard]] constexpr auto operator()(_Tp&& value) const
      noexcept(noexcept(std::ranges::repeat_view<std::decay_t<_Tp>>(static_cast<_Tp&&>(value)))) {
    return std::ranges::repeat_view<std::decay_t<_Tp>>(static_cast<_Tp&&>(value));
  }
  template <class _Tp, class _Bp>
    requires requires { std::ranges::repeat_view(std::declval<_Tp>(), std::declval<_Bp>()); }
  [[nodiscard]] constexpr auto operator()(_Tp&& value, _Bp&& __y_bound) const
      noexcept(noexcept(std::ranges::repeat_view(static_cast<_Tp&&>(value), static_cast<_Bp&&>(__y_bound)))) {
    return std::ranges::repeat_view(static_cast<_Tp&&>(value), static_cast<_Bp&&>(__y_bound));
  }
};

} // namespace view_fn
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges::views {
inline constexpr __ycxx::__detail::__view_fn::__single_fn single{};
inline constexpr __ycxx::__detail::__view_fn::__iota_fn iota{};
inline constexpr __ycxx::__detail::__view_fn::__indices_fn indices{};
inline constexpr __ycxx::__detail::__view_fn::__repeat_fn repeat{};
}}} // namespace std::ranges::views

// ---- [range.istream] ---------------------------------------------------------------------------
// The view needs only the stream's interface: basic_istream is declared here (without default
// arguments, which <istream>/<iosfwd> supply) and must be complete where the view is used.
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _CharT, class _Traits>
class basic_istream;
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Val, class _CharT, class _Traits>
concept __stream_extractable = requires(std::basic_istream<_CharT, _Traits>& is, _Val& t) { is >> t; };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

template <movable _Val, class _CharT, class _Traits = char_traits<_CharT>>
  requires default_initializable<_Val> && __ycxx::__detail::__stream_extractable<_Val, _CharT, _Traits>
class basic_istream_view : public view_interface<basic_istream_view<_Val, _CharT, _Traits>> {
  class iterator {
    friend basic_istream_view;
    basic_istream_view* __parent_;
    constexpr explicit iterator(basic_istream_view& __parent) noexcept : __parent_(__builtin_addressof(__parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = _Val;

    iterator(const iterator&) = delete;
    iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = delete;
    iterator& operator=(iterator&&) = default;

    iterator& operator++() {
      *__parent_->__stream_ >> __parent_->__value_;
      return *this;
    }
    void operator++(int) { ++*this; }
    _Val& operator*() const { return __parent_->__value_; }
    friend bool operator==(const iterator& __x, default_sentinel_t) { return !*__x.__parent_->__stream_; }
  };

  basic_istream<_CharT, _Traits>* __stream_;
  _Val __value_ = _Val();

public:
  constexpr explicit basic_istream_view(basic_istream<_CharT, _Traits>& stream) : __stream_(__builtin_addressof(stream)) {}
  constexpr auto begin() {
    *__stream_ >> __value_;
    return iterator{*this};
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
};

template <class _Val>
using istream_view = basic_istream_view<_Val, char>;
template <class _Val>
using wistream_view = basic_istream_view<_Val, wchar_t>;

}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__view_fn {
template <class _Tp>
struct __istream_fn {
  template <class _Ep>
    requires requires {
      typename std::remove_cvref_t<_Ep>::char_type;
      typename std::remove_cvref_t<_Ep>::traits_type;
    } && std::derived_from<std::remove_cvref_t<_Ep>, std::basic_istream<typename std::remove_cvref_t<_Ep>::char_type,
                                                                     typename std::remove_cvref_t<_Ep>::traits_type>> &&
             requires(_Ep& e) {
               std::ranges::basic_istream_view<_Tp, typename std::remove_cvref_t<_Ep>::char_type,
                                               typename std::remove_cvref_t<_Ep>::traits_type>(e);
             }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    using _Up = std::remove_cvref_t<_Ep>;
    return std::ranges::basic_istream_view<_Tp, typename _Up::char_type, typename _Up::traits_type>(e);
  }
};
}} // namespace __ycxx::__detail::__view_fn

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges::views {
template <class _Tp>
constexpr __ycxx::__detail::__view_fn::__istream_fn<_Tp> istream{};
}}} // namespace std::ranges::views
