// libycxx core: the single-range adaptors of [range.adaptors]: as_rvalue, filter, transform,
// take, take_while, drop, drop_while, counted, common, reverse, as_const, cache_latest and
// as_input.
#pragma once

#include <ycxx/core/ranges_factories.hpp>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/string_view.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// (is_span, is_optional and is_subrange come from span.hpp, optional.hpp and pair.hpp.)
template <class _Tp>
inline constexpr bool __is_string_view = false;
template <class _Cp, class _Tp>
inline constexpr bool __is_string_view<std::basic_string_view<_Cp, _Tp>> = true;
template <class _Tp>
inline constexpr bool __is_empty_view = false;
template <class _Tp>
inline constexpr bool __is_empty_view<std::ranges::empty_view<_Tp>> = true;
// subrange<I, S, K>::StoreSize ([range.subrange.general]): sized only through a stored size.
// filter_view offers const iteration only for input ranges, whose begin() needs no cache.
template <class _Vp, class _Pred>
concept __filter_const_iterable = std::ranges::input_range<const _Vp> && !std::ranges::forward_range<const _Vp> &&
                                std::indirect_unary_predicate<const _Pred, std::ranges::iterator_t<const _Vp>>;
template <class _Tp>
inline constexpr bool __subrange_stores_size = false;
template <class _Ip, class _Sp, std::ranges::subrange_kind _Kp>
inline constexpr bool __subrange_stores_size<std::ranges::subrange<_Ip, _Sp, _Kp>> =
    _Kp == std::ranges::subrange_kind::sized && !std::sized_sentinel_for<_Sp, _Ip>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// =============================================================================================
// [range.as.rvalue]
// =============================================================================================
template <view _Vp>
  requires input_range<_Vp>
class as_rvalue_view : public view_interface<as_rvalue_view<_Vp>> {
  _Vp __base_ = _Vp();

public:
  as_rvalue_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit as_rvalue_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return move_iterator(ranges::begin(__base_));
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return move_iterator(ranges::begin(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (common_range<_Vp>)
      return move_iterator(ranges::end(__base_));
    else
      return move_sentinel(ranges::end(__base_));
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    if constexpr (common_range<const _Vp>)
      return move_iterator(ranges::end(__base_));
    else
      return move_sentinel(ranges::end(__base_));
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
template <class _Rp>
as_rvalue_view(_Rp&&) -> as_rvalue_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<as_rvalue_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.filter]
// =============================================================================================
template <input_range _Vp, indirect_unary_predicate<iterator_t<_Vp>> _Pred>
  requires view<_Vp> && is_object_v<_Pred>
class filter_view : public view_interface<filter_view<_Vp, _Pred>> {
  template <bool _Const>
  class iterator;
  template <bool _Const>
  class sentinel;


  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__movable_box<_Pred> __pred_;
  [[no_unique_address]] __ycxx::__detail::__position_cache_if<forward_range<_Vp>, _Vp> __begin_;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (derived_from<_Cp, bidirectional_iterator_tag>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<_Cp, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<_Cp>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend filter_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, filter_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    _Parent* __parent_ = nullptr;

    constexpr iterator(_Parent& __parent, iterator_t<_Base> current)
        : __current_(std::move(current)), __parent_(__builtin_addressof(__parent)) {}

  public:
    using iterator_concept =
        conditional_t<_Const, input_iterator_tag,
                      conditional_t<bidirectional_range<_Vp>, bidirectional_iterator_tag,
                                    conditional_t<forward_range<_Vp>, forward_iterator_tag, input_iterator_tag>>>;
    using value_type = range_value_t<_Base>;
    using difference_type = range_difference_t<_Base>;

    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)), __parent_(i.__parent_) {}

    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr range_reference_t<_Base> operator*() const { return *__current_; }
    constexpr iterator_t<_Base> operator->() const
      requires __ycxx::__detail::__has_arrow<iterator_t<_Base>> && copyable<iterator_t<_Base>>
    {
      return __current_;
    }

    constexpr iterator& operator++() {
      __current_ = ranges::find_if(std::move(++__current_), ranges::end(__parent_->__base_), std::ref(*__parent_->__pred_));
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
      do
        --__current_;
      while (!::__ycxx::__detail::invoke(*__parent_->__pred_, *__current_));
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Base>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires equality_comparable<iterator_t<_Base>>
    {
      return __x.__current_ == y.__current_;
    }
    friend constexpr range_rvalue_reference_t<_Base> iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(i.__current_))) {
      return ranges::iter_move(i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(__x.__current_, y.__current_)))
      requires indirectly_swappable<iterator_t<_Base>>
    {
      ranges::iter_swap(__x.__current_, y.__current_);
    }
  };

  template <bool _Const>
  class sentinel {
    friend filter_view;
    friend sentinel<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, filter_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(_Parent& __parent) : __end_(ranges::end(__parent.__base_)) {}

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
  };

public:
  filter_view()
    requires default_initializable<_Vp> && default_initializable<_Pred>
  = default;
  constexpr explicit filter_view(_Vp base, _Pred pred) : __base_(std::move(base)), __pred_(in_place, std::move(pred)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
  constexpr const _Pred& pred() const { return *__pred_; }

  constexpr iterator<false> begin() {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "filter_view::begin: no predicate");
    if constexpr (forward_range<_Vp>) {
      if (!__begin_.has_value())
        __begin_.set(__base_, ranges::find_if(__base_, std::ref(*__pred_)));
      return {*this, __begin_.get(__base_)};
    } else {
      return {*this, ranges::find_if(__base_, std::ref(*__pred_))};
    }
  }
  constexpr iterator<true> begin() const
    requires __ycxx::__detail::__filter_const_iterable<_Vp, _Pred>
  {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "filter_view::begin: no predicate");
    return {*this, ranges::find_if(__base_, std::ref(*__pred_))};
  }
  constexpr auto end() {
    if constexpr (common_range<_Vp>)
      return iterator<false>{*this, ranges::end(__base_)};
    else
      return sentinel<false>{*this};
  }
  constexpr sentinel<true> end() const
    requires __ycxx::__detail::__filter_const_iterable<_Vp, _Pred>
  {
    return sentinel<true>{*this};
  }
};
template <class _Rp, class _Pred>
filter_view(_Rp&&, _Pred) -> filter_view<views::all_t<_Rp>, _Pred>;

// =============================================================================================
// [range.transform]
// =============================================================================================
template <input_range _Vp, move_constructible _Fp>
  requires view<_Vp> && is_object_v<_Fp> && regular_invocable<_Fp&, range_reference_t<_Vp>> &&
           __ycxx::__detail::__can_reference<invoke_result_t<_Fp&, range_reference_t<_Vp>>>
class transform_view : public view_interface<transform_view<_Vp, _Fp>> {
  template <bool _Const>
  class iterator;
  template <bool _Const>
  class sentinel;

  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__movable_box<_Fp> __fun_;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (is_reference_v<invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&, range_reference_t<_Base>>>) {
        if constexpr (derived_from<_Cp, contiguous_iterator_tag>)
          return type_identity<random_access_iterator_tag>{};
        else
          return type_identity<_Cp>{};
      } else {
        return type_identity<input_iterator_tag>{};
      }
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend transform_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, transform_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    _Parent* __parent_ = nullptr;

    constexpr iterator(_Parent& __parent, iterator_t<_Base> current)
        : __current_(std::move(current)), __parent_(__builtin_addressof(__parent)) {}

  public:
    using iterator_concept = __ycxx::__detail::__range_strength_t<_Base>;
    using value_type = remove_cvref_t<invoke_result_t<__ycxx::__detail::__maybe_const<_Const, _Fp>&, range_reference_t<_Base>>>;
    using difference_type = range_difference_t<_Base>;

    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)), __parent_(i.__parent_) {}

    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr decltype(auto) operator*() const noexcept(noexcept(::__ycxx::__detail::invoke(*__parent_->__fun_, *__current_))) {
      return ::__ycxx::__detail::invoke(*__parent_->__fun_, *__current_);
    }

    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr void operator++(int) { ++__current_; }
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
      return ::__ycxx::__detail::invoke(*__parent_->__fun_, __current_[n]);
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
    friend constexpr iterator operator+(iterator i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator{*i.__parent_, i.__current_ + n};
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
      requires random_access_range<_Base>
    {
      return iterator{*i.__parent_, i.__current_ + n};
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
      requires random_access_range<_Base>
    {
      return iterator{*i.__parent_, i.__current_ - n};
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>>
    {
      return __x.__current_ - y.__current_;
    }
  };

  template <bool _Const>
  class sentinel {
    friend transform_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(sentinel_t<_Base> end) : __end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> i)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(i.__end_)) {}
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
    friend constexpr range_difference_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>> operator-(const sentinel& y,
                                                                                            const iterator<_OtherConst>& __x) {
      return y.__end_ - __ycxx::__detail::__view_access::current(__x);
    }
  };

public:
  transform_view()
    requires default_initializable<_Vp> && default_initializable<_Fp>
  = default;
  constexpr explicit transform_view(_Vp base, _Fp fun) : __base_(std::move(base)), __fun_(in_place, std::move(fun)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr iterator<false> begin() { return iterator<false>{*this, ranges::begin(__base_)}; }
  constexpr iterator<true> begin() const
    requires range<const _Vp> && regular_invocable<const _Fp&, range_reference_t<const _Vp>>
  {
    return iterator<true>{*this, ranges::begin(__base_)};
  }
  constexpr sentinel<false> end() { return sentinel<false>{ranges::end(__base_)}; }
  constexpr iterator<false> end()
    requires common_range<_Vp>
  {
    return iterator<false>{*this, ranges::end(__base_)};
  }
  constexpr sentinel<true> end() const
    requires range<const _Vp> && regular_invocable<const _Fp&, range_reference_t<const _Vp>>
  {
    return sentinel<true>{ranges::end(__base_)};
  }
  constexpr iterator<true> end() const
    requires common_range<const _Vp> && regular_invocable<const _Fp&, range_reference_t<const _Vp>>
  {
    return iterator<true>{*this, ranges::end(__base_)};
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
template <class _Rp, class _Fp>
transform_view(_Rp&&, _Fp) -> transform_view<views::all_t<_Rp>, _Fp>;

// =============================================================================================
// [range.take]
// =============================================================================================
template <view _Vp>
class take_view : public view_interface<take_view<_Vp>> {
  template <bool _Const>
  class sentinel {
    friend take_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    template <bool _OtherConst>
    using _CI = counted_iterator<iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(sentinel_t<_Base> end) : __end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> s)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(s.__end_)) {}
    constexpr sentinel_t<_Base> base() const { return __end_; }

    friend constexpr bool operator==(const _CI<_Const>& y, const sentinel& __x) {
      return y.count() == 0 || y.base() == __x.__end_;
    }
    template <bool _OtherConst = !_Const>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const _CI<_OtherConst>& y, const sentinel& __x) {
      return y.count() == 0 || y.base() == __x.__end_;
    }
  };

  _Vp __base_ = _Vp();
  range_difference_t<_Vp> __count_ = 0;

  template <class _Self>
  static constexpr auto __begin_impl(_Self& __self) {
    using _Bp = __ycxx::__detail::__maybe_const<is_const_v<_Self>, _Vp>;
    if constexpr (sized_range<_Bp>) {
      if constexpr (random_access_range<_Bp>) {
        return ranges::begin(__self.__base_);
      } else {
        auto __sz = range_difference_t<_Bp>(__self.size());
        return counted_iterator(ranges::begin(__self.__base_), __sz);
      }
    } else if constexpr (sized_sentinel_for<sentinel_t<_Bp>, iterator_t<_Bp>>) {
      auto __it = ranges::begin(__self.__base_);
      auto __sz = std::min(__self.__count_, ranges::end(__self.__base_) - __it);
      return counted_iterator(std::move(__it), __sz);
    } else {
      return counted_iterator(ranges::begin(__self.__base_), __self.__count_);
    }
  }
  template <class _Self>
  static constexpr auto __end_impl(_Self& __self) {
    using _Bp = __ycxx::__detail::__maybe_const<is_const_v<_Self>, _Vp>;
    if constexpr (sized_range<_Bp>) {
      if constexpr (random_access_range<_Bp>)
        return ranges::begin(__self.__base_) + range_difference_t<_Bp>(__self.size());
      else
        return default_sentinel;
    } else if constexpr (sized_sentinel_for<sentinel_t<_Bp>, iterator_t<_Bp>>) {
      return default_sentinel;
    } else {
      return sentinel<is_const_v<_Self>>{ranges::end(__self.__base_)};
    }
  }

public:
  take_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit take_view(_Vp base, range_difference_t<_Vp> count) : __base_(std::move(base)), __count_(count) {
    ::__ycxx::__detail::__precondition(count >= 0, "take_view: negative count");
  }

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return __begin_impl(*this);
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return __begin_impl(*this);
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return __end_impl(*this);
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    return __end_impl(*this);
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    auto n = ranges::size(__base_);
    return ranges::min(n, static_cast<decltype(n)>(__count_));
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    auto n = ranges::size(__base_);
    return ranges::min(n, static_cast<decltype(n)>(__count_));
  }
  constexpr auto reserve_hint() {
    if constexpr (approximately_sized_range<_Vp>) {
      auto n = static_cast<range_difference_t<_Vp>>(ranges::reserve_hint(__base_));
      return ::__ycxx::__detail::__to_unsigned_like(ranges::min(n, __count_));
    } else {
      return ::__ycxx::__detail::__to_unsigned_like(__count_);
    }
  }
  constexpr auto reserve_hint() const {
    if constexpr (approximately_sized_range<const _Vp>) {
      auto n = static_cast<range_difference_t<const _Vp>>(ranges::reserve_hint(__base_));
      return ::__ycxx::__detail::__to_unsigned_like(ranges::min(n, __count_));
    } else {
      return ::__ycxx::__detail::__to_unsigned_like(__count_);
    }
  }
};
template <class _Rp>
take_view(_Rp&&, range_difference_t<_Rp>) -> take_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<take_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.take.while]
// =============================================================================================
template <view _Vp, class _Pred>
  requires input_range<_Vp> && is_object_v<_Pred> && indirect_unary_predicate<const _Pred, iterator_t<_Vp>>
class take_while_view : public view_interface<take_while_view<_Vp, _Pred>> {
  template <bool _Const>
  class sentinel {
    friend take_while_view;
    friend sentinel<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    const _Pred* __pred_ = nullptr;
    constexpr explicit sentinel(sentinel_t<_Base> end, const _Pred* pred) : __end_(end), __pred_(pred) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> s)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(s.__end_)), __pred_(s.__pred_) {}
    constexpr sentinel_t<_Base> base() const { return __end_; }

    friend constexpr bool operator==(const iterator_t<_Base>& __x, const sentinel& y) {
      return y.__end_ == __x || !::__ycxx::__detail::invoke(*y.__pred_, *__x);
    }
    template <bool _OtherConst = !_Const>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>& __x, const sentinel& y) {
      return y.__end_ == __x || !::__ycxx::__detail::invoke(*y.__pred_, *__x);
    }
  };

  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__movable_box<_Pred> __pred_;

public:
  take_while_view()
    requires default_initializable<_Vp> && default_initializable<_Pred>
  = default;
  constexpr explicit take_while_view(_Vp base, _Pred pred) : __base_(std::move(base)), __pred_(in_place, std::move(pred)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
  constexpr const _Pred& pred() const { return *__pred_; }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return ranges::begin(__base_);
  }
  constexpr auto begin() const
    requires range<const _Vp> && indirect_unary_predicate<const _Pred, iterator_t<const _Vp>>
  {
    return ranges::begin(__base_);
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return sentinel<false>(ranges::end(__base_), __builtin_addressof(*__pred_));
  }
  constexpr auto end() const
    requires range<const _Vp> && indirect_unary_predicate<const _Pred, iterator_t<const _Vp>>
  {
    return sentinel<true>(ranges::end(__base_), __builtin_addressof(*__pred_));
  }
};
template <class _Rp, class _Pred>
take_while_view(_Rp&&, _Pred) -> take_while_view<views::all_t<_Rp>, _Pred>;

// =============================================================================================
// [range.drop]
// =============================================================================================
template <view _Vp>
class drop_view : public view_interface<drop_view<_Vp>> {
  // begin() is cached for forward ranges whose start cannot be computed in O(1).
  static constexpr bool __caches = forward_range<_Vp> && !(random_access_range<_Vp> && sized_range<_Vp>);

  _Vp __base_ = _Vp();
  range_difference_t<_Vp> __count_ = 0;
  [[no_unique_address]] __ycxx::__detail::__position_cache_if<__caches, _Vp> __begin_;

public:
  drop_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit drop_view(_Vp base, range_difference_t<_Vp> count) : __base_(std::move(base)), __count_(count) {
    ::__ycxx::__detail::__precondition(count >= 0, "drop_view: negative count");
  }

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!(__ycxx::__detail::__simple_view<_Vp> && random_access_range<const _Vp> && sized_range<const _Vp>))
  {
    if constexpr (random_access_range<_Vp> && sized_range<_Vp>) {
      return ranges::begin(__base_) + std::min<range_difference_t<_Vp>>(__count_, ranges::distance(__base_));
    } else if constexpr (__caches) {
      if (!__begin_.has_value())
        __begin_.set(__base_, ranges::next(ranges::begin(__base_), __count_, ranges::end(__base_)));
      return __begin_.get(__base_);
    } else {
      return ranges::next(ranges::begin(__base_), __count_, ranges::end(__base_));
    }
  }
  // For sized random-access ranges ranges::next(begin, count_, end) is computed in O(1) without
  // comparing against the sentinel.
  constexpr auto begin() const
    requires random_access_range<const _Vp> && sized_range<const _Vp>
  {
    return ranges::begin(__base_) + std::min<range_difference_t<const _Vp>>(__count_, ranges::distance(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return ranges::end(__base_);
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    return ranges::end(__base_);
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    const auto s = ranges::size(__base_);
    const auto c = static_cast<decltype(s)>(__count_);
    return s < c ? 0 : s - c;
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    const auto s = ranges::size(__base_);
    const auto c = static_cast<decltype(s)>(__count_);
    return s < c ? 0 : s - c;
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    const auto s = static_cast<range_difference_t<_Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(s < __count_ ? 0 : s - __count_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    const auto s = static_cast<range_difference_t<const _Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(s < __count_ ? 0 : s - __count_);
  }
};
template <class _Rp>
drop_view(_Rp&&, range_difference_t<_Rp>) -> drop_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<drop_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.drop.while]
// =============================================================================================
template <view _Vp, class _Pred>
  requires input_range<_Vp> && is_object_v<_Pred> && indirect_unary_predicate<const _Pred, iterator_t<_Vp>>
class drop_while_view : public view_interface<drop_while_view<_Vp, _Pred>> {
  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__movable_box<_Pred> __pred_;
  [[no_unique_address]] __ycxx::__detail::__position_cache_if<forward_range<_Vp>, _Vp> __begin_;

public:
  drop_while_view()
    requires default_initializable<_Vp> && default_initializable<_Pred>
  = default;
  constexpr explicit drop_while_view(_Vp base, _Pred pred) : __base_(std::move(base)), __pred_(in_place, std::move(pred)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
  constexpr const _Pred& pred() const { return *__pred_; }

  constexpr auto begin() {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "drop_while_view::begin: no predicate");
    if constexpr (forward_range<_Vp>) {
      if (!__begin_.has_value())
        __begin_.set(__base_, ranges::find_if_not(__base_, std::cref(*__pred_)));
      return __begin_.get(__base_);
    } else {
      return ranges::find_if_not(__base_, std::cref(*__pred_));
    }
  }
  constexpr auto end() { return ranges::end(__base_); }
};
template <class _Rp, class _Pred>
drop_while_view(_Rp&&, _Pred) -> drop_while_view<views::all_t<_Rp>, _Pred>;
template <class _Tp, class _Pred>
constexpr bool enable_borrowed_range<drop_while_view<_Tp, _Pred>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.common]
// =============================================================================================
template <view _Vp>
  requires(!common_range<_Vp> && copyable<iterator_t<_Vp>>)
class common_view : public view_interface<common_view<_Vp>> {
  _Vp __base_ = _Vp();

public:
  common_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit common_view(_Vp r) : __base_(std::move(r)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (random_access_range<_Vp> && sized_range<_Vp>)
      return ranges::begin(__base_);
    else
      return common_iterator<iterator_t<_Vp>, sentinel_t<_Vp>>(ranges::begin(__base_));
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    if constexpr (random_access_range<const _Vp> && sized_range<const _Vp>)
      return ranges::begin(__base_);
    else
      return common_iterator<iterator_t<const _Vp>, sentinel_t<const _Vp>>(ranges::begin(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (random_access_range<_Vp> && sized_range<_Vp>)
      return ranges::begin(__base_) + ranges::distance(__base_);
    else
      return common_iterator<iterator_t<_Vp>, sentinel_t<_Vp>>(ranges::end(__base_));
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    if constexpr (random_access_range<const _Vp> && sized_range<const _Vp>)
      return ranges::begin(__base_) + ranges::distance(__base_);
    else
      return common_iterator<iterator_t<const _Vp>, sentinel_t<const _Vp>>(ranges::end(__base_));
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
template <class _Rp>
common_view(_Rp&&) -> common_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<common_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.reverse]
// =============================================================================================
template <view _Vp>
  requires bidirectional_range<_Vp>
class reverse_view : public view_interface<reverse_view<_Vp>> {
  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__position_cache_if<!common_range<_Vp>, _Vp> __begin_;

public:
  reverse_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit reverse_view(_Vp r) : __base_(std::move(r)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr reverse_iterator<iterator_t<_Vp>> begin() {
    if (!__begin_.has_value())
      __begin_.set(__base_, ranges::next(ranges::begin(__base_), ranges::end(__base_)));
    return std::make_reverse_iterator(__begin_.get(__base_));
  }
  constexpr reverse_iterator<iterator_t<_Vp>> begin()
    requires common_range<_Vp>
  {
    return std::make_reverse_iterator(ranges::end(__base_));
  }
  constexpr auto begin() const
    requires common_range<const _Vp>
  {
    return std::make_reverse_iterator(ranges::end(__base_));
  }
  constexpr reverse_iterator<iterator_t<_Vp>> end() { return std::make_reverse_iterator(ranges::begin(__base_)); }
  constexpr auto end() const
    requires common_range<const _Vp>
  {
    return std::make_reverse_iterator(ranges::begin(__base_));
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
template <class _Rp>
reverse_view(_Rp&&) -> reverse_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<reverse_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.as.const]
// =============================================================================================
template <view _Vp>
  requires input_range<_Vp>
class as_const_view : public view_interface<as_const_view<_Vp>> {
  _Vp __base_ = _Vp();

public:
  as_const_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit as_const_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return ranges::cbegin(__base_);
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return ranges::cbegin(__base_);
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return ranges::cend(__base_);
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    return ranges::cend(__base_);
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
template <class _Rp>
as_const_view(_Rp&&) -> as_const_view<views::all_t<_Rp>>;
template <class _Tp>
constexpr bool enable_borrowed_range<as_const_view<_Tp>> = enable_borrowed_range<_Tp>;

// =============================================================================================
// [range.cache.latest]
// =============================================================================================
template <input_range _Vp>
  requires view<_Vp>
class cache_latest_view : public view_interface<cache_latest_view<_Vp>> {
  using __cache_t = conditional_t<is_reference_v<range_reference_t<_Vp>>, add_pointer_t<range_reference_t<_Vp>>,
                                range_reference_t<_Vp>>;

  class sentinel;
  class iterator {
    friend cache_latest_view;
    friend __ycxx::__detail::__view_access;
    cache_latest_view* __parent_;
    iterator_t<_Vp> __current_;

    constexpr explicit iterator(cache_latest_view& __parent)
        : __parent_(__builtin_addressof(__parent)), __current_(ranges::begin(__parent.__base_)) {}

  public:
    using difference_type = range_difference_t<_Vp>;
    using value_type = range_value_t<_Vp>;
    using iterator_concept = input_iterator_tag;

    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;

    constexpr iterator_t<_Vp> base() && { return std::move(__current_); }
    constexpr const iterator_t<_Vp>& base() const& noexcept { return __current_; }

    constexpr range_reference_t<_Vp>& operator*() const {
      if constexpr (is_reference_v<range_reference_t<_Vp>>) {
        if (!__parent_->__cache_.has_value())
          __parent_->__cache_.emplace(__builtin_addressof(__ycxx::__detail::__as_lvalue(*__current_)));
        return **__parent_->__cache_;
      } else {
        if (!__parent_->__cache_.has_value())
          __parent_->__cache_.__emplace_deref(__current_);
        return *__parent_->__cache_;
      }
    }
    constexpr iterator& operator++() {
      __parent_->__cache_.reset();
      ++__current_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr range_rvalue_reference_t<_Vp> iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(i.__current_))) {
      return ranges::iter_move(i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(__x.__current_, y.__current_)))
      requires indirectly_swappable<iterator_t<_Vp>>
    {
      ranges::iter_swap(__x.__current_, y.__current_);
    }
  };

  class sentinel {
    friend cache_latest_view;
    sentinel_t<_Vp> __end_ = sentinel_t<_Vp>();
    constexpr explicit sentinel(cache_latest_view& __parent) : __end_(ranges::end(__parent.__base_)) {}

  public:
    sentinel() = default;
    constexpr sentinel_t<_Vp> base() const { return __end_; }
    friend constexpr bool operator==(const iterator& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__end_;
    }
    friend constexpr range_difference_t<_Vp> operator-(const iterator& __x, const sentinel& y)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return __ycxx::__detail::__view_access::current(__x) - y.__end_;
    }
    friend constexpr range_difference_t<_Vp> operator-(const sentinel& __x, const iterator& y)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return __x.__end_ - __ycxx::__detail::__view_access::current(y);
    }
  };

  _Vp __base_ = _Vp();
  __ycxx::__detail::__non_propagating_cache<__cache_t> __cache_;

public:
  cache_latest_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit cache_latest_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin() { return iterator(*this); }
  constexpr auto end() { return sentinel(*this); }
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
template <class _Rp>
cache_latest_view(_Rp&&) -> cache_latest_view<views::all_t<_Rp>>;

// =============================================================================================
// [range.as.input]
// =============================================================================================
template <input_range _Vp>
  requires view<_Vp>
class as_input_view : public view_interface<as_input_view<_Vp>> {
  template <bool _Const>
  class iterator {
    friend as_input_view;
    friend iterator<!_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    constexpr explicit iterator(iterator_t<_Base> current) : __current_(std::move(current)) {}

  public:
    using difference_type = range_difference_t<_Base>;
    using value_type = range_value_t<_Base>;
    using iterator_concept = input_iterator_tag;

    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)) {}

    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr decltype(auto) operator*() const { return *__current_; }
    constexpr iterator& operator++() {
      ++__current_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const iterator& __x, const sentinel_t<_Base>& y) { return __x.__current_ == y; }
    friend constexpr difference_type operator-(const sentinel_t<_Base>& y, const iterator& __x)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return y - __x.__current_;
    }
    friend constexpr difference_type operator-(const iterator& __x, const sentinel_t<_Base>& y)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return __x.__current_ - y;
    }
    friend constexpr range_rvalue_reference_t<_Base> iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(i.__current_))) {
      return ranges::iter_move(i.__current_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(__x.__current_, y.__current_)))
      requires indirectly_swappable<iterator_t<_Base>>
    {
      ranges::iter_swap(__x.__current_, y.__current_);
    }
  };

  _Vp __base_ = _Vp();

public:
  as_input_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit as_input_view(_Vp base) : __base_(std::move(base)) {}

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
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return ranges::end(__base_);
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    return ranges::end(__base_);
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
template <class _Rp>
as_input_view(_Rp&&) -> as_input_view<views::all_t<_Rp>>;
template <class _Vp>
constexpr bool enable_borrowed_range<as_input_view<_Vp>> = enable_borrowed_range<_Vp>;

}} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__view_fn {

struct __as_rvalue_fn : std::ranges::range_adaptor_closure<__as_rvalue_fn> {
  template <class _Ep>
    requires requires { std::ranges::as_rvalue_view(std::declval<_Ep>()); } ||
             (std::ranges::input_range<_Ep> &&
              std::same_as<std::ranges::range_rvalue_reference_t<_Ep>, std::ranges::range_reference_t<_Ep>> &&
              requires { std::views::all(std::declval<_Ep>()); })
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    if constexpr (std::ranges::input_range<_Ep> &&
                  std::same_as<std::ranges::range_rvalue_reference_t<_Ep>, std::ranges::range_reference_t<_Ep>>)
      return std::views::all(static_cast<_Ep&&>(e));
    else
      return std::ranges::as_rvalue_view(static_cast<_Ep&&>(e));
  }
};

struct __filter_fn {
  template <class _Ep, class _Pp>
    requires requires { std::ranges::filter_view(std::declval<_Ep>(), std::declval<_Pp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Pp&& p) const {
    return std::ranges::filter_view(static_cast<_Ep&&>(e), static_cast<_Pp&&>(p));
  }
  template <class _Pp>
  [[nodiscard]] constexpr auto operator()(_Pp&& p) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p));
  }
};

struct __transform_fn {
  template <class _Ep, class _Fp>
    requires requires { std::ranges::transform_view(std::declval<_Ep>(), std::declval<_Fp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    return std::ranges::transform_view(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f));
  }
};

// views::take / views::drop: the type-preserving cases of [range.take.overview]/2 and
// [range.drop.overview]/2.
template <class _Ep, class _Fp>
concept __take_drop_args = std::ranges::viewable_range<_Ep> && std::convertible_to<_Fp, std::ranges::range_difference_t<_Ep>>;

template <class _Tp>
concept __sized_ra = std::ranges::random_access_range<_Tp> && std::ranges::sized_range<_Tp>;

struct __take_fn {
  template <class _Ep, class _Fp>
    requires __take_drop_args<_Ep, _Fp>
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    using _Tp = std::remove_cvref_t<_Ep>;
    using _Dp = std::ranges::range_difference_t<_Ep>;
    if constexpr (__is_empty_view<_Tp>) {
      (void)__f;
      return ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e));
    } else if constexpr (__is_optional<_Tp> && std::ranges::view<_Tp>) {
      return static_cast<_Dp>(static_cast<_Fp&&>(__f)) == _Dp() ? ((void)e, _Tp()) : ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e));
    } else if constexpr (__sized_ra<_Tp> && (__is_span<_Tp> || __is_string_view<_Tp> || __is_subrange<_Tp>)) {
      auto&& r = e;
      auto first = std::ranges::begin(r);
      auto n = std::min<_Dp>(std::ranges::distance(r), static_cast<_Dp>(static_cast<_Fp&&>(__f)));
      if constexpr (__is_span<_Tp>)
        return std::span<typename _Tp::element_type>(first, first + n);
      else if constexpr (__is_string_view<_Tp>)
        return _Tp(first, first + n);
      else
        return std::ranges::subrange<std::ranges::iterator_t<_Tp>>(first, first + n);
    } else if constexpr (__is_iota_view<_Tp> && __sized_ra<_Tp>) {
      auto&& r = e;
      auto first = std::ranges::begin(r);
      auto n = std::min<_Dp>(std::ranges::distance(r), static_cast<_Dp>(static_cast<_Fp&&>(__f)));
      return std::ranges::iota_view(*first, *(first + n));
    } else if constexpr (__is_repeat_view<_Tp>) {
      if constexpr (std::ranges::sized_range<_Tp>) {
        auto&& r = e;
        auto n = std::min<_Dp>(std::ranges::distance(r), static_cast<_Dp>(static_cast<_Fp&&>(__f)));
        return std::views::repeat(__repeat_access::value(static_cast<_Ep&&>(r)), n);
      } else {
        return std::views::repeat(__repeat_access::value(static_cast<_Ep&&>(e)), static_cast<_Dp>(static_cast<_Fp&&>(__f)));
      }
    } else {
      return std::ranges::take_view(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
    }
  }
  template <class _Fp>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f));
  }
};

struct __drop_fn {
  template <class _Ep, class _Fp>
    requires __take_drop_args<_Ep, _Fp>
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    using _Tp = std::remove_cvref_t<_Ep>;
    using _Dp = std::ranges::range_difference_t<_Ep>;
    if constexpr (__is_empty_view<_Tp>) {
      (void)__f;
      return ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e));
    } else if constexpr (__is_optional<_Tp> && std::ranges::view<_Tp>) {
      return static_cast<_Dp>(static_cast<_Fp&&>(__f)) == _Dp() ? ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e)) : ((void)e, _Tp());
    } else if constexpr (__sized_ra<_Tp> &&
                         (__is_span<_Tp> || __is_string_view<_Tp> || __is_iota_view<_Tp> ||
                          (__is_subrange<_Tp> && !__subrange_stores_size<_Tp>))) {
      auto&& r = e;
      auto n = std::min<_Dp>(std::ranges::distance(r), static_cast<_Dp>(static_cast<_Fp&&>(__f)));
      if constexpr (__is_span<_Tp>)
        return std::span<typename _Tp::element_type>(std::ranges::begin(r) + n, std::ranges::end(r));
      else
        return _Tp(std::ranges::begin(r) + n, std::ranges::end(r));
    } else if constexpr (__is_subrange<_Tp> && __sized_ra<_Tp>) {
      auto&& r = e;
      auto d = std::ranges::distance(r);
      auto n = std::min<_Dp>(d, static_cast<_Dp>(static_cast<_Fp&&>(__f)));
      return _Tp(std::ranges::begin(r) + n, std::ranges::end(r), ::__ycxx::__detail::__to_unsigned_like(d - n));
    } else if constexpr (__is_repeat_view<_Tp>) {
      if constexpr (std::ranges::sized_range<_Tp>) {
        auto&& r = e;
        auto d = std::ranges::distance(r);
        return std::views::repeat(__repeat_access::value(static_cast<_Ep&&>(r)), d - std::min<_Dp>(d, static_cast<_Dp>(static_cast<_Fp&&>(__f))));
      } else {
        (void)__f;
        return ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e));
      }
    } else {
      return std::ranges::drop_view(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
    }
  }
  template <class _Fp>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f));
  }
};

struct __take_while_fn {
  template <class _Ep, class _Pp>
    requires requires { std::ranges::take_while_view(std::declval<_Ep>(), std::declval<_Pp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Pp&& p) const {
    return std::ranges::take_while_view(static_cast<_Ep&&>(e), static_cast<_Pp&&>(p));
  }
  template <class _Pp>
  [[nodiscard]] constexpr auto operator()(_Pp&& p) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p));
  }
};

struct __drop_while_fn {
  template <class _Ep, class _Pp>
    requires requires { std::ranges::drop_while_view(std::declval<_Ep>(), std::declval<_Pp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Pp&& p) const {
    return std::ranges::drop_while_view(static_cast<_Ep&&>(e), static_cast<_Pp&&>(p));
  }
  template <class _Pp>
  [[nodiscard]] constexpr auto operator()(_Pp&& p) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p));
  }
};

// The selected form of views::counted(E, F) is well-formed.
template <class _Ep, class _Fp>
concept __counted_ok =
    std::input_or_output_iterator<std::decay_t<_Ep>> && std::convertible_to<_Fp, std::iter_difference_t<std::decay_t<_Ep>>> &&
    (std::contiguous_iterator<std::decay_t<_Ep>> ||
     (std::random_access_iterator<std::decay_t<_Ep>> && std::constructible_from<std::decay_t<_Ep>, _Ep>) ||
     requires(_Ep&& e, std::iter_difference_t<std::decay_t<_Ep>> n) { std::counted_iterator(static_cast<_Ep&&>(e), n); });

struct __counted_fn {
  template <class _Ep, class _Fp>
    requires __counted_ok<_Ep, _Fp>
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    using _Tp = std::decay_t<_Ep>;
    using _Dp = std::iter_difference_t<_Tp>;
    if constexpr (std::contiguous_iterator<_Tp>) {
      return std::span(std::to_address(e), static_cast<std::size_t>(static_cast<_Dp>(static_cast<_Fp&&>(__f))));
    } else if constexpr (std::random_access_iterator<_Tp>) {
      _Tp __it = static_cast<_Ep&&>(e);
      auto last = __it + static_cast<_Dp>(static_cast<_Fp&&>(__f));
      return std::ranges::subrange(std::move(__it), std::move(last));
    } else {
      return std::ranges::subrange(std::counted_iterator(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)), std::default_sentinel);
    }
  }
};

struct __common_fn : std::ranges::range_adaptor_closure<__common_fn> {
  template <class _Ep>
    requires(std::ranges::common_range<_Ep> && requires { std::views::all(std::declval<_Ep>()); }) ||
            requires { std::ranges::common_view{std::declval<_Ep>()}; }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    if constexpr (std::ranges::common_range<_Ep> && requires { std::views::all(std::declval<_Ep>()); })
      return std::views::all(static_cast<_Ep&&>(e));
    else
      return std::ranges::common_view{static_cast<_Ep&&>(e)};
  }
};

template <class _Tp>
inline constexpr bool __is_reverse_view = false;
template <class _Vp>
inline constexpr bool __is_reverse_view<std::ranges::reverse_view<_Vp>> = true;
template <class _Tp>
inline constexpr bool __is_reversed_subrange = false;
template <class _Ip, std::ranges::subrange_kind _Kp>
inline constexpr bool __is_reversed_subrange<std::ranges::subrange<std::reverse_iterator<_Ip>, std::reverse_iterator<_Ip>, _Kp>> =
    true;
template <class _Tp>
struct __reversed_subrange;
template <class _Ip, std::ranges::subrange_kind _Kp>
struct __reversed_subrange<std::ranges::subrange<std::reverse_iterator<_Ip>, std::reverse_iterator<_Ip>, _Kp>> {
  using type = std::ranges::subrange<_Ip, _Ip, _Kp>;
  static constexpr bool sized = _Kp == std::ranges::subrange_kind::sized;
};

struct __reverse_fn : std::ranges::range_adaptor_closure<__reverse_fn> {
  template <class _Ep>
    requires __is_reverse_view<std::remove_cvref_t<_Ep>> ||
             (__is_optional<std::remove_cvref_t<_Ep>> && std::ranges::view<std::remove_cvref_t<_Ep>>) ||
             __is_reversed_subrange<std::remove_cvref_t<_Ep>> || requires { std::ranges::reverse_view{std::declval<_Ep>()}; }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    using _Tp = std::remove_cvref_t<_Ep>;
    if constexpr (__is_reverse_view<_Tp>) {
      return static_cast<_Ep&&>(e).base();
    } else if constexpr (__is_optional<_Tp> && std::ranges::view<_Tp>) {
      return ::__ycxx::__detail::__decay_copy(static_cast<_Ep&&>(e));
    } else if constexpr (__is_reversed_subrange<_Tp>) {
      using _Rp = __reversed_subrange<_Tp>;
      auto&& r = e;
      if constexpr (_Rp::sized)
        return typename _Rp::type(r.end().base(), r.begin().base(), r.size());
      else
        return typename _Rp::type(r.end().base(), r.begin().base());
    } else {
      return std::ranges::reverse_view{static_cast<_Ep&&>(e)};
    }
  }
};

template <class _Tp>
struct __empty_view_elem;
template <class _Xp>
struct __empty_view_elem<std::ranges::empty_view<_Xp>> {
  using type = _Xp;
};
template <class _Tp>
inline constexpr bool __is_optional_ref = false;
template <class _Xp>
inline constexpr bool __is_optional_ref<std::optional<_Xp&>> = true;
template <class _Tp>
inline constexpr bool __is_ref_view = false;
template <class _Xp>
inline constexpr bool __is_ref_view<std::ranges::ref_view<_Xp>> = true;

template <class _Tp>
concept __all_t_constant = std::ranges::viewable_range<_Tp> && std::ranges::constant_range<std::views::all_t<_Tp>>;
template <class _Up>
concept __ref_view_of_constant =
    __is_ref_view<_Up> && std::ranges::constant_range<const std::remove_reference_t<decltype(std::declval<_Up&>().base())>>;

struct __as_const_fn : std::ranges::range_adaptor_closure<__as_const_fn> {
  template <class _Ep>
  static consteval int kind() {
    using _Tp = _Ep;
    using _Up = std::remove_cvref_t<_Tp>;
    if constexpr (__all_t_constant<_Tp>)
      return 1;
    else if constexpr (__is_empty_view<_Up>)
      return 2;
    else if constexpr (__is_optional_ref<_Up>)
      return 3;
    else if constexpr (__is_span<_Up>)
      return 4;
    else if constexpr (__ref_view_of_constant<_Up>)
      return 5;
    else if constexpr (std::is_lvalue_reference_v<_Ep> && std::ranges::constant_range<const _Up> && !std::ranges::view<_Up>)
      return 6;
    else if constexpr (requires { std::ranges::as_const_view(std::declval<_Ep>()); })
      return 7;
    else
      return 0;
  }
  template <class _Ep>
    requires(kind<_Ep>() != 0)
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    using _Up = std::remove_cvref_t<_Ep>;
    constexpr int k = kind<_Ep>();
    if constexpr (k == 1) {
      return std::views::all(static_cast<_Ep&&>(e));
    } else if constexpr (k == 2) {
      return auto(std::views::empty<const typename __empty_view_elem<_Up>::type>);
    } else if constexpr (k == 3) {
      return std::optional<const std::remove_reference_t<decltype(*e)>&>(static_cast<_Ep&&>(e));
    } else if constexpr (k == 4) {
      return std::span<const typename _Up::element_type, _Up::extent>(static_cast<_Ep&&>(e));
    } else if constexpr (k == 5) {
      using _Xp = std::remove_reference_t<decltype(e.base())>;
      return std::ranges::ref_view(static_cast<const _Xp&>(e.base()));
    } else if constexpr (k == 6) {
      return std::ranges::ref_view(static_cast<const _Up&>(e));
    } else {
      return std::ranges::as_const_view(static_cast<_Ep&&>(e));
    }
  }
};

struct __cache_latest_fn : std::ranges::range_adaptor_closure<__cache_latest_fn> {
  template <class _Ep>
    requires requires { std::ranges::cache_latest_view(std::declval<_Ep>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    return std::ranges::cache_latest_view(static_cast<_Ep&&>(e));
  }
};

struct __as_input_fn : std::ranges::range_adaptor_closure<__as_input_fn> {
  template <class _Ep>
    requires(std::ranges::input_range<_Ep> && !std::ranges::common_range<_Ep> && !std::ranges::forward_range<_Ep> &&
             requires { std::views::all(std::declval<_Ep>()); }) ||
            requires { std::ranges::as_input_view(std::declval<_Ep>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    if constexpr (std::ranges::input_range<_Ep> && !std::ranges::common_range<_Ep> && !std::ranges::forward_range<_Ep>)
      return std::views::all(static_cast<_Ep&&>(e));
    else
      return std::ranges::as_input_view(static_cast<_Ep&&>(e));
  }
};

}} // namespace __ycxx::__detail::__view_fn

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges::views {
inline constexpr __ycxx::__detail::__view_fn::__as_rvalue_fn as_rvalue{};
inline constexpr __ycxx::__detail::__view_fn::__filter_fn filter{};
inline constexpr __ycxx::__detail::__view_fn::__transform_fn transform{};
inline constexpr __ycxx::__detail::__view_fn::__take_fn take{};
inline constexpr __ycxx::__detail::__view_fn::__take_while_fn take_while{};
inline constexpr __ycxx::__detail::__view_fn::__drop_fn drop{};
inline constexpr __ycxx::__detail::__view_fn::__drop_while_fn drop_while{};
inline constexpr __ycxx::__detail::__view_fn::__counted_fn counted{};
inline constexpr __ycxx::__detail::__view_fn::__common_fn common{};
inline constexpr __ycxx::__detail::__view_fn::__reverse_fn reverse{};
inline constexpr __ycxx::__detail::__view_fn::__as_const_fn as_const{};
inline constexpr __ycxx::__detail::__view_fn::__cache_latest_fn cache_latest{};
inline constexpr __ycxx::__detail::__view_fn::__as_input_fn as_input{};
}} // namespace std::ranges::views
