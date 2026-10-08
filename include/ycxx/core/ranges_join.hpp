// libycxx core: the range adaptors that join or split sequences: join, join_with, lazy_split,
// split and concat.
#pragma once

#include <ycxx/core/ranges_zip.hpp>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/variant.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Calls f(integral_constant<size_t, I>{}) for the I in [0, N) equal to i.
template <std::size_t _Np, class _Fp>
constexpr void __with_index(std::size_t i, _Fp&& __f) {
  [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    (void)((i == _Ip ? (__f(std::integral_constant<std::size_t, _Ip>{}), true) : false) || ...);
  }(std::make_index_sequence<_Np>{});
}

template <class _Rp>
concept __bidirectional_common = std::ranges::bidirectional_range<_Rp> && std::ranges::common_range<_Rp>;

// ---- [range.concat.view] ------------------------------------------------------------------------
template <class... _Rs>
using __concat_reference_t = std::common_reference_t<std::ranges::range_reference_t<_Rs>...>;
template <class... _Rs>
using __concat_value_t = std::common_type_t<std::ranges::range_value_t<_Rs>...>;
template <class... _Rs>
using __concat_rvalue_reference_t = std::common_reference_t<std::ranges::range_rvalue_reference_t<_Rs>...>;

template <class _Ref, class _RRef, class _It>
concept __concat_indirectly_readable_impl = requires(const _It __it) {
  { *__it } -> std::convertible_to<_Ref>;
  { std::ranges::iter_move(__it) } -> std::convertible_to<_RRef>;
};
template <class... _Rs>
concept __concat_indirectly_readable =
    std::common_reference_with<__concat_reference_t<_Rs...>&&, __concat_value_t<_Rs...>&> &&
    std::common_reference_with<__concat_reference_t<_Rs...>&&, __concat_rvalue_reference_t<_Rs...>&&> &&
    std::common_reference_with<__concat_rvalue_reference_t<_Rs...>&&, const __concat_value_t<_Rs...>&> &&
    (__concat_indirectly_readable_impl<__concat_reference_t<_Rs...>, __concat_rvalue_reference_t<_Rs...>,
                                     std::ranges::iterator_t<_Rs>> &&
     ...);
template <class... _Rs>
concept __concatable = requires {
  typename __concat_reference_t<_Rs...>;
  typename __concat_value_t<_Rs...>;
  typename __concat_rvalue_reference_t<_Rs...>;
} && __concat_indirectly_readable<_Rs...>;

// The pack without its last element, as a check over each of them.
template <bool _Const, class... _Rs>
consteval bool __all_but_last_common() {
  return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    return (std::ranges::common_range<__maybe_const<_Const, _Rs...[_Ip]>> && ...);
  }(std::make_index_sequence<sizeof...(_Rs) - 1>{});
}
template <bool _Const, class... _Rs>
concept __concat_is_random_access = __all_random_access<_Const, _Rs...> && __all_but_last_common<_Const, _Rs...>();
template <bool _Const, class... _Rs>
concept __concat_is_bidirectional = __all_bidirectional<_Const, _Rs...> && __all_but_last_common<_Const, _Rs...>();

template <bool _Const, class... _Rs>
consteval bool __all_but_first_sized() {
  return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    return (std::ranges::sized_range<__maybe_const<_Const, _Rs...[_Ip + 1]>> && ...);
  }(std::make_index_sequence<sizeof...(_Rs) - 1>{});
}
template <bool _Const, class... _Rs>
concept __concat_sized_sentinel = (std::sized_sentinel_for<std::ranges::sentinel_t<__maybe_const<_Const, _Rs>>,
                                                         std::ranges::iterator_t<__maybe_const<_Const, _Rs>>> &&
                                 ...) &&
                                __all_but_first_sized<_Const, _Rs...>();

// ---- [range.lazy.split.view] ----------------------------------------------------------------------
template <auto>
struct __require_constant;
template <class _Rp>
concept __tiny_range = std::ranges::sized_range<_Rp> &&
                     requires { typename __require_constant<std::remove_reference_t<_Rp>::size()>; } &&
                     (std::remove_reference_t<_Rp>::size() <= 1);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {

// =============================================================================================
// [range.join]
// =============================================================================================
template <input_range _Vp>
  requires view<_Vp> && input_range<range_reference_t<_Vp>>
class join_view : public view_interface<join_view<_Vp>> {
  using _InnerRng = range_reference_t<_Vp>;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!(is_reference_v<range_reference_t<_Base>> && forward_range<_Base> &&
                    forward_range<range_reference_t<_Base>>)) {
      return type_identity<void>{};
    } else {
      using _OUTERC = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      using _INNERC = __ycxx::__detail::__iter_category_t<iterator_t<range_reference_t<_Base>>>;
      if constexpr (derived_from<_OUTERC, bidirectional_iterator_tag> && derived_from<_INNERC, bidirectional_iterator_tag> &&
                    common_range<range_reference_t<_Base>>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<_OUTERC, forward_iterator_tag> && derived_from<_INNERC, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend join_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, join_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    using _OuterIter = iterator_t<_Base>;
    using _InnerIter = iterator_t<range_reference_t<_Base>>;
    static constexpr bool __ref_is_glvalue = is_reference_v<range_reference_t<_Base>>;

    [[no_unique_address]] conditional_t<forward_range<_Base>, _OuterIter, __ycxx::__detail::__empty_cache> __current_ =
        conditional_t<forward_range<_Base>, _OuterIter, __ycxx::__detail::__empty_cache>();
    optional<_InnerIter> __inner_;
    _Parent* __parent_ = nullptr;

    constexpr _OuterIter& outer() {
      if constexpr (forward_range<_Base>)
        return __current_;
      else
        return *__parent_->__outer_;
    }
    constexpr const _OuterIter& outer() const {
      if constexpr (forward_range<_Base>)
        return __current_;
      else
        return *__parent_->__outer_;
    }
    constexpr void satisfy() {
      auto __update_inner = [this](const iterator_t<_Base>& __x) -> auto&& {
        if constexpr (__ref_is_glvalue)
          return *__x;
        else
          return __parent_->__inner_.__emplace_deref(__x);
      };
      for (; outer() != ranges::end(__parent_->__base_); ++outer()) {
        auto&& __inner = __update_inner(outer());
        __inner_ = ranges::begin(__inner);
        if (*__inner_ != ranges::end(__inner))
          return;
      }
      if constexpr (__ref_is_glvalue)
        __inner_.reset();
    }
    constexpr iterator(_Parent& __parent, _OuterIter outer)
      requires forward_range<_Base>
        : __current_(std::move(outer)), __parent_(__builtin_addressof(__parent)) {
      satisfy();
    }
    constexpr explicit iterator(_Parent& __parent)
      requires(!forward_range<_Base>)
        : __parent_(__builtin_addressof(__parent)) {
      satisfy();
    }

  public:
    using iterator_concept = conditional_t<
        __ref_is_glvalue && bidirectional_range<_Base> && __ycxx::__detail::__bidirectional_common<range_reference_t<_Base>>,
        bidirectional_iterator_tag,
        conditional_t<__ref_is_glvalue && forward_range<_Base> && forward_range<range_reference_t<_Base>>,
                      forward_iterator_tag, input_iterator_tag>>;
    using value_type = range_value_t<range_reference_t<_Base>>;
    using difference_type = common_type_t<range_difference_t<_Base>, range_difference_t<range_reference_t<_Base>>>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, _OuterIter> && convertible_to<iterator_t<_InnerRng>, _InnerIter>
        : __current_(std::move(i.__current_)), __inner_(std::move(i.__inner_)), __parent_(i.__parent_) {}

    constexpr decltype(auto) operator*() const { return **__inner_; }
    constexpr _InnerIter operator->() const
      requires __ycxx::__detail::__has_arrow<_InnerIter> && copyable<_InnerIter>
    {
      return *__inner_;
    }

    constexpr iterator& operator++() {
      auto&& __inner_range = [&]() -> auto&& {
        if constexpr (__ref_is_glvalue)
          return *outer();
        else
          return *__parent_->__inner_;
      }();
      if (++*__inner_ == ranges::end(__ycxx::__detail::__as_lvalue(__inner_range))) {
        ++outer();
        satisfy();
      }
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires __ref_is_glvalue && forward_range<_Base> && forward_range<range_reference_t<_Base>>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires __ref_is_glvalue && bidirectional_range<_Base> && bidirectional_range<range_reference_t<_Base>> &&
               common_range<range_reference_t<_Base>>
    {
      if (__current_ == ranges::end(__parent_->__base_))
        __inner_ = ranges::end(__ycxx::__detail::__as_lvalue(*--__current_));
      while (*__inner_ == ranges::begin(__ycxx::__detail::__as_lvalue(*__current_)))
        *__inner_ = ranges::end(__ycxx::__detail::__as_lvalue(*--__current_));
      --*__inner_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ref_is_glvalue && bidirectional_range<_Base> && bidirectional_range<range_reference_t<_Base>> &&
               common_range<range_reference_t<_Base>>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires __ref_is_glvalue && forward_range<_Base> && equality_comparable<iterator_t<range_reference_t<_Base>>>
    {
      return __x.__current_ == y.__current_ && __x.__inner_ == y.__inner_;
    }
    friend constexpr decltype(auto) iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(*i.__inner_))) {
      return ranges::iter_move(*i.__inner_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(*__x.__inner_, *y.__inner_)))
      requires indirectly_swappable<_InnerIter>
    {
      ranges::iter_swap(*__x.__inner_, *y.__inner_);
    }
  };

  template <bool _Const>
  class sentinel {
    friend join_view;
    friend sentinel<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, join_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(_Parent& __parent) : __end_(ranges::end(__parent.__base_)) {}

    template <bool _OtherConst>
    static constexpr decltype(auto) __outer_of(const iterator<_OtherConst>& __x) {
      if constexpr (forward_range<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>)
        return (__ycxx::__detail::__view_access::current(__x));
      else
        return *__ycxx::__detail::__view_access::__parent(__x)->__outer_;
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> s)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(s.__end_)) {}

    template <bool _OtherConst>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __outer_of(__x) == y.__end_;
    }
  };

  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__cache_if<!forward_range<_Vp>, iterator_t<_Vp>> __outer_;
  [[no_unique_address]] __ycxx::__detail::__cache_if<!is_reference_v<_InnerRng>, remove_cv_t<_InnerRng>> __inner_;

public:
  join_view()
    requires default_initializable<_Vp>
  = default;
  constexpr explicit join_view(_Vp base) : __base_(std::move(base)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin() {
    if constexpr (forward_range<_Vp>) {
      constexpr bool __use_const = __ycxx::__detail::__simple_view<_Vp> && is_reference_v<_InnerRng>;
      return iterator<__use_const>{*this, ranges::begin(__base_)};
    } else {
      __outer_.emplace(ranges::begin(__base_));
      return iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<const _Vp> && is_reference_v<range_reference_t<const _Vp>> &&
             input_range<range_reference_t<const _Vp>>
  {
    return iterator<true>{*this, ranges::begin(__base_)};
  }
  constexpr auto end() {
    if constexpr (forward_range<_Vp> && is_reference_v<_InnerRng> && forward_range<_InnerRng> && common_range<_Vp> &&
                  common_range<_InnerRng>)
      return iterator<__ycxx::__detail::__simple_view<_Vp>>{*this, ranges::end(__base_)};
    else
      return sentinel<__ycxx::__detail::__simple_view<_Vp>>{*this};
  }
  constexpr auto end() const
    requires forward_range<const _Vp> && is_reference_v<range_reference_t<const _Vp>> &&
             input_range<range_reference_t<const _Vp>>
  {
    if constexpr (forward_range<range_reference_t<const _Vp>> && common_range<const _Vp> &&
                  common_range<range_reference_t<const _Vp>>)
      return iterator<true>{*this, ranges::end(__base_)};
    else
      return sentinel<true>{*this};
  }
};
template <class _Rp>
explicit join_view(_Rp&&) -> join_view<views::all_t<_Rp>>;

// =============================================================================================
// [range.join.with]
// =============================================================================================
template <input_range _Vp, forward_range _Pattern>
  requires view<_Vp> && input_range<range_reference_t<_Vp>> && view<_Pattern> &&
           __ycxx::__detail::__concatable<range_reference_t<_Vp>, _Pattern>
class join_with_view : public view_interface<join_with_view<_Vp, _Pattern>> {
  using _InnerRng = range_reference_t<_Vp>;

  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    using _InnerBase = range_reference_t<_Base>;
    using _PatternBase = __ycxx::__detail::__maybe_const<_Const, _Pattern>;
    if constexpr (!(is_reference_v<_InnerBase> && forward_range<_Base> && forward_range<_InnerBase>)) {
      return type_identity<void>{};
    } else {
      using _OUTERC = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      using _INNERC = __ycxx::__detail::__iter_category_t<iterator_t<_InnerBase>>;
      using _PATTERNC = __ycxx::__detail::__iter_category_t<iterator_t<_PatternBase>>;
      if constexpr (!is_reference_v<common_reference_t<iter_reference_t<iterator_t<_InnerBase>>,
                                                       iter_reference_t<iterator_t<_PatternBase>>>>)
        return type_identity<input_iterator_tag>{};
      else if constexpr (derived_from<_OUTERC, bidirectional_iterator_tag> &&
                         derived_from<_INNERC, bidirectional_iterator_tag> &&
                         derived_from<_PATTERNC, bidirectional_iterator_tag> && common_range<_InnerBase> &&
                         common_range<_PatternBase>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<_OUTERC, forward_iterator_tag> && derived_from<_INNERC, forward_iterator_tag> &&
                         derived_from<_PATTERNC, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend join_with_view;
    friend iterator<!_Const>;
    friend __ycxx::__detail::__view_access;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, join_with_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    using _InnerBase = range_reference_t<_Base>;
    using _PatternBase = __ycxx::__detail::__maybe_const<_Const, _Pattern>;
    using _OuterIter = iterator_t<_Base>;
    using _InnerIter = iterator_t<_InnerBase>;
    using _PatternIter = iterator_t<_PatternBase>;
    static constexpr bool __ref_is_glvalue = is_reference_v<_InnerBase>;

    _Parent* __parent_ = nullptr;
    [[no_unique_address]] conditional_t<forward_range<_Base>, _OuterIter, __ycxx::__detail::__empty_cache> __current_ =
        conditional_t<forward_range<_Base>, _OuterIter, __ycxx::__detail::__empty_cache>();
    variant<_PatternIter, _InnerIter> __inner_it_;

    constexpr _OuterIter& outer() {
      if constexpr (forward_range<_Base>)
        return __current_;
      else
        return *__parent_->__outer_it_;
    }
    constexpr const _OuterIter& outer() const {
      if constexpr (forward_range<_Base>)
        return __current_;
      else
        return *__parent_->__outer_it_;
    }
    constexpr auto& __update_inner() {
      if constexpr (__ref_is_glvalue)
        return __ycxx::__detail::__as_lvalue(*outer());
      else
        return __parent_->__inner_.__emplace_deref(outer());
    }
    constexpr auto& __get_inner() {
      if constexpr (__ref_is_glvalue)
        return __ycxx::__detail::__as_lvalue(*outer());
      else
        return *__parent_->__inner_;
    }
    constexpr void satisfy() {
      while (true) {
        if (__inner_it_.index() == 0) {
          if (std::get<0>(__inner_it_) != ranges::end(__parent_->__pattern_))
            break;
          __inner_it_.template emplace<1>(ranges::begin(__update_inner()));
        } else {
          if (std::get<1>(__inner_it_) != ranges::end(__get_inner()))
            break;
          if (++outer() == ranges::end(__parent_->__base_)) {
            if constexpr (__ref_is_glvalue)
              __inner_it_.template emplace<0>();
            break;
          }
          __inner_it_.template emplace<0>(ranges::begin(__parent_->__pattern_));
        }
      }
    }
    constexpr void start() {
      if (outer() != ranges::end(__parent_->__base_)) {
        __inner_it_.template emplace<1>(ranges::begin(__update_inner()));
        satisfy();
      }
    }
    constexpr iterator(_Parent& __parent, _OuterIter outer)
      requires forward_range<_Base>
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(outer)) {
      start();
    }
    constexpr explicit iterator(_Parent& __parent)
      requires(!forward_range<_Base>)
        : __parent_(__builtin_addressof(__parent)) {
      start();
    }

  public:
    using iterator_concept = conditional_t<
        __ref_is_glvalue && bidirectional_range<_Base> && __ycxx::__detail::__bidirectional_common<_InnerBase> &&
            __ycxx::__detail::__bidirectional_common<_PatternBase>,
        bidirectional_iterator_tag,
        conditional_t<__ref_is_glvalue && forward_range<_Base> && forward_range<_InnerBase>, forward_iterator_tag,
                      input_iterator_tag>>;
    using value_type = common_type_t<iter_value_t<_InnerIter>, iter_value_t<_PatternIter>>;
    using difference_type =
        common_type_t<iter_difference_t<_OuterIter>, iter_difference_t<_InnerIter>, iter_difference_t<_PatternIter>>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, _OuterIter> && convertible_to<iterator_t<_InnerRng>, _InnerIter> &&
               convertible_to<iterator_t<_Pattern>, _PatternIter>
        : __parent_(i.__parent_), __current_(std::move(i.__current_)) {
      if (i.__inner_it_.index() == 0)
        __inner_it_.template emplace<0>(std::get<0>(std::move(i.__inner_it_)));
      else
        __inner_it_.template emplace<1>(std::get<1>(std::move(i.__inner_it_)));
    }

    constexpr decltype(auto) operator*() const {
      using reference = common_reference_t<iter_reference_t<_InnerIter>, iter_reference_t<_PatternIter>>;
      return std::visit([](auto& __it) -> reference { return *__it; }, __inner_it_);
    }
    constexpr iterator& operator++() {
      std::visit([](auto& __it) { ++__it; }, __inner_it_);
      satisfy();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires __ref_is_glvalue && forward_iterator<_OuterIter> && forward_iterator<_InnerIter>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires __ref_is_glvalue && bidirectional_range<_Base> && __ycxx::__detail::__bidirectional_common<_InnerBase> &&
               __ycxx::__detail::__bidirectional_common<_PatternBase>
    {
      if (__current_ == ranges::end(__parent_->__base_)) {
        auto&& __inner = *--__current_;
        __inner_it_.template emplace<1>(ranges::end(__inner));
      }
      while (true) {
        if (__inner_it_.index() == 0) {
          auto& __it = std::get<0>(__inner_it_);
          if (__it == ranges::begin(__parent_->__pattern_)) {
            auto&& __inner = *--__current_;
            __inner_it_.template emplace<1>(ranges::end(__inner));
          } else {
            break;
          }
        } else {
          auto& __it = std::get<1>(__inner_it_);
          auto&& __inner = *__current_;
          if (__it == ranges::begin(__inner))
            __inner_it_.template emplace<0>(ranges::end(__parent_->__pattern_));
          else
            break;
        }
      }
      std::visit([](auto& __it) { --__it; }, __inner_it_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ref_is_glvalue && bidirectional_range<_Base> && __ycxx::__detail::__bidirectional_common<_InnerBase> &&
               __ycxx::__detail::__bidirectional_common<_PatternBase>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires __ref_is_glvalue && forward_range<_Base> && equality_comparable<_InnerIter>
    {
      return __x.__current_ == y.__current_ && __x.__inner_it_ == y.__inner_it_;
    }
    friend constexpr decltype(auto) iter_move(const iterator& __x) {
      using __rvalue_reference = common_reference_t<iter_rvalue_reference_t<_InnerIter>, iter_rvalue_reference_t<_PatternIter>>;
      return std::visit<__rvalue_reference>(ranges::iter_move, __x.__inner_it_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y)
      requires indirectly_swappable<_InnerIter, _PatternIter>
    {
      std::visit(ranges::iter_swap, __x.__inner_it_, y.__inner_it_);
    }
  };

  template <bool _Const>
  class sentinel {
    friend join_with_view;
    friend sentinel<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, join_with_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    constexpr explicit sentinel(_Parent& __parent) : __end_(ranges::end(__parent.__base_)) {}

    template <bool _OtherConst>
    static constexpr decltype(auto) __outer_of(const iterator<_OtherConst>& __x) {
      if constexpr (forward_range<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>)
        return (__ycxx::__detail::__view_access::current(__x));
      else
        return *__ycxx::__detail::__view_access::__parent(__x)->__outer_it_;
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!_Const> s)
      requires _Const && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __end_(std::move(s.__end_)) {}

    template <bool _OtherConst>
      requires sentinel_for<sentinel_t<_Base>, iterator_t<__ycxx::__detail::__maybe_const<_OtherConst, _Vp>>>
    friend constexpr bool operator==(const iterator<_OtherConst>& __x, const sentinel& y) {
      return __outer_of(__x) == y.__end_;
    }
  };

  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__cache_if<!forward_range<_Vp>, iterator_t<_Vp>> __outer_it_;
  [[no_unique_address]] __ycxx::__detail::__cache_if<!is_reference_v<_InnerRng>, remove_cv_t<_InnerRng>> __inner_;
  _Pattern __pattern_ = _Pattern();

public:
  join_with_view()
    requires default_initializable<_Vp> && default_initializable<_Pattern>
  = default;
  constexpr explicit join_with_view(_Vp base, _Pattern pattern) : __base_(std::move(base)), __pattern_(std::move(pattern)) {}
  template <input_range _Rp>
    requires constructible_from<_Vp, views::all_t<_Rp>> && constructible_from<_Pattern, single_view<range_value_t<_InnerRng>>>
  constexpr explicit join_with_view(_Rp&& r, range_value_t<_InnerRng> e)
      : __base_(views::all(static_cast<_Rp&&>(r))), __pattern_(views::single(std::move(e))) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin() {
    if constexpr (forward_range<_Vp>) {
      constexpr bool __use_const =
          __ycxx::__detail::__simple_view<_Vp> && is_reference_v<_InnerRng> && __ycxx::__detail::__simple_view<_Pattern>;
      return iterator<__use_const>{*this, ranges::begin(__base_)};
    } else {
      __outer_it_.emplace(ranges::begin(__base_));
      return iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<const _Vp> && forward_range<const _Pattern> && is_reference_v<range_reference_t<const _Vp>> &&
             input_range<range_reference_t<const _Vp>> &&
             __ycxx::__detail::__concatable<range_reference_t<const _Vp>, const _Pattern>
  {
    return iterator<true>{*this, ranges::begin(__base_)};
  }
  constexpr auto end() {
    constexpr bool c = __ycxx::__detail::__simple_view<_Vp> && __ycxx::__detail::__simple_view<_Pattern>;
    if constexpr (forward_range<_Vp> && is_reference_v<_InnerRng> && forward_range<_InnerRng> && common_range<_Vp> &&
                  common_range<_InnerRng>)
      return iterator<c>{*this, ranges::end(__base_)};
    else
      return sentinel<c>{*this};
  }
  constexpr auto end() const
    requires forward_range<const _Vp> && forward_range<const _Pattern> && is_reference_v<range_reference_t<const _Vp>> &&
             input_range<range_reference_t<const _Vp>> &&
             __ycxx::__detail::__concatable<range_reference_t<const _Vp>, const _Pattern>
  {
    using _InnerConstRng = range_reference_t<const _Vp>;
    if constexpr (forward_range<_InnerConstRng> && common_range<const _Vp> && common_range<_InnerConstRng>)
      return iterator<true>{*this, ranges::end(__base_)};
    else
      return sentinel<true>{*this};
  }
};
template <class _Rp, class _Pp>
join_with_view(_Rp&&, _Pp&&) -> join_with_view<views::all_t<_Rp>, views::all_t<_Pp>>;
template <input_range _Rp>
join_with_view(_Rp&&, range_value_t<range_reference_t<_Rp>>)
    -> join_with_view<views::all_t<_Rp>, single_view<range_value_t<range_reference_t<_Rp>>>>;

// =============================================================================================
// [range.lazy.split]
// =============================================================================================
template <input_range _Vp, forward_range _Pattern>
  requires view<_Vp> && view<_Pattern> && indirectly_comparable<iterator_t<_Vp>, iterator_t<_Pattern>, ranges::equal_to> &&
           (forward_range<_Vp> || __ycxx::__detail::__tiny_range<_Pattern>)
class lazy_split_view : public view_interface<lazy_split_view<_Vp, _Pattern>> {
  template <bool _Const>
  class __inner_iterator;

  template <bool _Const>
  class __outer_iterator
      : public __ycxx::__detail::__category_base<conditional_t<forward_range<__ycxx::__detail::__maybe_const<_Const, _Vp>>,
                                                         input_iterator_tag, void>> {
    friend lazy_split_view;
    friend __outer_iterator<!_Const>;
    friend __inner_iterator<_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, lazy_split_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    _Parent* __parent_ = nullptr;
    [[no_unique_address]] conditional_t<forward_range<_Vp>, iterator_t<_Base>, __ycxx::__detail::__empty_cache> __current_ =
        conditional_t<forward_range<_Vp>, iterator_t<_Base>, __ycxx::__detail::__empty_cache>();
    bool __trailing_empty_ = false;

    // The notional member "current" ([range.lazy.split.outer]/1).
    constexpr iterator_t<_Base>& current() const noexcept {
      if constexpr (forward_range<_Vp>)
        return const_cast<iterator_t<_Base>&>(__current_);
      else
        return *__parent_->__current_;
    }

    constexpr explicit __outer_iterator(_Parent& __parent)
      requires(!forward_range<_Base>)
        : __parent_(__builtin_addressof(__parent)) {}
    constexpr __outer_iterator(_Parent& __parent, iterator_t<_Base> current)
      requires forward_range<_Base>
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(current)) {}

  public:
    using iterator_concept = conditional_t<forward_range<_Base>, forward_iterator_tag, input_iterator_tag>;
    struct value_type : view_interface<value_type> {
    private:
      friend __outer_iterator;
      __outer_iterator __i_ = __outer_iterator();
      constexpr explicit value_type(__outer_iterator i) : __i_(std::move(i)) {}

    public:
      constexpr __inner_iterator<_Const> begin() const { return __inner_iterator<_Const>{__i_}; }
      constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
    };
    using difference_type = range_difference_t<_Base>;

    __outer_iterator() = default;
    constexpr __outer_iterator(__outer_iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __parent_(i.__parent_), __current_(std::move(i.__current_)), __trailing_empty_(i.__trailing_empty_) {}

    constexpr value_type operator*() const { return value_type{*this}; }
    constexpr __outer_iterator& operator++() {
      const auto end = ranges::end(__parent_->__base_);
      auto& cur = current();
      if (cur == end) {
        __trailing_empty_ = false;
        return *this;
      }
      const auto [__pbegin, __pend] = subrange{__parent_->__pattern_};
      if (__pbegin == __pend) {
        ++cur;
      } else if constexpr (__ycxx::__detail::__tiny_range<_Pattern>) {
        cur = ranges::find(std::move(cur), end, *__pbegin);
        if (cur != end) {
          ++cur;
          if (cur == end)
            __trailing_empty_ = true;
          else if constexpr (!forward_range<_Vp>)
            __trailing_empty_ = true;
        }
      } else {
        do {
          auto [b, p] = ranges::mismatch(cur, end, __pbegin, __pend);
          if (p == __pend) {
            cur = b;
            if (cur == end)
              __trailing_empty_ = true;
            break;
          }
        } while (++cur != end);
      }
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr __outer_iterator operator++(int)
      requires forward_range<_Base>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }

    friend constexpr bool operator==(const __outer_iterator& __x, const __outer_iterator& y)
      requires forward_range<_Base>
    {
      return __x.__current_ == y.__current_ && __x.__trailing_empty_ == y.__trailing_empty_;
    }
    friend constexpr bool operator==(const __outer_iterator& __x, default_sentinel_t) {
      return __x.current() == ranges::end(__x.__parent_->__base_) && !__x.__trailing_empty_;
    }
  };

  template <bool _Const>
  static consteval auto __inner_category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (derived_from<_Cp, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<_Cp>{};
    }
  }

  template <bool _Const>
  class __inner_iterator : public __ycxx::__detail::__category_base<typename decltype(__inner_category<_Const>())::type> {
    friend __outer_iterator<_Const>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    __outer_iterator<_Const> __i_ = __outer_iterator<_Const>();
    bool __incremented_ = false;
    constexpr explicit __inner_iterator(__outer_iterator<_Const> i) : __i_(std::move(i)) {}

  public:
    using iterator_concept = typename __outer_iterator<_Const>::iterator_concept;
    using value_type = range_value_t<_Base>;
    using difference_type = range_difference_t<_Base>;

    __inner_iterator() = default;
    constexpr const iterator_t<_Base>& base() const& noexcept { return __i_.current(); }
    constexpr iterator_t<_Base> base() &&
      requires forward_range<_Vp>
    {
      return std::move(__i_.current());
    }
    constexpr decltype(auto) operator*() const { return *__i_.current(); }
    constexpr __inner_iterator& operator++() {
      __incremented_ = true;
      if constexpr (!forward_range<_Base>) {
        if constexpr (_Pattern::size() == 0)
          return *this;
      }
      ++__i_.current();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr __inner_iterator operator++(int)
      requires forward_range<_Base>
    {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }

    friend constexpr bool operator==(const __inner_iterator& __x, const __inner_iterator& y)
      requires forward_range<_Base>
    {
      return __x.__i_.current() == y.__i_.current();
    }
    friend constexpr bool operator==(const __inner_iterator& __x, default_sentinel_t) {
      auto [__pcur, __pend] = subrange{__x.__i_.__parent_->__pattern_};
      auto end = ranges::end(__x.__i_.__parent_->__base_);
      if constexpr (__ycxx::__detail::__tiny_range<_Pattern>) {
        const auto& cur = __x.__i_.current();
        if (cur == end)
          return true;
        if (__pcur == __pend)
          return __x.__incremented_;
        return bool(*cur == *__pcur);
      } else {
        auto cur = __x.__i_.current();
        if (cur == end)
          return true;
        if (__pcur == __pend)
          return __x.__incremented_;
        do {
          if (!bool(*cur == *__pcur))
            return false;
          if (++__pcur == __pend)
            return true;
        } while (++cur != end);
        return false;
      }
    }
    friend constexpr decltype(auto) iter_move(const __inner_iterator& i) noexcept(
        noexcept(ranges::iter_move(i.__i_.current()))) {
      return ranges::iter_move(i.__i_.current());
    }
    friend constexpr void iter_swap(const __inner_iterator& __x, const __inner_iterator& y) noexcept(
        noexcept(ranges::iter_swap(__x.__i_.current(), y.__i_.current())))
      requires indirectly_swappable<iterator_t<_Base>>
    {
      ranges::iter_swap(__x.__i_.current(), y.__i_.current());
    }
  };

  _Vp __base_ = _Vp();
  _Pattern __pattern_ = _Pattern();
  [[no_unique_address]] __ycxx::__detail::__cache_if<!forward_range<_Vp>, iterator_t<_Vp>> __current_;

public:
  lazy_split_view()
    requires default_initializable<_Vp> && default_initializable<_Pattern>
  = default;
  constexpr explicit lazy_split_view(_Vp base, _Pattern pattern) : __base_(std::move(base)), __pattern_(std::move(pattern)) {}
  template <input_range _Rp>
    requires constructible_from<_Vp, views::all_t<_Rp>> && constructible_from<_Pattern, single_view<range_value_t<_Rp>>>
  constexpr explicit lazy_split_view(_Rp&& r, range_value_t<_Rp> e)
      : __base_(views::all(static_cast<_Rp&&>(r))), __pattern_(views::single(std::move(e))) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin() {
    if constexpr (forward_range<_Vp>) {
      return __outer_iterator<__ycxx::__detail::__simple_view<_Vp> && __ycxx::__detail::__simple_view<_Pattern>>{*this,
                                                                                                ranges::begin(__base_)};
    } else {
      __current_.emplace(ranges::begin(__base_));
      return __outer_iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<_Vp> && forward_range<const _Vp> && forward_range<const _Pattern>
  {
    return __outer_iterator<true>{*this, ranges::begin(__base_)};
  }
  constexpr auto end()
    requires forward_range<_Vp> && common_range<_Vp>
  {
    return __outer_iterator<__ycxx::__detail::__simple_view<_Vp> && __ycxx::__detail::__simple_view<_Pattern>>{*this, ranges::end(__base_)};
  }
  constexpr auto end() const {
    if constexpr (forward_range<_Vp> && forward_range<const _Vp> && common_range<const _Vp> && forward_range<const _Pattern>)
      return __outer_iterator<true>{*this, ranges::end(__base_)};
    else
      return default_sentinel;
  }
};
template <class _Rp, class _Pp>
lazy_split_view(_Rp&&, _Pp&&) -> lazy_split_view<views::all_t<_Rp>, views::all_t<_Pp>>;
template <input_range _Rp>
lazy_split_view(_Rp&&, range_value_t<_Rp>) -> lazy_split_view<views::all_t<_Rp>, single_view<range_value_t<_Rp>>>;

// =============================================================================================
// [range.split]
// =============================================================================================
template <forward_range _Vp, forward_range _Pattern>
  requires view<_Vp> && view<_Pattern> && indirectly_comparable<iterator_t<_Vp>, iterator_t<_Pattern>, ranges::equal_to>
class split_view : public view_interface<split_view<_Vp, _Pattern>> {
  class sentinel;
  class iterator {
    friend split_view;
    friend sentinel;
    friend __ycxx::__detail::__view_access;
    split_view* __parent_ = nullptr;
    iterator_t<_Vp> __current_ = iterator_t<_Vp>();
    subrange<iterator_t<_Vp>> __next_ = subrange<iterator_t<_Vp>>();
    bool __trailing_empty_ = false;

    constexpr iterator(split_view& __parent, iterator_t<_Vp> current, subrange<iterator_t<_Vp>> next)
        : __parent_(__builtin_addressof(__parent)), __current_(std::move(current)), __next_(std::move(next)) {}

  public:
    using iterator_concept = forward_iterator_tag;
    using iterator_category = input_iterator_tag;
    using value_type = subrange<iterator_t<_Vp>>;
    using difference_type = range_difference_t<_Vp>;

    iterator() = default;
    constexpr iterator_t<_Vp> base() const { return __current_; }
    constexpr value_type operator*() const { return {__current_, __next_.begin()}; }
    constexpr iterator& operator++() {
      __current_ = __next_.begin();
      if (__current_ != ranges::end(__parent_->__base_)) {
        __current_ = __next_.end();
        if (__current_ == ranges::end(__parent_->__base_)) {
          __trailing_empty_ = true;
          __next_ = {__current_, __current_};
        } else {
          __next_ = __parent_->__find_next(__current_);
        }
      } else {
        __trailing_empty_ = false;
      }
      return *this;
    }
    constexpr iterator operator++(int) {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    friend constexpr bool operator==(const iterator& __x, const iterator& y) {
      return __x.__current_ == y.__current_ && __x.__trailing_empty_ == y.__trailing_empty_;
    }
  };

  class sentinel {
    friend split_view;
    sentinel_t<_Vp> __end_ = sentinel_t<_Vp>();
    constexpr explicit sentinel(split_view& __parent) : __end_(ranges::end(__parent.__base_)) {}

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator& __x, const sentinel& y) {
      return __ycxx::__detail::__view_access::current(__x) == y.__end_ && !__x.__trailing_empty_;
    }
  };

  _Vp __base_ = _Vp();
  _Pattern __pattern_ = _Pattern();
  __ycxx::__detail::__non_propagating_cache<subrange<iterator_t<_Vp>>> __next_;

  constexpr subrange<iterator_t<_Vp>> __find_next(iterator_t<_Vp> __it) {
    auto [b, e] = ranges::search(subrange(__it, ranges::end(__base_)), __pattern_);
    if (b != ranges::end(__base_) && ranges::empty(__pattern_)) {
      ++b;
      ++e;
    }
    return {b, e};
  }

public:
  split_view()
    requires default_initializable<_Vp> && default_initializable<_Pattern>
  = default;
  constexpr explicit split_view(_Vp base, _Pattern pattern) : __base_(std::move(base)), __pattern_(std::move(pattern)) {}
  template <forward_range _Rp>
    requires constructible_from<_Vp, views::all_t<_Rp>> && constructible_from<_Pattern, single_view<range_value_t<_Rp>>>
  constexpr explicit split_view(_Rp&& r, range_value_t<_Rp> e)
      : __base_(views::all(static_cast<_Rp&&>(r))), __pattern_(views::single(std::move(e))) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr iterator begin() {
    if (!__next_.has_value())
      __next_.emplace(__find_next(ranges::begin(__base_)));
    return {*this, ranges::begin(__base_), *__next_};
  }
  constexpr auto end() {
    if constexpr (common_range<_Vp>)
      return iterator{*this, ranges::end(__base_), {}};
    else
      return sentinel{*this};
  }
};
template <class _Rp, class _Pp>
split_view(_Rp&&, _Pp&&) -> split_view<views::all_t<_Rp>, views::all_t<_Pp>>;
template <forward_range _Rp>
split_view(_Rp&&, range_value_t<_Rp>) -> split_view<views::all_t<_Rp>, single_view<range_value_t<_Rp>>>;

// =============================================================================================
// [range.concat]
// =============================================================================================
template <input_range... _Views>
  requires(view<_Views> && ...) && (sizeof...(_Views) > 0) && __ycxx::__detail::__concatable<_Views...>
class concat_view : public view_interface<concat_view<_Views...>> {
  static constexpr size_t _Np = sizeof...(_Views);

  template <bool _Const>
  static consteval auto category() {
    using namespace __ycxx::__detail;
    if constexpr (!__all_forward<_Const, _Views...>) {
      return type_identity<void>{};
    } else if constexpr (!is_reference_v<__concat_reference_t<__maybe_const<_Const, _Views>...>>) {
      return type_identity<input_iterator_tag>{};
    } else if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>, random_access_iterator_tag> &&
                          ...) &&
                         __concat_is_random_access<_Const, _Views...>) {
      return type_identity<random_access_iterator_tag>{};
    } else if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>,
                                       bidirectional_iterator_tag> &&
                          ...) &&
                         __concat_is_bidirectional<_Const, _Views...>) {
      return type_identity<bidirectional_iterator_tag>{};
    } else if constexpr ((derived_from<__iter_category_t<iterator_t<__maybe_const<_Const, _Views>>>, forward_iterator_tag> &&
                          ...)) {
      return type_identity<forward_iterator_tag>{};
    } else {
      return type_identity<input_iterator_tag>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend concat_view;
    friend iterator<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, concat_view>;

  public:
    using iterator_concept =
        conditional_t<__ycxx::__detail::__concat_is_random_access<_Const, _Views...>, random_access_iterator_tag,
                      conditional_t<__ycxx::__detail::__concat_is_bidirectional<_Const, _Views...>, bidirectional_iterator_tag,
                                    conditional_t<__ycxx::__detail::__all_forward<_Const, _Views...>, forward_iterator_tag,
                                                  input_iterator_tag>>>;
    using value_type = __ycxx::__detail::__concat_value_t<__ycxx::__detail::__maybe_const<_Const, _Views>...>;
    using difference_type = common_type_t<range_difference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>;

  private:
    using __base_iter = variant<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>...>;
    using reference = __ycxx::__detail::__concat_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>...>;
    using __rvalue_reference = __ycxx::__detail::__concat_rvalue_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>...>;

    _Parent* __parent_ = nullptr;
    __base_iter __it_;

    template <size_t _Ip>
    constexpr void satisfy() {
      if constexpr (_Ip < _Np - 1) {
        if (std::get<_Ip>(__it_) == ranges::end(std::get<_Ip>(__parent_->__views_))) {
          __it_.template emplace<_Ip + 1>(ranges::begin(std::get<_Ip + 1>(__parent_->__views_)));
          satisfy<_Ip + 1>();
        }
      }
    }
    template <size_t _Ip>
    constexpr void prev() {
      if constexpr (_Ip == 0) {
        --std::get<0>(__it_);
      } else {
        if (std::get<_Ip>(__it_) == ranges::begin(std::get<_Ip>(__parent_->__views_))) {
          __it_.template emplace<_Ip - 1>(ranges::end(std::get<_Ip - 1>(__parent_->__views_)));
          prev<_Ip - 1>();
        } else {
          --std::get<_Ip>(__it_);
        }
      }
    }
    template <size_t _Ip>
    constexpr void __advance_fwd(difference_type offset, difference_type __steps) {
      using __underlying_diff_type = iter_difference_t<variant_alternative_t<_Ip, __base_iter>>;
      if constexpr (_Ip == _Np - 1) {
        std::get<_Ip>(__it_) += static_cast<__underlying_diff_type>(__steps);
      } else {
        difference_type __n_size = ranges::distance(std::get<_Ip>(__parent_->__views_));
        if (offset + __steps < __n_size) {
          std::get<_Ip>(__it_) += static_cast<__underlying_diff_type>(__steps);
        } else {
          __it_.template emplace<_Ip + 1>(ranges::begin(std::get<_Ip + 1>(__parent_->__views_)));
          __advance_fwd<_Ip + 1>(0, offset + __steps - __n_size);
        }
      }
    }
    template <size_t _Ip>
    constexpr void __advance_bwd(difference_type offset, difference_type __steps) {
      using __underlying_diff_type = iter_difference_t<variant_alternative_t<_Ip, __base_iter>>;
      if constexpr (_Ip == 0) {
        std::get<_Ip>(__it_) -= static_cast<__underlying_diff_type>(__steps);
      } else {
        if (offset >= __steps) {
          std::get<_Ip>(__it_) -= static_cast<__underlying_diff_type>(__steps);
        } else {
          difference_type __prev_size = ranges::distance(std::get<_Ip - 1>(__parent_->__views_));
          __it_.template emplace<_Ip - 1>(ranges::end(std::get<_Ip - 1>(__parent_->__views_)));
          __advance_bwd<_Ip - 1>(__prev_size, __steps - offset);
        }
      }
    }
    // The sum of the sizes of the underlying ranges with indices in [from, to).
    constexpr difference_type __size_between(size_t from, size_t to) const {
      difference_type s = 0;
      [&]<size_t... _Ip>(index_sequence<_Ip...>) {
        ((_Ip >= from && _Ip < to ? (void)(s += static_cast<difference_type>(ranges::distance(std::get<_Ip>(__parent_->__views_))))
                              : (void)0),
         ...);
      }(make_index_sequence<_Np>{});
      return s;
    }

    template <class... _Args>
    constexpr explicit iterator(_Parent* __parent, _Args&&... __args)
      requires constructible_from<__base_iter, _Args&&...>
        : __parent_(__parent), __it_(static_cast<_Args&&>(__args)...) {}

  public:
    iterator() = default;
    constexpr iterator(iterator<!_Const> __it)
      requires _Const && (convertible_to<iterator_t<_Views>, iterator_t<const _Views>> && ...)
        : __parent_(__it.__parent_) {
      __ycxx::__detail::__with_index<_Np>(__it.__it_.index(), [&](auto i) {
        __it_.template emplace<i>(std::get<i>(std::move(__it.__it_)));
      });
    }

    constexpr decltype(auto) operator*() const {
      return std::visit([](auto&& __it) -> reference { return *__it; }, __it_);
    }
    constexpr iterator& operator++() {
      __ycxx::__detail::__with_index<_Np>(__it_.index(), [&](auto i) {
        ++std::get<i>(__it_);
        satisfy<i>();
      });
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
      requires __ycxx::__detail::__concat_is_bidirectional<_Const, _Views...>
    {
      __ycxx::__detail::__with_index<_Np>(__it_.index(), [&](auto i) { prev<i>(); });
      return *this;
    }
    constexpr iterator operator--(int)
      requires __ycxx::__detail::__concat_is_bidirectional<_Const, _Views...>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      __ycxx::__detail::__with_index<_Np>(__it_.index(), [&](auto i) {
        difference_type offset = std::get<i>(__it_) - ranges::begin(std::get<i>(__parent_->__views_));
        if (n > 0)
          __advance_fwd<i>(offset, n);
        else if (n < 0)
          __advance_bwd<i>(offset, -n);
      });
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      *this += -n;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      return *((*this) + n);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y)
      requires(equality_comparable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> && ...)
    {
      return __x.__it_ == y.__it_;
    }
    friend constexpr bool operator==(const iterator& __it, default_sentinel_t) {
      constexpr auto __last_idx = _Np - 1;
      return __it.__it_.index() == __last_idx &&
             std::get<__last_idx>(__it.__it_) == ranges::end(std::get<__last_idx>(__it.__parent_->__views_));
    }
    friend constexpr bool operator<(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __x.__it_ < y.__it_;
    }
    friend constexpr bool operator>(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __x.__it_ > y.__it_;
    }
    friend constexpr bool operator<=(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __x.__it_ <= y.__it_;
    }
    friend constexpr bool operator>=(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__all_random_access<_Const, _Views...>
    {
      return __x.__it_ >= y.__it_;
    }
    friend constexpr auto operator<=>(const iterator& __x, const iterator& y)
      requires(__ycxx::__detail::__all_random_access<_Const, _Views...> &&
               (three_way_comparable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>> && ...))
    {
      return __x.__it_ <=> y.__it_;
    }
    friend constexpr iterator operator+(const iterator& __it, difference_type n)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      auto __temp = __it;
      __temp += n;
      return __temp;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& __it)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      return __it + n;
    }
    friend constexpr iterator operator-(const iterator& __it, difference_type n)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      auto __temp = __it;
      __temp -= n;
      return __temp;
    }
    friend constexpr difference_type operator-(const iterator& __x, const iterator& y)
      requires __ycxx::__detail::__concat_is_random_access<_Const, _Views...>
    {
      size_t __ix = __x.__it_.index(), __iy = y.__it_.index();
      if (__ix < __iy)
        return -(y - __x);
      difference_type result = 0;
      __ycxx::__detail::__with_index<_Np>(__ix, [&](auto i) {
        if (__ix == __iy) {
          result = std::get<i>(__x.__it_) - std::get<i>(y.__it_);
        } else {
          difference_type __dx = ranges::distance(ranges::begin(std::get<i>(__x.__parent_->__views_)), std::get<i>(__x.__it_));
          __ycxx::__detail::__with_index<_Np>(__iy, [&](auto __j) {
            difference_type __dy = ranges::distance(std::get<__j>(y.__it_), ranges::end(std::get<__j>(y.__parent_->__views_)));
            result = __dy + __x.__size_between(__iy + 1, __ix) + __dx;
          });
        }
      });
      return result;
    }
    friend constexpr difference_type operator-(const iterator& __x, default_sentinel_t)
      requires __ycxx::__detail::__concat_sized_sentinel<_Const, _Views...>
    {
      difference_type result = 0;
      __ycxx::__detail::__with_index<_Np>(__x.__it_.index(), [&](auto i) {
        difference_type __dx = ranges::distance(std::get<i>(__x.__it_), ranges::end(std::get<i>(__x.__parent_->__views_)));
        difference_type s = 0;
        [&]<size_t... _Kp>(index_sequence<_Kp...>) {
          ((_Kp > i ? (void)(s += static_cast<difference_type>(ranges::size(std::get<_Kp>(__x.__parent_->__views_)))) : (void)0),
           ...);
        }(make_index_sequence<_Np>{});
        result = -(__dx + s);
      });
      return result;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& __x)
      requires __ycxx::__detail::__concat_sized_sentinel<_Const, _Views...>
    {
      return -(__x - default_sentinel);
    }
    friend constexpr decltype(auto) iter_move(const iterator& __it) noexcept(
        ((is_nothrow_invocable_v<decltype(ranges::iter_move), const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&> &&
          is_nothrow_convertible_v<range_rvalue_reference_t<__ycxx::__detail::__maybe_const<_Const, _Views>>, __rvalue_reference>) &&
         ...)) {
      return std::visit([](const auto& i) -> __rvalue_reference { return ranges::iter_move(i); }, __it.__it_);
    }
    friend constexpr void iter_swap(const iterator& __x, const iterator& y) noexcept(
        noexcept(ranges::swap(*__x, *y)) &&
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>(),
                                    std::declval<const iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>&>())) &&
         ...))
      requires swappable_with<reference, reference> &&
               (... && indirectly_swappable<iterator_t<__ycxx::__detail::__maybe_const<_Const, _Views>>>)
    {
      std::visit(
          [&](const auto& __it1, const auto& __it2) {
            if constexpr (is_same_v<decltype(__it1), decltype(__it2)>)
              ranges::iter_swap(__it1, __it2);
            else
              ranges::swap(*__x, *y);
          },
          __x.__it_, y.__it_);
    }
  };

  tuple<_Views...> __views_;

  template <bool _Const, class _Self>
  static constexpr auto __end_impl(_Self* __self) {
    if constexpr (__ycxx::__detail::__all_forward<_Const, _Views...> &&
                  common_range<__ycxx::__detail::__maybe_const<_Const, _Views...[_Np - 1]>>)
      return iterator<_Const>(__self, in_place_index<_Np - 1>, ranges::end(std::get<_Np - 1>(__self->__views_)));
    else
      return default_sentinel;
  }

public:
  constexpr concat_view() = default;
  constexpr explicit concat_view(_Views... views) : __views_(std::move(views)...) {}

  constexpr iterator<false> begin()
    requires(!(__ycxx::__detail::__simple_view<_Views> && ...))
  {
    iterator<false> __it(this, in_place_index<0>, ranges::begin(std::get<0>(__views_)));
    __it.template satisfy<0>();
    return __it;
  }
  constexpr iterator<true> begin() const
    requires(range<const _Views> && ...) && __ycxx::__detail::__concatable<const _Views...>
  {
    iterator<true> __it(this, in_place_index<0>, ranges::begin(std::get<0>(__views_)));
    __it.template satisfy<0>();
    return __it;
  }
  constexpr auto end()
    requires(!(__ycxx::__detail::__simple_view<_Views> && ...))
  {
    return __end_impl<false>(this);
  }
  constexpr auto end() const
    requires(range<const _Views> && ...) && __ycxx::__detail::__concatable<const _Views...>
  {
    return __end_impl<true>(this);
  }
  constexpr auto size()
    requires(sized_range<_Views> && ...)
  {
    return std::apply(
        [](auto... __sizes) {
          using _CT = make_unsigned_t<common_type_t<decltype(__sizes)...>>;
          return (_CT(__sizes) + ...);
        },
        __ycxx::__detail::__tuple_transform(ranges::size, __views_));
  }
  constexpr auto size() const
    requires(sized_range<const _Views> && ...)
  {
    return std::apply(
        [](auto... __sizes) {
          using _CT = make_unsigned_t<common_type_t<decltype(__sizes)...>>;
          return (_CT(__sizes) + ...);
        },
        __ycxx::__detail::__tuple_transform(ranges::size, __views_));
  }
  constexpr auto reserve_hint()
    requires(approximately_sized_range<_Views> && ...)
  {
    return std::apply(
        [](auto... __sizes) {
          using _CT = make_unsigned_t<common_type_t<decltype(__sizes)...>>;
          return (_CT(__sizes) + ...);
        },
        __ycxx::__detail::__tuple_transform(ranges::reserve_hint, __views_));
  }
  constexpr auto reserve_hint() const
    requires(approximately_sized_range<const _Views> && ...)
  {
    return std::apply(
        [](auto... __sizes) {
          using _CT = make_unsigned_t<common_type_t<decltype(__sizes)...>>;
          return (_CT(__sizes) + ...);
        },
        __ycxx::__detail::__tuple_transform(ranges::reserve_hint, __views_));
  }
};
template <class... _Rp>
concat_view(_Rp&&...) -> concat_view<views::all_t<_Rp>...>;

}}} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__view_fn {

struct __join_fn : std::ranges::range_adaptor_closure<__join_fn> {
  template <class _Ep>
    requires requires { std::ranges::join_view<std::views::all_t<_Ep>>{std::declval<_Ep>()}; }
  [[nodiscard]] constexpr auto operator()(_Ep&& e) const {
    return std::ranges::join_view<std::views::all_t<_Ep>>{static_cast<_Ep&&>(e)};
  }
};

// The views taking a range and a pattern: views::X(E, F) is X_view(E, F).
template <template <class, class> class _View>
struct __pattern_fn {
  template <class _Ep, class _Fp>
    requires requires { _View(std::declval<_Ep>(), std::declval<_Fp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Fp&& __f) const {
    return _View(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  [[nodiscard]] constexpr auto operator()(_Fp&& __f) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Fp&&>(__f));
  }
};

struct __concat_fn {
  template <class... _Es>
    requires(sizeof...(_Es) == 1 && (std::ranges::input_range<_Es> && ...) &&
             requires { std::views::all(std::declval<_Es>()...); }) ||
            requires { std::ranges::concat_view(std::declval<_Es>()...); }
  [[nodiscard]] constexpr auto operator()(_Es&&... __es) const {
    if constexpr (sizeof...(_Es) == 1 && (std::ranges::input_range<_Es> && ...))
      return std::views::all(static_cast<_Es&&>(__es)...);
    else
      return std::ranges::concat_view(static_cast<_Es&&>(__es)...);
  }
};

}} // namespace __ycxx::__detail::__view_fn

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges::views {
inline constexpr __ycxx::__detail::__view_fn::__join_fn join{};
inline constexpr __ycxx::__detail::__view_fn::__pattern_fn<join_with_view> join_with{};
inline constexpr __ycxx::__detail::__view_fn::__pattern_fn<lazy_split_view> lazy_split{};
inline constexpr __ycxx::__detail::__view_fn::__pattern_fn<split_view> split{};
inline constexpr __ycxx::__detail::__view_fn::__concat_fn concat{};
}}} // namespace std::ranges::views
