// libycxx core: the range adaptors that produce tuples or combine several ranges: elements
// (keys, values), enumerate, zip, zip_transform, adjacent (pairwise), adjacent_transform
// (pairwise_transform) and cartesian_product.
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/array.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- [range.adaptor.helpers] -----------------------------------------------------------------
template <class _Fp, class _Tuple>
constexpr auto __tuple_transform(_Fp&& __f, _Tuple&& t) {
  return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    return std::tuple<std::invoke_result_t<_Fp&, decltype(std::get<_Ip>(static_cast<_Tuple&&>(t)))>...>(
        ::__ycxx::__detail::invoke(__f, std::get<_Ip>(static_cast<_Tuple&&>(t)))...);
  }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<_Tuple>>>{});
}
template <class _Fp, class _Tuple>
constexpr void __tuple_for_each(_Fp&& __f, _Tuple&& t) {
  [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    (static_cast<void>(::__ycxx::__detail::invoke(__f, std::get<_Ip>(static_cast<_Tuple&&>(t)))), ...);
  }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<_Tuple>>>{});
}

template <bool _Const, class... _Views>
concept __all_random_access = (std::ranges::random_access_range<__maybe_const<_Const, _Views>> && ...);
template <bool _Const, class... _Views>
concept __all_bidirectional = (std::ranges::bidirectional_range<__maybe_const<_Const, _Views>> && ...);
template <bool _Const, class... _Views>
concept __all_forward = (std::ranges::forward_range<__maybe_const<_Const, _Views>> && ...);

template <bool _Const, class... _Views>
consteval auto __all_strength() {
  if constexpr (__all_random_access<_Const, _Views...>)
    return std::random_access_iterator_tag{};
  else if constexpr (__all_bidirectional<_Const, _Views...>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (__all_forward<_Const, _Views...>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}

// The value with the smallest absolute value.
template <class _Dp>
constexpr _Dp __smallest_abs(std::initializer_list<_Dp> __ds) {
  auto __best = *__ds.begin();
  for (_Dp d : __ds)
    if ((d < 0 ? -d : d) < (__best < 0 ? -__best : __best))
      __best = d;
  return __best;
}

// REPEAT(T, N) ([range.adjacent.overview]/3).
template <class _Tp, std::size_t>
using __repeat_type = _Tp;
template <class _Fp, class _Tp, class _Seq>
inline constexpr bool __repeat_regular_invocable_v = false;
template <class _Fp, class _Tp, std::size_t... _Ip>
inline constexpr bool __repeat_regular_invocable_v<_Fp, _Tp, std::index_sequence<_Ip...>> =
    std::regular_invocable<_Fp, __repeat_type<_Tp, _Ip>...>;
template <class _Fp, class _Tp, std::size_t _Np>
concept __repeat_regular_invocable = __repeat_regular_invocable_v<_Fp, _Tp, std::make_index_sequence<_Np>>;
template <class _Fp, class _Tp, class _Seq>
struct __repeat_invoke_result;
template <class _Fp, class _Tp, std::size_t... _Ip>
struct __repeat_invoke_result<_Fp, _Tp, std::index_sequence<_Ip...>> {
  using type = std::invoke_result_t<_Fp, __repeat_type<_Tp, _Ip>...>;
};
template <class _Fp, class _Tp, std::size_t _Np>
using __repeat_invoke_result_t = typename __repeat_invoke_result<_Fp, _Tp, std::make_index_sequence<_Np>>::type;
template <class _Tp, class _Seq>
struct __repeat_tuple;
template <class _Tp, std::size_t... _Ip>
struct __repeat_tuple<_Tp, std::index_sequence<_Ip...>> {
  using type = std::tuple<__repeat_type<_Tp, _Ip>...>;
};
template <class _Tp, std::size_t _Np>
using __repeat_tuple_t = typename __repeat_tuple<_Tp, std::make_index_sequence<_Np>>::type;

// ---- [range.elements.view] ---------------------------------------------------------------------
template <class _Tp, std::size_t _Np>
concept __has_tuple_element = __tuple_like<_Tp> && _Np < std::tuple_size_v<_Tp> && requires(_Tp t) {
  { std::get<_Np>(t) } -> std::convertible_to<const std::tuple_element_t<_Np, _Tp>&>;
};
template <class _Tp, std::size_t _Np>
concept __returnable_element = std::is_reference_v<_Tp> || std::move_constructible<std::tuple_element_t<_Np, _Tp>>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// =============================================================================================
// [range.elements]
// =============================================================================================
template <input_range _Vp, size_t _Np>
  requires view<_Vp> && __ycxx::__detail::__has_tuple_element<range_value_t<_Vp>, _Np> &&
           __ycxx::__detail::__has_tuple_element<remove_reference_t<range_reference_t<_Vp>>, _Np> &&
           __ycxx::__detail::__returnable_element<range_reference_t<_Vp>, _Np>
class elements_view : public view_interface<elements_view<_Vp, _Np>> {
  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (!is_lvalue_reference_v<decltype(std::get<_Np>(*std::declval<iterator_t<_Base>&>()))>)
        return type_identity<input_iterator_tag>{};
      else if constexpr (derived_from<_Cp, random_access_iterator_tag>)
        return type_identity<random_access_iterator_tag>{};
      else
        return type_identity<_Cp>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend elements_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();

    static constexpr decltype(auto) __get_element(const iterator_t<_Base>& i) {
      if constexpr (is_reference_v<range_reference_t<_Base>>) {
        return std::get<_Np>(*i);
      } else {
        using _Ep = remove_cv_t<tuple_element_t<_Np, range_reference_t<_Base>>>;
        return static_cast<_Ep>(std::get<_Np>(*i));
      }
    }
    constexpr explicit iterator(iterator_t<_Base> current) : __current_(std::move(current)) {}

  public:
    using iterator_concept = __ycxx::__detail::__range_strength_t<_Base>;
    using value_type = remove_cvref_t<tuple_element_t<_Np, range_value_t<_Base>>>;
    using difference_type = range_difference_t<_Base>;

    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)) {}

    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr decltype(auto) operator*() const { return __get_element(__current_); }

    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr void operator++(int) { ++__current_; }
    constexpr iterator operator++(int)
      requires forward_range<_Base>
    {
      auto __tmp = *this;
      ++__current_;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Base>
    {
      --__current_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --__current_;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires random_access_range<_Base>
    {
      __current_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<_Base>
    {
      __current_ -= n;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return __get_element(__current_ + n);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires equality_comparable<iterator_t<_Base>>
    {
      return __x.__current_ == y.__current_;
    }
    friend constexpr bool operator<(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ < y.__current_;
    }
    friend constexpr bool operator>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return y < __x;
    }
    friend constexpr bool operator<=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return !(y < __x);
    }
    friend constexpr bool operator>=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return !(__x < y);
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base> && three_way_comparable<iterator_t<_Base>>
    {
      return __x.__current_ <=> y.__current_;
    }
    friend constexpr iterator operator+(const iterator& __x, difference_type y)
      requires random_access_range<_Base>
    {
      return iterator{__x} += y;
    }
    friend constexpr iterator operator+(difference_type __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return y + __x;
    }
    friend constexpr iterator operator-(const iterator& __x, difference_type y)
      requires random_access_range<_Base>
    {
      return iterator{__x} -= y;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>>
    {
      return __x.__current_ - y.__current_;
    }
  };

  template <bool _Const>
  class sentinel {
    friend elements_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(sentinel_t<_Base> end) : __end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> other)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(other.__end_)) {}
    constexpr sentinel_t<_Base> base() const { return __end_; }

    template <bool _OtherConst>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const iterator<_OtherConst>& __x,
                                                                                            const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) - y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const sentinel& __x,
                                                                                            const iterator<_OtherConst>& y) {
      return __x.__end_ - __ycxx::__detail::__view_access::current(y);
    }
  };

  _Vp __base_ = _Vp();

public:
  elements_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit elements_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return iterator<false>(ranges::begin(__base_));
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return iterator<true>(ranges::begin(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp> && !common_range<_Vp>)
  {
    return sentinel<false>{ranges::end(__base_)};
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp> && common_range<_Vp>)
  {
    return iterator<false>{ranges::end(__base_)};
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    return sentinel<true>{ranges::end(__base_)};
  }
  constexpr auto end() const
    requires common_range<const _Vp>
  {
    return iterator<true>{ranges::end(__base_)};
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return ranges::size(__base_);
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return ranges::size(__base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    return ranges::reserve_hint(__base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    return ranges::reserve_hint(__base_);
  }
};
template <class _Tp, size_t _Np>
constexpr bool enable_borrowed_range<elements_view<_Tp, _Np>> = enable_borrowed_range<_Tp>;
template <class _Rp>
using keys_view = elements_view<_Rp, 0>;
template <class _Rp>
using values_view = elements_view<_Rp, 1>;

// =============================================================================================
// [range.enumerate]
// =============================================================================================
template <view _Vp>
  requires __ycxx::__detail::__range_with_movable_references<_Vp>
class enumerate_view : public view_interface<enumerate_view<_Vp>> {
  template <bool _Const>
  class iterator {
    friend enumerate_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept = __ycxx::__detail::__range_strength_t<_Base>;
    using difference_type = range_difference_t<_Base>;
    using value_type = tuple<difference_type, range_value_t<_Base>>;

  private:
    using __reference_type = tuple<difference_type, range_reference_t<_Base>>;
    iterator_t<_Base> __current_ = iterator_t<_Base>();
    difference_type __pos_ = 0;

    constexpr explicit iterator(iterator_t<_Base> current, difference_type __pos)
        : __current_(std::move(current)), __pos_(__pos) {}

  public:
    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)), __pos_(i.__pos_) {}

    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr difference_type index() const noexcept { return __pos_; }
    constexpr auto operator*() const { return __reference_type(__pos_, *__current_); }

    constexpr iterator& operator++() {
      ++__current_;
      ++__pos_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires forward_range<_Base>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Base>
    {
      --__current_;
      --__pos_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires random_access_range<_Base>
    {
      __current_ += n;
      __pos_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<_Base>
    {
      __current_ -= n;
      __pos_ -= n;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return __reference_type(__pos_ + n, __current_[n]);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) noexcept { return __x.__pos_ == y.__pos_; }
    friend constexpr strong_ordering operator<=>(const iterator& __x, const iterator& y) noexcept {
      return __x.__pos_ <=> y.__pos_;
    }
    friend constexpr iterator operator+(const iterator& __x, difference_type y)
      requires random_access_range<_Base>
    {
      auto __temp = __x;
      __temp += y;
      return __temp;
    }
    friend constexpr iterator operator+(difference_type __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return y + __x;
    }
    friend constexpr iterator operator-(const iterator& __x, difference_type y)
      requires random_access_range<_Base>
    {
      auto __temp = __x;
      __temp -= y;
      return __temp;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y) noexcept { return __x.__pos_ - y.__pos_; }
    friend constexpr auto iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(i.__current_)) &&
                                                                is_nothrow_move_constructible_v<range_rvalue_reference_t<_Base>>) {
      return tuple<difference_type, range_rvalue_reference_t<_Base>>(i.__pos_, ranges::iter_move(i.__current_));
    }
  };

  template <bool _Const>
  class sentinel {
    friend enumerate_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(sentinel_t<_Base> end) : __end_(std::move(end)) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> other)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(other.__end_)) {}
    constexpr sentinel_t<_Base> base() const { return __end_; }

    template <bool _OtherConst>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const iterator<_OtherConst>& __x,
                                                                                            const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) - y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const sentinel& __x,
                                                                                            const iterator<_OtherConst>& y) {
      return __x.__end_ - __ycxx::__detail::__view_access::current(y);
    }
  };

  _Vp __base_ = _Vp();

public:
  constexpr enumerate_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit enumerate_view(_Vp base) : __base_(std::move(base)) {}

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return iterator<false>(ranges::begin(__base_), 0);
  }
  constexpr auto begin() const
    requires __ycxx::__detail::__range_with_movable_references<const _Vp>
  {
    return iterator<true>(ranges::begin(__base_), 0);
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (forward_range<_Vp> && common_range<_Vp> && sized_range<_Vp>)
      return iterator<false>(ranges::end(__base_), ranges::distance(__base_));
    else
      return sentinel<false>(ranges::end(__base_));
  }
  constexpr auto end() const
    requires __ycxx::__detail::__range_with_movable_references<const _Vp>
  {
    if constexpr (forward_range<const _Vp> && common_range<const _Vp> && sized_range<const _Vp>)
      return iterator<true>(ranges::end(__base_), ranges::distance(__base_));
    else
      return sentinel<true>(ranges::end(__base_));
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return ranges::size(__base_);
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return ranges::size(__base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    return ranges::reserve_hint(__base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    return ranges::reserve_hint(__base_);
  }
  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
};
template <class _Rp>
enumerate_view(_Rp&&) -> enumerate_view<views::all_t<_Rp>>;
template <class _View>
constexpr bool enable_borrowed_range<enumerate_view<_View>> = enable_borrowed_range<_View>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class... _Rs>
concept __zip_is_common = (sizeof...(_Rs) == 1 && (std::ranges::common_range<_Rs> && ...)) ||
                        (!(std::ranges::bidirectional_range<_Rs> && ...) && (std::ranges::common_range<_Rs> && ...)) ||
                        ((std::ranges::random_access_range<_Rs> && ...) && (std::ranges::sized_range<_Rs> && ...));
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// =============================================================================================
// [range.zip]
// =============================================================================================
template <input_range... _Views>
  requires(view<_Views> && ...) && (sizeof...(_Views) > 0)
class zip_view : public view_interface<zip_view<_Views...>> {
  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<
                       conditional_t<__ycxx::__detail::__all_forward<_Const, _Views...>, input_iterator_tag, void>> {
    friend zip_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;

    tuple<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...> __current_;
    constexpr explicit iterator(tuple<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...> current)
        : __current_(std::move(current)) {}

  public:
    using iterator_concept = decltype(__ycxx::__detail::__all_strength<_Const, _Views...>());
    using value_type = tuple<range_value_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>;
    using difference_type = common_type_t<range_difference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && (convertible_to<iterator_t<_Views>, iterator_t<const _Views>> && ...)
        : __current_(std::move(i.__current_)) {}

    constexpr auto operator*() const {
      return __ycxx::__detail::__tuple_transform([](auto& i) -> decltype(auto) { return *i; }, __current_);
    }
    constexpr iterator& operator++() {
      __ycxx::__detail::__tuple_for_each([](auto& i) { ++i; }, __current_);
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires __ycxx::__detail::__all_forward<_Const, _Views...>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires __ycxx::__detail::__all_bidirectional<_Const, _Views...>
    {
      __ycxx::__detail::__tuple_for_each([](auto& i) { --i; }, __current_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ycxx::__detail::__all_bidirectional<_Const, _Views...>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type __x)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      __ycxx::__detail::__tuple_for_each([&]<class _Ip>(_Ip& i) { i += iter_difference_t<_Ip>(__x); }, __current_);
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      __ycxx::__detail::__tuple_for_each([&]<class _Ip>(_Ip& i) { i -= iter_difference_t<_Ip>(__x); }, __current_);
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __ycxx::__detail::__tuple_transform(
          [&]<class _Ip>(_Ip& i) -> decltype(auto) { return i[iter_difference_t<_Ip>(n)]; }, __current_);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires(equality_comparable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> && ...)
    {
      if constexpr (__ycxx::__detail::__all_bidirectional<_Const, _Views...>) {
        return __x.__current_ == y.__current_;
      } else {
        return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
          return (bool(std::get<_Ip>(__x.__current_) == std::get<_Ip>(y.__current_)) || ...);
        }(index_sequence_for<_Views...>{});
      }
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __x.__current_ <=> y.__current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires(sized_sentinel_for<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>,
                                  iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> &&
               ...)
    {
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        return __ycxx::__detail::__smallest_abs<difference_type>(
            {difference_type(std::get<_Ip>(__x.__current_) - std::get<_Ip>(y.__current_))...});
      }(index_sequence_for<_Views...>{});
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        (noexcept(ranges::iter_move(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>())) && ...) &&
        (is_nothrow_move_constructible_v<range_rvalue_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> && ...)) {
      return __ycxx::__detail::__tuple_transform(ranges::iter_move, i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __l, const iterator& r) noexcept(
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>(),
                                    std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>())) &&
         ...))
      requires(indirectly_swappable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> && ...)
    {
      [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        (ranges::iter_swap(std::get<_Ip>(__l.__current_), std::get<_Ip>(r.__current_)), ...);
      }(index_sequence_for<_Views...>{});
    }
  };

  template <bool _Const>
  class sentinel {
    friend zip_view;
    friend sentinel<!_Const>;

    tuple<sentinel_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...> __end_;
    constexpr explicit sentinel(tuple<sentinel_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...> end) : __end_(end) {}

    template <bool _OtherConst>
    static constexpr auto distance(const iterator<_OtherConst>& __x, const sentinel& y) {
      using _Dp = common_type_t<range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>...>;
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        return __ycxx::__detail::__smallest_abs<_Dp>(
            {_Dp(std::get<_Ip>(__ycxx::__detail::__view_access::current(__x)) - std::get<_Ip>(y.__end_))...});
      }(index_sequence_for<_Views...>{});
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> i)
      requires _Const && (convertible_to<sentinel_t<_Views>, sentinel_t<const _Views>> && ...)
        : __end_(std::move(i.__end_)) {}

    template <bool _OtherConst>
      requires(sentinel_for<sentinel_t<__ycxx::__detail::__maybe_const<_Const, _Views>>,
                            iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>> &&
               ...)
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        return (bool(std::get<_Ip>(__ycxx::__detail::__view_access::current(__x)) == std::get<_Ip>(y.__end_)) || ...);
      }(index_sequence_for<_Views...>{});
    }
    template <bool _OtherConst>
      requires(sized_sentinel_for<sentinel_t<__ycxx::__detail::__maybe_const<_Const, _Views>>,
                                  iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>> &&
               ...)
    friend constexpr common_type_t<range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>...> operator-(
        const iterator<_OtherConst>& __x, const sentinel& y) {
      return distance(__x, y);
    }
    template <bool _OtherConst>
      requires(sized_sentinel_for<sentinel_t<__ycxx::__detail::__maybe_const<_Const, _Views>>,
                                  iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>> &&
               ...)
    friend constexpr common_type_t<range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Views>>...> operator-(
        const sentinel& y, const iterator<_OtherConst>& __x) {
      return -distance(__x, y);
    }
  };

  tuple<_Views...> __views_;

  template <bool _Const>
  static constexpr auto size_of(__ycxx::__detail::__maybe_const<_Const, tuple<_Views...>>& views) {
    return std::apply(
        [](auto... __sizes) {
          using _CT = make_unsigned_t<common_type_t<decltype(__sizes)...>>;
          return ranges::min({_CT(__sizes)...});
        },
        __ycxx::__detail::__tuple_transform(ranges::size, views));
  }

public:
  zip_view() = default;
  constexpr explicit zip_view(_Views... views) : __views_(std::move(views)...) {}

  constexpr auto begin()
    requires(!(__ycxx::__detail::__simple_view<_Views> && ...))
  {
    return iterator<false>(__ycxx::__detail::__tuple_transform(ranges::begin, __views_));
  }
  constexpr auto begin() const
    requires(range<const _Views> && ...)
  {
    return iterator<true>(__ycxx::__detail::__tuple_transform(ranges::begin, __views_));
  }
  constexpr auto end()
    requires(!(__ycxx::__detail::__simple_view<_Views> && ...))
  {
    if constexpr (!__ycxx::__detail::__zip_is_common<_Views...>)
      return sentinel<false>(__ycxx::__detail::__tuple_transform(ranges::end, __views_));
    else if constexpr ((random_access_range<_Views> && ...))
      return begin() + iter_difference_t<iterator<false>>(size());
    else
      return iterator<false>(__ycxx::__detail::__tuple_transform(ranges::end, __views_));
  }
  constexpr auto end() const
    requires(range<const _Views> && ...)
  {
    if constexpr (!__ycxx::__detail::__zip_is_common<const _Views...>)
      return sentinel<true>(__ycxx::__detail::__tuple_transform(ranges::end, __views_));
    else if constexpr ((random_access_range<const _Views> && ...))
      return begin() + iter_difference_t<iterator<true>>(size());
    else
      return iterator<true>(__ycxx::__detail::__tuple_transform(ranges::end, __views_));
  }
  constexpr auto size()
    requires(sized_range<_Views> && ...)
  {
    return size_of<false>(__views_);
  }
  constexpr auto size() const
    requires(sized_range<const _Views> && ...)
  {
    return size_of<true>(__views_);
  }
};
template <class... _Rs>
zip_view(_Rs&&...) -> zip_view<views::all_t<_Rs>...>;
template <class... _Views>
constexpr bool enable_borrowed_range<zip_view<_Views...>> = (enable_borrowed_range<_Views> && ...);

// =============================================================================================
// [range.zip.transform]
// =============================================================================================
template <move_constructible _Fp, input_range... _Views>
  requires(view<_Views> && ...) && (sizeof...(_Views) > 0) && is_object_v<_Fp> &&
          regular_invocable<_Fp&, range_reference_t<_Views>...> &&
          __ycxx::__detail::__can_reference<invoke_result_t<_Fp&, range_reference_t<_Views>...>>
class zip_transform_view : public view_interface<zip_transform_view<_Fp, _Views...>> {
  using _InnerView = zip_view<_Views...>;
  template <bool _Const>
  using __ziperator = iterator_t<__ycxx::__detail::__maybe_const<_Const, _InnerView>>;
  template <bool _Const>
  using __zentinel = sentinel_t<__ycxx::__detail::__maybe_const<_Const, _InnerView>>;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _InnerView>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else if constexpr (!is_reference_v<invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&,
                                                          range_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>>) {
      return type_identity<input_iterator_tag>{};
    } else {
      using namespace __ycxx::__detail;
      if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>, random_access_iterator_tag> &&
                     ...))
        return type_identity<random_access_iterator_tag>{};
      else if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>,
                                       bidirectional_iterator_tag> &&
                          ...))
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>, forward_iterator_tag> &&
                          ...))
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend zip_transform_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, zip_transform_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _InnerView>;

    _Parent* __parent_ = nullptr;
    __ziperator<_Const> __current_;

    constexpr iterator(_Parent& __parent, __ziperator<_Const> __inner)
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(__inner)) {}

  public:
    using iterator_concept = typename __ziperator<_Const>::iterator_concept;
    using value_type = remove_cvref_t<
        invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&, range_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>>;
    using difference_type = range_difference_t<_Base>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<__ziperator<false>, __ziperator<_Const>>
        : __parent_(i.__parent_), __current_(std::move(i.__current_)) {}

    constexpr decltype(auto) operator*() const
        noexcept(noexcept(::__ycxx::__detail::invoke(std::declval<__ycxx::__detail::__maybe_const<_Const, _Fp>&>(),
                                                 *std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>()...))) {
      return std::apply(
          [&](const auto&... __iters) -> decltype(auto) { return ::__ycxx::__detail::invoke(*__parent_->__fun_, *__iters...); },
          __ycxx::__detail::__view_access::current(__current_));
    }
    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires forward_range<_Base>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Base>
    {
      --__current_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type __x)
      requires random_access_range<_Base>
    {
      __current_ += __x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires random_access_range<_Base>
    {
      __current_ -= __x;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return std::apply(
          [&]<class... _Is>(const _Is&... __iters) -> decltype(auto) {
            return ::__ycxx::__detail::invoke(*__parent_->__fun_, __iters[iter_difference_t<_Is>(n)]...);
          },
          __ycxx::__detail::__view_access::current(__current_));
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires equality_comparable<__ziperator<_Const>>
    {
      return __x.__current_ == y.__current_;
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ <=> y.__current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ + n);
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ + n);
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ - n);
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires sized_sentinel_for<__ziperator<_Const>, __ziperator<_Const>>
    {
      return __x.__current_ - y.__current_;
    }
  };

  template <bool _Const>
  class sentinel {
    friend zip_transform_view;
    friend sentinel<!_Const>;

    __zentinel<_Const> __inner_;
    constexpr explicit sentinel(__zentinel<_Const> __inner) : __inner_(__inner) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> i)
      requires _Const && convertible_to<__zentinel<false>, __zentinel<_Const>>
        : __inner_(std::move(i.__inner_)) {}

    template <bool _OtherConst>
      requires sentinel_for<__zentinel<_Const>, __ziperator<_OtherConst>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__inner_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<__zentinel<_Const>, __ziperator<_OtherConst>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _InnerView>> operator-(
        const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) - y.__inner_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<__zentinel<_Const>, __ziperator<_OtherConst>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _InnerView>> operator-(
        const sentinel& __x, const iterator<_OtherConst>& y) {
      return __x.__inner_ - __ycxx::__detail::__view_access::current(y);
    }
  };

  [[no_unique_address]] __ycxx::__detail::__movable_box<_Fp> __fun_;
  _InnerView __zip_;

public:
  zip_transform_view() = default;
  constexpr explicit zip_transform_view(_Fp fun, _Views... views) : __fun_(in_place, std::move(fun)), __zip_(std::move(views)...) {}

  constexpr auto begin() { return iterator<false>(*this, __zip_.begin()); }
  constexpr auto begin() const
    requires range<const _InnerView> && regular_invocable<const _Fp&, range_reference_t<const _Views>...>
  {
    return iterator<true>(*this, __zip_.begin());
  }
  constexpr auto end() {
    if constexpr (common_range<_InnerView>)
      return iterator<false>(*this, __zip_.end());
    else
      return sentinel<false>(__zip_.end());
  }
  constexpr auto end() const
    requires range<const _InnerView> && regular_invocable<const _Fp&, range_reference_t<const _Views>...>
  {
    if constexpr (common_range<const _InnerView>)
      return iterator<true>(*this, __zip_.end());
    else
      return sentinel<true>(__zip_.end());
  }
  constexpr auto size()
    requires sized_range<_InnerView>
  {
    return __zip_.size();
  }
  constexpr auto size() const
    requires sized_range<const _InnerView>
  {
    return __zip_.size();
  }
};
template <class _Fp, class... _Rs>
zip_transform_view(_Fp, _Rs&&...) -> zip_transform_view<_Fp, views::all_t<_Rs>...>;

// =============================================================================================
// [range.adjacent]
// =============================================================================================
template <forward_range _Vp, size_t _Np>
  requires view<_Vp> && (_Np > 0)
class adjacent_view : public view_interface<adjacent_view<_Vp, _Np>> {
  struct __as_sentinel {};

  template <bool _Const>
  class iterator {
    friend adjacent_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    array<iterator_t<_Base>, _Np> __current_ = array<iterator_t<_Base>, _Np>();

    constexpr iterator(iterator_t<_Base> first, sentinel_t<_Base> last) {
      __current_[0] = first;
      for (size_t i = 1; i < _Np; ++i)
        __current_[i] = ranges::next(__current_[i - 1], 1, last);
    }
    constexpr iterator(__as_sentinel, iterator_t<_Base> first, iterator_t<_Base> last) {
      if constexpr (!bidirectional_range<_Base>) {
        for (auto& __it : __current_)
          __it = last;
      } else {
        __current_[_Np - 1] = last;
        for (size_t i = _Np - 1; i-- > 0;)
          __current_[i] = ranges::prev(__current_[i + 1], 1, first);
      }
    }

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<_Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<_Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = __ycxx::__detail::__repeat_tuple_t<range_value_t<_Base>, _Np>;
    using difference_type = range_difference_t<_Base>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
    {
      for (size_t k = 0; k < _Np; ++k)
        __current_[k] = std::move(i.__current_[k]);
    }

    constexpr auto operator*() const {
      return __ycxx::__detail::__tuple_transform([](auto& i) -> decltype(auto) { return *i; }, __current_);
    }
    constexpr iterator& operator++() {
      for (auto& i : __current_)
        ++i;
      return *this;
    }
    constexpr iterator operator++(int) {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Base>
    {
      for (auto& i : __current_)
        --i;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type __x)
      requires random_access_range<_Base>
    {
      for (auto& i : __current_)
        i += __x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires random_access_range<_Base>
    {
      for (auto& i : __current_)
        i -= __x;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return __ycxx::__detail::__tuple_transform([&](auto& i) -> decltype(auto) { return i[n]; }, __current_);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) {
      return __x.__current_.back() == y.__current_.back();
    }
    friend constexpr bool operator<(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_.back() < y.__current_.back();
    }
    friend constexpr bool operator>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return y < __x;
    }
    friend constexpr bool operator<=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return !(y < __x);
    }
    friend constexpr bool operator>=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return !(__x < y);
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base> && three_way_comparable<iterator_t<_Base>>
    {
      return __x.__current_.back() <=> y.__current_.back();
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<_Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>>
    {
      return __x.__current_.back() - y.__current_.back();
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(std::declval<const iterator_t<_Base>&>())) &&
        is_nothrow_move_constructible_v<range_rvalue_reference_t<_Base>>) {
      return __ycxx::__detail::__tuple_transform(ranges::iter_move, i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __l, const iterator& r) noexcept(
        noexcept(ranges::iter_swap(std::declval<iterator_t<_Base>>(), std::declval<iterator_t<_Base>>())))
      requires indirectly_swappable<iterator_t<_Base>>
    {
      for (size_t i = 0; i < _Np; ++i)
        ranges::iter_swap(__l.__current_[i], r.__current_[i]);
    }
  };

  template <bool _Const>
  class sentinel {
    friend adjacent_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(sentinel_t<_Base> end) : __end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> i)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(i.__end_)) {}

    template <bool _OtherConst>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x).back() == y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const iterator<_OtherConst>& __x,
                                                                                            const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x).back() - y.__end_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const sentinel& y,
                                                                                            const iterator<_OtherConst>& __x) {
      return y.__end_ - __ycxx::__detail::__view_access::current(__x).back();
    }
  };

  _Vp __base_ = _Vp();

  template <class _Rp>
  static constexpr auto size_of(_Rp& r) {
    using _ST = decltype(ranges::size(r));
    using _CT = common_type_t<_ST, size_t>;
    auto __sz = static_cast<_CT>(ranges::size(r));
    __sz -= std::min<_CT>(__sz, _Np - 1);
    return static_cast<_ST>(__sz);
  }
  template <class _Rp>
  static constexpr auto __hint_of(_Rp& r) {
    using _DT = range_difference_t<_Rp>;
    using _CT = common_type_t<_DT, size_t>;
    auto __sz = static_cast<_CT>(ranges::reserve_hint(r));
    __sz -= std::min<_CT>(__sz, _Np - 1);
    return ::__ycxx::__detail::__to_unsigned_like(__sz);
  }

public:
  adjacent_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit adjacent_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return iterator<false>(ranges::begin(__base_), ranges::end(__base_));
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return iterator<true>(ranges::begin(__base_), ranges::end(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (common_range<_Vp>)
      return iterator<false>(__as_sentinel{}, ranges::begin(__base_), ranges::end(__base_));
    else
      return sentinel<false>(ranges::end(__base_));
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    if constexpr (common_range<const _Vp>)
      return iterator<true>(__as_sentinel{}, ranges::begin(__base_), ranges::end(__base_));
    else
      return sentinel<true>(ranges::end(__base_));
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return size_of(__base_);
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return size_of(__base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    return __hint_of(__base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    return __hint_of(__base_);
  }
};
template <class _Vp, size_t _Np>
constexpr bool enable_borrowed_range<adjacent_view<_Vp, _Np>> = enable_borrowed_range<_Vp>;

// =============================================================================================
// [range.adjacent.transform]
// =============================================================================================
template <forward_range _Vp, move_constructible _Fp, size_t _Np>
  requires view<_Vp> && (_Np > 0) && is_object_v<_Fp> &&
           __ycxx::__detail::__repeat_regular_invocable<_Fp&, range_reference_t<_Vp>, _Np> &&
           __ycxx::__detail::__can_reference<__ycxx::__detail::__repeat_invoke_result_t<_Fp&, range_reference_t<_Vp>, _Np>>
class adjacent_transform_view : public view_interface<adjacent_transform_view<_Vp, _Fp, _Np>> {
  using _InnerView = adjacent_view<_Vp, _Np>;
  template <bool _Const>
  using __inner_iterator = iterator_t<__ycxx::__detail::__maybe_const<_Const, _InnerView>>;
  template <bool _Const>
  using __inner_sentinel = sentinel_t<__ycxx::__detail::__maybe_const<_Const, _InnerView>>;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!is_reference_v<__ycxx::__detail::__repeat_invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&,
                                                                        range_reference_t<_Base>, _Np>>) {
      return input_iterator_tag{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (derived_from<_Cp, random_access_iterator_tag>)
        return random_access_iterator_tag{};
      else if constexpr (derived_from<_Cp, bidirectional_iterator_tag>)
        return bidirectional_iterator_tag{};
      else if constexpr (derived_from<_Cp, forward_iterator_tag>)
        return forward_iterator_tag{};
      else
        return input_iterator_tag{};
    }
  }

  template <bool _Const>
  class iterator {
    friend adjacent_transform_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, adjacent_transform_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    _Parent* __parent_ = nullptr;
    __inner_iterator<_Const> __current_;

    constexpr iterator(_Parent& __parent, __inner_iterator<_Const> __inner)
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(__inner)) {}

    template <class _Fn>
    constexpr decltype(auto) __apply_inner(_Fn&& __fn) const {
      auto& __its = __ycxx::__detail::__view_access::current(__current_);
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) -> decltype(auto) {
        return __fn(__its[_Ip]...);
      }(make_index_sequence<_Np>{});
    }

  public:
    using iterator_category = decltype(category<_Const>());
    using iterator_concept = typename __inner_iterator<_Const>::iterator_concept;
    using value_type = remove_cvref_t<
        __ycxx::__detail::__repeat_invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&, range_reference_t<_Base>, _Np>>;
    using difference_type = range_difference_t<_Base>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<__inner_iterator<false>, __inner_iterator<_Const>>
        : __parent_(i.__parent_), __current_(std::move(i.__current_)) {}

    static consteval bool __deref_nothrow() {
      return []<size_t... _Ip>(index_sequence<_Ip...>) {
        return noexcept(::__ycxx::__detail::invoke(
            std::declval<__ycxx::__detail::__maybe_const<_Const, _Fp>&>(),
            *std::declval<__ycxx::__detail::__repeat_type<const iterator_t<_Base>&, _Ip>>()...));
      }(make_index_sequence<_Np>{});
    }
    constexpr decltype(auto) operator*() const noexcept(__deref_nothrow()) {
      return __apply_inner(
          [&](const auto&... __iters) -> decltype(auto) { return ::__ycxx::__detail::invoke(*__parent_->__fun_, *__iters...); });
    }
    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr iterator operator++(int) {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Base>
    {
      --__current_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type __x)
      requires random_access_range<_Base>
    {
      __current_ += __x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires random_access_range<_Base>
    {
      __current_ -= __x;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return __apply_inner(
          [&](const auto&... __iters) -> decltype(auto) { return ::__ycxx::__detail::invoke(*__parent_->__fun_, __iters[n]...); });
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) { return __x.__current_ == y.__current_; }
    friend constexpr bool operator<(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ < y.__current_;
    }
    friend constexpr bool operator>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ > y.__current_;
    }
    friend constexpr bool operator<=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ <= y.__current_;
    }
    friend constexpr bool operator>=(const iterator& __x, const iterator& y)
      requires random_access_range<_Base>
    {
      return __x.__current_ >= y.__current_;
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires random_access_range<_Base> && three_way_comparable<__inner_iterator<_Const>>
    {
      return __x.__current_ <=> y.__current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ + n);
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ + n);
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator(*i.__parent_, i.__current_ - n);
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires sized_sentinel_for<__inner_iterator<_Const>, __inner_iterator<_Const>>
    {
      return __x.__current_ - y.__current_;
    }
  };

  template <bool _Const>
  class sentinel {
    friend adjacent_transform_view;
    friend sentinel<!_Const>;

    __inner_sentinel<_Const> __inner_;
    constexpr explicit sentinel(__inner_sentinel<_Const> __inner) : __inner_(__inner) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> i)
      requires _Const && convertible_to<__inner_sentinel<false>, __inner_sentinel<_Const>>
        : __inner_(std::move(i.__inner_)) {}

    template <bool _OtherConst>
      requires sentinel_for<__inner_sentinel<_Const>, __inner_iterator<_OtherConst>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__inner_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<__inner_sentinel<_Const>, __inner_iterator<_OtherConst>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _InnerView>> operator-(
        const iterator<_OtherConst>& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) - y.__inner_;
    }
    template <bool _OtherConst>
      requires sized_sentinel_for<__inner_sentinel<_Const>, __inner_iterator<_OtherConst>>
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _InnerView>> operator-(
        const sentinel& __x, const iterator<_OtherConst>& y) {
      return __x.__inner_ - __ycxx::__detail::__view_access::current(y);
    }
  };

  [[no_unique_address]] __ycxx::__detail::__movable_box<_Fp> __fun_;
  _InnerView __inner_;

public:
  adjacent_transform_view() = default;
  constexpr explicit adjacent_transform_view(_Vp base, _Fp fun) : __fun_(in_place, std::move(fun)), __inner_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __inner_.base();
  }
  constexpr _Vp base() && { return std::move(__inner_).base(); }

  constexpr auto begin() { return iterator<false>(*this, __inner_.begin()); }
  constexpr auto begin() const
    requires range<const _InnerView> && __ycxx::__detail::__repeat_regular_invocable<const _Fp&, range_reference_t<const _Vp>, _Np>
  {
    return iterator<true>(*this, __inner_.begin());
  }
  constexpr auto end() {
    if constexpr (common_range<_InnerView>)
      return iterator<false>(*this, __inner_.end());
    else
      return sentinel<false>(__inner_.end());
  }
  constexpr auto end() const
    requires range<const _InnerView> && __ycxx::__detail::__repeat_regular_invocable<const _Fp&, range_reference_t<const _Vp>, _Np>
  {
    if constexpr (common_range<const _InnerView>)
      return iterator<true>(*this, __inner_.end());
    else
      return sentinel<true>(__inner_.end());
  }
  constexpr auto size()
    requires sized_range<_InnerView>
  {
    return __inner_.size();
  }
  constexpr auto size() const
    requires sized_range<const _InnerView>
  {
    return __inner_.size();
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_InnerView>
  {
    return __inner_.reserve_hint();
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _InnerView>
  {
    return __inner_.reserve_hint();
  }
};

}} // namespace std::ranges

// =============================================================================================
// [range.cartesian]
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <bool _Const, class _First, class... _Vs>
concept __cartesian_product_is_random_access =
    (std::ranges::random_access_range<__maybe_const<_Const, _First>> && ... &&
     (std::ranges::random_access_range<__maybe_const<_Const, _Vs>> && std::ranges::sized_range<__maybe_const<_Const, _Vs>>));
template <class _Rp>
concept __cartesian_product_common_arg =
    std::ranges::common_range<_Rp> || (std::ranges::sized_range<_Rp> && std::ranges::random_access_range<_Rp>);
template <bool _Const, class _First, class... _Vs>
concept __cartesian_product_is_bidirectional =
    (std::ranges::bidirectional_range<__maybe_const<_Const, _First>> && ... &&
     (std::ranges::bidirectional_range<__maybe_const<_Const, _Vs>> && __cartesian_product_common_arg<__maybe_const<_Const, _Vs>>));
template <class _First, class...>
concept __cartesian_product_is_common = __cartesian_product_common_arg<_First>;
template <class... _Vs>
concept __cartesian_product_is_sized = (std::ranges::sized_range<_Vs> && ...);
template <bool _Const, template <class> class _FirstSent, class _First, class... _Vs>
concept __cartesian_is_sized_sentinel =
    (std::sized_sentinel_for<_FirstSent<__maybe_const<_Const, _First>>, std::ranges::iterator_t<__maybe_const<_Const, _First>>> &&
     ... &&
     (std::ranges::sized_range<__maybe_const<_Const, _Vs>> &&
      std::sized_sentinel_for<std::ranges::iterator_t<__maybe_const<_Const, _Vs>>,
                              std::ranges::iterator_t<__maybe_const<_Const, _Vs>>>));

template <__cartesian_product_common_arg _Rp>
constexpr auto __cartesian_common_arg_end(_Rp& r) {
  if constexpr (std::ranges::common_range<_Rp>)
    return std::ranges::end(r);
  else
    return std::ranges::begin(r) + std::ranges::distance(r);
}

template <class _Rp>
using __cartesian_sentinel_t = std::ranges::sentinel_t<_Rp>;
template <class _Rp>
using __cartesian_iterator_t = std::ranges::iterator_t<_Rp>;

// The difference type: the first range's alone; for a product of several ranges a 128-bit type
// where available, wide enough for the product of two 64-bit sizes.
template <class... _Ds>
consteval auto __cartesian_difference() {
  if constexpr (sizeof...(_Ds) > 1 && __cfg::__has_int128)
    return std::type_identity<__y_int128>{};
  else
    return std::type_identity<std::common_type_t<std::ptrdiff_t, _Ds...>>{};
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <input_range _First, forward_range... _Vs>
  requires(view<_First> && ... && view<_Vs>)
class cartesian_product_view : public view_interface<cartesian_product_view<_First, _Vs...>> {
  template <bool _Const>
  class iterator {
    friend cartesian_product_view;
    friend iterator<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, cartesian_product_view>;

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept = conditional_t<
        __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>, random_access_iterator_tag,
        conditional_t<__ycxx::__detail::__cartesian_product_is_bidirectional<_Const, _First, _Vs...>, bidirectional_iterator_tag,
                      conditional_t<forward_range<__ycxx::__detail::__maybe_const<_Const, _First>>, forward_iterator_tag,
                                    input_iterator_tag>>>;
    using value_type =
        tuple<range_value_t<__ycxx::__detail::__maybe_const<_Const, _First>>, range_value_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>;
    using reference = tuple<range_reference_t<__ycxx::__detail::__maybe_const<_Const, _First>>,
                            range_reference_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>;
    using difference_type = typename decltype(__ycxx::__detail::__cartesian_difference<
                                              range_difference_t<__ycxx::__detail::__maybe_const<_Const, _First>>,
                                              range_difference_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>())::type;

  private:
    _Parent* __parent_ = nullptr;
    tuple<iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>, iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>
        __current_;

    template <size_t _Np = sizeof...(_Vs)>
    constexpr void next() {
      auto& __it = std::get<_Np>(__current_);
      ++__it;
      if constexpr (_Np > 0) {
        if (__it == ranges::end(std::get<_Np>(__parent_->__bases_))) {
          __it = ranges::begin(std::get<_Np>(__parent_->__bases_));
          next<_Np - 1>();
        }
      }
    }
    template <size_t _Np = sizeof...(_Vs)>
    constexpr void prev() {
      auto& __it = std::get<_Np>(__current_);
      if constexpr (_Np > 0) {
        if (__it == ranges::begin(std::get<_Np>(__parent_->__bases_))) {
          __it = __ycxx::__detail::__cartesian_common_arg_end(std::get<_Np>(__parent_->__bases_));
          prev<_Np - 1>();
        }
      }
      --__it;
    }
    // Moves the Nth iterator by x positions with carry into the (N-1)th (random access).
    template <size_t _Np = sizeof...(_Vs)>
    constexpr void advance(difference_type __x) {
      auto& __it = std::get<_Np>(__current_);
      if constexpr (_Np == 0) {
        __it += static_cast<range_difference_t<decltype(std::get<_Np>(__parent_->__bases_))>>(__x);
      } else {
        auto& base = std::get<_Np>(__parent_->__bases_);
        auto size = static_cast<difference_type>(ranges::size(base));
        if (size == 0)
          return;
        auto first = ranges::begin(base);
        difference_type __pos = static_cast<difference_type>(__it - first) + __x;
        difference_type __carry = __pos / size;
        __pos %= size;
        if (__pos < 0) {
          __pos += size;
          --__carry;
        }
        __it = first + static_cast<range_difference_t<decltype(base)>>(__pos);
        if (__carry != 0)
          advance<_Np - 1>(__carry);
      }
    }
    template <class _Tuple>
    constexpr difference_type __distance_from(const _Tuple& t) const {
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        difference_type sum = 0;
        ((sum += static_cast<difference_type>(std::get<_Ip>(__current_) - std::get<_Ip>(t)) * __scaled_size<_Ip + 1>()), ...);
        return sum;
      }(make_index_sequence<1 + sizeof...(_Vs)>{});
    }
    template <size_t _Np>
    constexpr difference_type __scaled_size() const {
      if constexpr (_Np <= sizeof...(_Vs))
        return static_cast<difference_type>(ranges::size(std::get<_Np>(__parent_->__bases_))) * __scaled_size<_Np + 1>();
      else
        return static_cast<difference_type>(1);
    }

    constexpr iterator(_Parent& __parent,
                       tuple<iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>,
                             iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>
                           current)
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(current)) {}

  public:
    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && (convertible_to<iterator_t<_First>, iterator_t<const _First>> && ... &&
                         convertible_to<iterator_t<_Vs>, iterator_t<const _Vs>>)
        : __parent_(i.__parent_), __current_(std::move(i.__current_)) {}

    constexpr auto operator*() const {
      return __ycxx::__detail::__tuple_transform([](auto& i) -> decltype(auto) { return *i; }, __current_);
    }
    constexpr iterator& operator++() {
      next();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires forward_range<__ycxx::__detail::__maybe_const<_Const, _First>>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires __ycxx::__detail::__cartesian_product_is_bidirectional<_Const, _First, _Vs...>
    {
      prev();
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ycxx::__detail::__cartesian_product_is_bidirectional<_Const, _First, _Vs...>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type __x)
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      if (__x != 0)
        advance(__x);
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      *this += -__x;
      return *this;
    }
    constexpr reference operator[](difference_type n) const
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      return *((*this) + n);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires equality_comparable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>>
    {
      return __x.__current_ == y.__current_;
    }
    friend constexpr bool operator==(const iterator& __x, default_sentinel_t) {
      return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        return ((std::get<_Ip>(__x.__current_) == ranges::end(std::get<_Ip>(__x.__parent_->__bases_))) || ...);
      }(make_index_sequence<1 + sizeof...(_Vs)>{});
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _First, _Vs...>
    {
      return __x.__current_ <=> y.__current_;
    }
    friend constexpr iterator operator+(const iterator& __x, difference_type y)
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      return iterator(__x) += y;
    }
    friend constexpr iterator operator+(difference_type __x, const iterator& y)
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      return y + __x;
    }
    friend constexpr iterator operator-(const iterator& __x, difference_type y)
      requires __ycxx::__detail::__cartesian_product_is_random_access<_Const, _First, _Vs...>
    {
      return iterator(__x) -= y;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__cartesian_is_sized_sentinel<_Const, __ycxx::__detail::__cartesian_iterator_t, _First, _Vs...>
    {
      return __x.__distance_from(y.__current_);
    }
    friend constexpr difference_type operator-(const iterator& i, default_sentinel_t)
      requires __ycxx::__detail::__cartesian_is_sized_sentinel<_Const, __ycxx::__detail::__cartesian_sentinel_t, _First, _Vs...>
    {
      auto __end_tuple = [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        return tuple(ranges::end(std::get<0>(i.__parent_->__bases_)), ranges::begin(std::get<_Ip + 1>(i.__parent_->__bases_))...);
      }(index_sequence_for<_Vs...>{});
      return i.__distance_from(__end_tuple);
    }
    friend constexpr difference_type operator-(default_sentinel_t s, const iterator& i)
      requires __ycxx::__detail::__cartesian_is_sized_sentinel<_Const, __ycxx::__detail::__cartesian_sentinel_t, _First, _Vs...>
    {
      return -(i - s);
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        (noexcept(ranges::iter_move(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>&>())) && ... &&
         noexcept(ranges::iter_move(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>&>()))) &&
        (is_nothrow_move_constructible_v<range_rvalue_reference_t<__ycxx::__detail::__maybe_const<_Const, _First>>> && ... &&
         is_nothrow_move_constructible_v<range_rvalue_reference_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>>)) {
      return __ycxx::__detail::__tuple_transform(ranges::iter_move, i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __l, const iterator& r) noexcept(
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>&>(),
                                    std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>&>())) &&
         ... &&
         noexcept(ranges::iter_swap(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>&>(),
                                    std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>&>()))))
      requires(indirectly_swappable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>> && ... &&
               indirectly_swappable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>>)
    {
      [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        (ranges::iter_swap(std::get<_Ip>(__l.__current_), std::get<_Ip>(r.__current_)), ...);
      }(make_index_sequence<1 + sizeof...(_Vs)>{});
    }
  };

  tuple<_First, _Vs...> __bases_;

  template <bool _Const, class _Self>
  static constexpr iterator<_Const> __end_impl(_Self& __self) {
    bool is_empty = [&]<size_t... _Ip>(index_sequence<_Ip...>) {
      return (ranges::empty(std::get<_Ip + 1>(__self.__bases_)) || ...);
    }(index_sequence_for<_Vs...>{});
    auto& first = std::get<0>(__self.__bases_);
    return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
      return iterator<_Const>(__self, tuple<iterator_t<__ycxx::__detail::__maybe_const<_Const, _First>>,
                                         iterator_t<__ycxx::__detail::__maybe_const<_Const, _Vs>>...>(
                                       is_empty ? ranges::begin(first) : __ycxx::__detail::__cartesian_common_arg_end(first),
                                       ranges::begin(std::get<_Ip + 1>(__self.__bases_))...));
    }(index_sequence_for<_Vs...>{});
  }
  template <class _Self>
  static constexpr auto __size_impl(_Self& __self) {
    using _Dp = typename iterator<is_const_v<_Self>>::difference_type;
    using _Up = make_unsigned_t<_Dp>;
    return std::apply([](auto&... __y_bases) { return (_Up(1) * ... * static_cast<_Up>(ranges::size(__y_bases))); }, __self.__bases_);
  }

public:
  constexpr cartesian_product_view() = default;
  constexpr explicit cartesian_product_view(_First __first_base, _Vs... __y_bases)
      : __bases_(std::move(__first_base), std::move(__y_bases)...) {}

  constexpr iterator<false> begin()
    requires(!__ycxx::__detail::__simple_view<_First> || ... || !__ycxx::__detail::__simple_view<_Vs>)
  {
    return iterator<false>(*this, __ycxx::__detail::__tuple_transform(ranges::begin, __bases_));
  }
  constexpr iterator<true> begin() const
    requires(range<const _First> && ... && range<const _Vs>)
  {
    return iterator<true>(*this, __ycxx::__detail::__tuple_transform(ranges::begin, __bases_));
  }
  constexpr iterator<false> end()
    requires((!__ycxx::__detail::__simple_view<_First> || ... || !__ycxx::__detail::__simple_view<_Vs>) &&
             __ycxx::__detail::__cartesian_product_is_common<_First, _Vs...>)
  {
    return __end_impl<false>(*this);
  }
  constexpr iterator<true> end() const
    requires __ycxx::__detail::__cartesian_product_is_common<const _First, const _Vs...>
  {
    return __end_impl<true>(*this);
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
  constexpr auto size()
    requires __ycxx::__detail::__cartesian_product_is_sized<_First, _Vs...>
  {
    return __size_impl(*this);
  }
  constexpr auto size() const
    requires __ycxx::__detail::__cartesian_product_is_sized<const _First, const _Vs...>
  {
    return __size_impl(*this);
  }
};
template <class... _Vs>
cartesian_product_view(_Vs&&...) -> cartesian_product_view<views::all_t<_Vs>...>;

}} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__view_fn {

template <std::size_t _Np>
struct __elements_fn : std::ranges::range_adaptor_closure<__elements_fn<_Np>> {
  template <class _Ep>
    requires requires { std::ranges::elements_view<std::views::all_t<_Ep>, _Np>{std::declval<_Ep>()}; }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    return std::ranges::elements_view<std::views::all_t<_Ep>, _Np>{static_cast<_Ep&&>(e)};
  }
};

struct __enumerate_fn : std::ranges::range_adaptor_closure<__enumerate_fn> {
  template <class _Ep>
    requires requires { std::ranges::enumerate_view<std::views::all_t<_Ep>>(std::declval<_Ep>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    return std::ranges::enumerate_view<std::views::all_t<_Ep>>(static_cast<_Ep&&>(e));
  }
};

struct __zip_fn {
  [[nodiscard]] constexpr auto operator()() const noexcept { return auto(std::views::empty<std::tuple<>>); }
  template <class... _Es>
    requires(sizeof...(_Es) > 0) &&
            requires { std::ranges::zip_view<std::views::all_t<_Es>...>(std::declval<_Es>()...); }
  [[nodiscard]] constexpr auto operator()(_Es&&... __es) const {
    return std::ranges::zip_view<std::views::all_t<_Es>...>(static_cast<_Es&&>(__es)...);
  }
};

struct __zip_transform_fn {
  template <class _Fp>
    requires std::move_constructible<std::decay_t<_Fp>> && std::regular_invocable<std::decay_t<_Fp>&> &&
             std::is_object_v<std::decay_t<std::invoke_result_t<std::decay_t<_Fp>&>>>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const {
    (void)__f;
    return auto(std::views::empty<std::decay_t<std::invoke_result_t<std::decay_t<_Fp>&>>>);
  }
  template <class _Fp, class... _Es>
    requires(sizeof...(_Es) > 0) && requires { std::ranges::zip_transform_view(std::declval<_Fp>(), std::declval<_Es>()...); }
  [[nodiscard]] constexpr auto operator()(_Fp&& __f, _Es&&... __es) const {
    return std::ranges::zip_transform_view(static_cast<_Fp&&>(__f), static_cast<_Es&&>(__es)...);
  }
};

template <std::size_t _Np>
struct __adjacent_fn : std::ranges::range_adaptor_closure<__adjacent_fn<_Np>> {
  template <class _Ep>
    requires(_Np == 0 && std::ranges::forward_range<_Ep>) ||
            requires { std::ranges::adjacent_view<std::views::all_t<_Ep>, _Np>(std::declval<_Ep>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    if constexpr (_Np == 0 && std::ranges::forward_range<_Ep>)
      return ((void)e, auto(std::views::empty<std::tuple<>>));
    else
      return std::ranges::adjacent_view<std::views::all_t<_Ep>, _Np>(static_cast<_Ep&&>(e));
  }
};

template <std::size_t _Np>
struct __adjacent_transform_fn {
  template <class _Ep, class _Fp>
    requires(_Np == 0 && std::ranges::forward_range<_Ep> && requires { __zip_transform_fn{}(std::declval<_Fp>()); }) ||
            requires {
              std::ranges::adjacent_transform_view<std::views::all_t<_Ep>, std::decay_t<_Fp>, _Np>(std::declval<_Ep>(),
                                                                                            std::declval<_Fp>());
            }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    if constexpr (_Np == 0 && std::ranges::forward_range<_Ep>)
      return ((void)e, __zip_transform_fn{}(static_cast<_Fp&&>(__f)));
    else
      return std::ranges::adjacent_transform_view<std::views::all_t<_Ep>, std::decay_t<_Fp>, _Np>(static_cast<_Ep&&>(e),
                                                                                           static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f));
  }
};

struct __cartesian_product_fn {
  [[nodiscard]] constexpr auto operator()() const { return std::views::single(std::tuple()); }
  template <class... _Es>
    requires(sizeof...(_Es) > 0) &&
            requires { std::ranges::cartesian_product_view<std::views::all_t<_Es>...>(std::declval<_Es>()...); }
  [[nodiscard]] constexpr auto operator()(_Es&&... __es) const {
    return std::ranges::cartesian_product_view<std::views::all_t<_Es>...>(static_cast<_Es&&>(__es)...);
  }
};

}} // namespace __ycxx::__detail::__view_fn

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges::views {
template <size_t _Np>
constexpr __ycxx::__detail::__view_fn::__elements_fn<_Np> elements{};
inline constexpr auto keys = elements<0>;
inline constexpr auto values = elements<1>;
inline constexpr __ycxx::__detail::__view_fn::__enumerate_fn enumerate{};
inline constexpr __ycxx::__detail::__view_fn::__zip_fn zip{};
inline constexpr __ycxx::__detail::__view_fn::__zip_transform_fn zip_transform{};
template <size_t _Np>
constexpr __ycxx::__detail::__view_fn::__adjacent_fn<_Np> adjacent{};
inline constexpr auto pairwise = adjacent<2>;
template <size_t _Np>
constexpr __ycxx::__detail::__view_fn::__adjacent_transform_fn<_Np> adjacent_transform{};
inline constexpr auto pairwise_transform = adjacent_transform<2>;
inline constexpr __ycxx::__detail::__view_fn::__cartesian_product_fn cartesian_product{};
}} // namespace std::ranges::views
