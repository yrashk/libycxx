// libycxx core: the range adaptors that group or skip elements: chunk, slide, chunk_by and
// stride.
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/bind.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Ip>
constexpr _Ip __div_ceil(_Ip num, _Ip __denom) {
  _Ip r = num / __denom;
  if (num % __denom)
    ++r;
  return r;
}

template <class _Vp>
concept __slide_caches_nothing = std::ranges::random_access_range<_Vp> && std::ranges::sized_range<_Vp>;
template <class _Vp>
concept __slide_caches_last =
    !__slide_caches_nothing<_Vp> && std::ranges::bidirectional_range<_Vp> && std::ranges::common_range<_Vp>;
template <class _Vp>
concept __slide_caches_first = !__slide_caches_nothing<_Vp> && !__slide_caches_last<_Vp>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

// =============================================================================================
// [range.chunk]
// =============================================================================================
template <view _Vp>
  requires input_range<_Vp>
class chunk_view : public view_interface<chunk_view<_Vp>> {
  _Vp __base_;
  range_difference_t<_Vp> __n_;
  range_difference_t<_Vp> __remainder_ = 0;
  __ycxx::__detail::__non_propagating_cache<iterator_t<_Vp>> __current_;

  class __inner_iterator {
    friend chunk_view;
    chunk_view* __parent_;
    constexpr explicit __inner_iterator(chunk_view& __parent) noexcept : __parent_(__builtin_addressof(__parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = range_difference_t<_Vp>;
    using value_type = range_value_t<_Vp>;

    __inner_iterator(__inner_iterator&&) = default;
    __inner_iterator& operator=(__inner_iterator&&) = default;

    constexpr const iterator_t<_Vp>& base() const& { return *__parent_->__current_; }
    constexpr range_reference_t<_Vp> operator*() const {
      ::__ycxx::__detail::__precondition(!(*this == default_sentinel), "chunk_view: dereference past the chunk");
      return **__parent_->__current_;
    }
    constexpr __inner_iterator& operator++() {
      ::__ycxx::__detail::__precondition(!(*this == default_sentinel), "chunk_view: increment past the chunk");
      ++*__parent_->__current_;
      if (*__parent_->__current_ == ranges::end(__parent_->__base_))
        __parent_->__remainder_ = 0;
      else
        --__parent_->__remainder_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const __inner_iterator& __x, default_sentinel_t) { return __x.__parent_->__remainder_ == 0; }
    friend constexpr difference_type operator-(default_sentinel_t, const __inner_iterator& __x)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return ranges::min(__x.__parent_->__remainder_, ranges::end(__x.__parent_->__base_) - *__x.__parent_->__current_);
    }
    friend constexpr difference_type operator-(const __inner_iterator& __x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return -(y - __x);
    }
    friend constexpr range_rvalue_reference_t<_Vp> iter_move(const __inner_iterator& i) noexcept(
        noexcept(ranges::iter_move(*i.__parent_->__current_))) {
      return ranges::iter_move(*i.__parent_->__current_);
    }
    friend constexpr void iter_swap(const __inner_iterator& __x, const __inner_iterator& y) noexcept(
        noexcept(ranges::iter_swap(*__x.__parent_->__current_, *y.__parent_->__current_)))
      requires indirectly_swappable<iterator_t<_Vp>>
    {
      ranges::iter_swap(*__x.__parent_->__current_, *y.__parent_->__current_);
    }
  };

  class __outer_iterator {
    friend chunk_view;
    chunk_view* __parent_;
    constexpr explicit __outer_iterator(chunk_view& __parent) : __parent_(__builtin_addressof(__parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = range_difference_t<_Vp>;

    struct value_type : view_interface<value_type> {
    private:
      friend __outer_iterator;
      chunk_view* __parent_;
      constexpr explicit value_type(chunk_view& __parent) : __parent_(__builtin_addressof(__parent)) {}

    public:
      constexpr __inner_iterator begin() const noexcept { return __inner_iterator(*__parent_); }
      constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
      constexpr auto size() const
        requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
      {
        return ::__ycxx::__detail::__to_unsigned_like(
            ranges::min(__parent_->__remainder_, ranges::end(__parent_->__base_) - *__parent_->__current_));
      }
      constexpr auto reserve_hint() const noexcept { return ::__ycxx::__detail::__to_unsigned_like(__parent_->__remainder_); }
    };

    __outer_iterator(__outer_iterator&&) = default;
    __outer_iterator& operator=(__outer_iterator&&) = default;

    constexpr value_type operator*() const {
      ::__ycxx::__detail::__precondition(!(*this == default_sentinel), "chunk_view: dereference of the end iterator");
      return value_type(*__parent_);
    }
    constexpr __outer_iterator& operator++() {
      ::__ycxx::__detail::__precondition(!(*this == default_sentinel), "chunk_view: increment of the end iterator");
      ranges::advance(*__parent_->__current_, __parent_->__remainder_, ranges::end(__parent_->__base_));
      __parent_->__remainder_ = __parent_->__n_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const __outer_iterator& __x, default_sentinel_t) {
      return *__x.__parent_->__current_ == ranges::end(__x.__parent_->__base_) && __x.__parent_->__remainder_ != 0;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const __outer_iterator& __x)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      const auto __dist = ranges::end(__x.__parent_->__base_) - *__x.__parent_->__current_;
      if (__dist < __x.__parent_->__remainder_)
        return __dist == 0 ? 0 : 1;
      return __ycxx::__detail::__div_ceil(__dist - __x.__parent_->__remainder_, __x.__parent_->__n_) + 1;
    }
    friend constexpr difference_type operator-(const __outer_iterator& __x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return -(y - __x);
    }
  };

public:
  constexpr explicit chunk_view(_Vp base, range_difference_t<_Vp> n) : __base_(std::move(base)), __n_(n) {
    ::__ycxx::__detail::__precondition(n > 0, "chunk_view: the chunk size must be positive");
  }

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr __outer_iterator begin() {
    __current_.emplace(ranges::begin(__base_));
    __remainder_ = __n_;
    return __outer_iterator(*this);
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __n_));
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __n_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    auto s = static_cast<range_difference_t<_Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __n_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    auto s = static_cast<range_difference_t<const _Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __n_));
  }
};

template <view _Vp>
  requires forward_range<_Vp>
class chunk_view<_Vp> : public view_interface<chunk_view<_Vp>> {
  template <bool _Const>
  class iterator {
    friend chunk_view;
    friend iterator<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, chunk_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    range_difference_t<_Base> __n_ = 0;
    range_difference_t<_Base> __missing_ = 0;

    constexpr iterator(_Parent* __parent, iterator_t<_Base> current, range_difference_t<_Base> __missing = 0)
        : __current_(current), __end_(ranges::end(__parent->__base_)), __n_(__parent->__n_), __missing_(__missing) {}

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<_Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<_Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = decltype(views::take(subrange(__current_, __end_), __n_));
    using difference_type = range_difference_t<_Base>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>> && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __current_(std::move(i.__current_)), __end_(std::move(i.__end_)), __n_(i.__n_), __missing_(i.__missing_) {}

    constexpr iterator_t<_Base> base() const { return __current_; }
    constexpr value_type operator*() const {
      ::__ycxx::__detail::__precondition(__current_ != __end_, "chunk_view: dereference of the end iterator");
      return views::take(subrange(__current_, __end_), __n_);
    }
    constexpr iterator& operator++() {
      ::__ycxx::__detail::__precondition(__current_ != __end_, "chunk_view: increment of the end iterator");
      __missing_ = ranges::advance(__current_, __n_, __end_);
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
      ranges::advance(__current_, __missing_ - __n_);
      __missing_ = 0;
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
      if (__x > 0) {
        ::__ycxx::__detail::__precondition(ranges::distance(__current_, __end_) > __n_ * (__x - 1),
                                     "chunk_view: advance past the end");
        ranges::advance(__current_, __n_ * (__x - 1));
        __missing_ = ranges::advance(__current_, __n_, __end_);
      } else if (__x < 0) {
        ranges::advance(__current_, __n_ * __x + __missing_);
        __missing_ = 0;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires random_access_range<_Base>
    {
      return *this += -__x;
    }
    constexpr value_type operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return *(*this + n);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) { return __x.__current_ == y.__current_; }
    friend constexpr bool operator==(const iterator& __x, default_sentinel_t) { return __x.__current_ == __x.__end_; }
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
      return (__x.__current_ - y.__current_ + __x.__missing_ - y.__missing_) / __x.__n_;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& __x)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return __ycxx::__detail::__div_ceil(__x.__end_ - __x.__current_, __x.__n_);
    }
    friend constexpr difference_type operator-(const iterator& __x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return -(y - __x);
    }
  };

  _Vp __base_;
  range_difference_t<_Vp> __n_;

public:
  constexpr explicit chunk_view(_Vp base, range_difference_t<_Vp> n) : __base_(std::move(base)), __n_(n) {
    ::__ycxx::__detail::__precondition(n > 0, "chunk_view: the chunk size must be positive");
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
    return iterator<false>(this, ranges::begin(__base_));
  }
  constexpr auto begin() const
    requires forward_range<const _Vp>
  {
    return iterator<true>(this, ranges::begin(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (common_range<_Vp> && sized_range<_Vp>) {
      auto __missing = (__n_ - ranges::distance(__base_) % __n_) % __n_;
      return iterator<false>(this, ranges::end(__base_), __missing);
    } else if constexpr (common_range<_Vp> && !bidirectional_range<_Vp>) {
      return iterator<false>(this, ranges::end(__base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto end() const
    requires forward_range<const _Vp>
  {
    if constexpr (common_range<const _Vp> && sized_range<const _Vp>) {
      auto __missing = (__n_ - ranges::distance(__base_) % __n_) % __n_;
      return iterator<true>(this, ranges::end(__base_), __missing);
    } else if constexpr (common_range<const _Vp> && !bidirectional_range<const _Vp>) {
      return iterator<true>(this, ranges::end(__base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __n_));
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __n_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    auto s = static_cast<range_difference_t<_Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __n_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    auto s = static_cast<range_difference_t<const _Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __n_));
  }
};
template <class _Rp>
chunk_view(_Rp&&, range_difference_t<_Rp>) -> chunk_view<views::all_t<_Rp>>;
template <class _Vp>
constexpr bool enable_borrowed_range<chunk_view<_Vp>> = forward_range<_Vp> && enable_borrowed_range<_Vp>;

// =============================================================================================
// [range.slide]
// =============================================================================================
template <forward_range _Vp>
  requires view<_Vp>
class slide_view : public view_interface<slide_view<_Vp>> {
  class sentinel;

  template <bool _Const>
  class iterator {
    friend slide_view;
    friend iterator<!_Const>;
    friend sentinel;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    static constexpr bool __has_last = __ycxx::__detail::__slide_caches_first<_Base>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    [[no_unique_address]] conditional_t<__has_last, iterator_t<_Base>, __ycxx::__detail::__empty_cache> __last_ele_ =
        conditional_t<__has_last, iterator_t<_Base>, __ycxx::__detail::__empty_cache>();
    range_difference_t<_Base> __n_ = 0;

    constexpr iterator(iterator_t<_Base> current, range_difference_t<_Base> n)
      requires(!__ycxx::__detail::__slide_caches_first<_Base>)
        : __current_(current), __n_(n) {}
    constexpr iterator(iterator_t<_Base> current, iterator_t<_Base> __last_ele, range_difference_t<_Base> n)
      requires __ycxx::__detail::__slide_caches_first<_Base>
        : __current_(current), __last_ele_(__last_ele), __n_(n) {}

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<_Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<_Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = decltype(views::counted(__current_, __n_));
    using difference_type = range_difference_t<_Base>;

    iterator() = default;
    constexpr iterator(iterator<!_Const> i)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>>
        : __current_(std::move(i.__current_)), __n_(i.__n_) {}

    constexpr auto operator*() const { return views::counted(__current_, __n_); }
    constexpr iterator& operator++() {
      ++__current_;
      if constexpr (__has_last)
        ++__last_ele_;
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
      if constexpr (__has_last)
        --__last_ele_;
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
      if constexpr (__has_last)
        __last_ele_ += __x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type __x)
      requires random_access_range<_Base>
    {
      __current_ -= __x;
      if constexpr (__has_last)
        __last_ele_ -= __x;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return views::counted(__current_ + n, __n_);
    }

    friend constexpr bool operator==(const iterator& __x, const iterator& y) {
      if constexpr (__has_last)
        return __x.__last_ele_ == y.__last_ele_;
      else
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
      if constexpr (__has_last)
        return __x.__last_ele_ - y.__last_ele_;
      else
        return __x.__current_ - y.__current_;
    }
  };

  class sentinel {
    friend slide_view;
    sentinel_t<_Vp> __end_ = sentinel_t<_Vp>();
    constexpr explicit sentinel(sentinel_t<_Vp> end) : __end_(end) {}

    static constexpr const auto& __last_of(const iterator<false>& __x) { return __x.__last_ele_; }

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator<false>& __x, const sentinel& y) { return __last_of(__x) == y.__end_; }
    friend constexpr range_difference_t<_Vp> operator-(const iterator<false>& __x, const sentinel& y)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return __last_of(__x) - y.__end_;
    }
    friend constexpr range_difference_t<_Vp> operator-(const sentinel& y, const iterator<false>& __x)
      requires sized_sentinel_for<sentinel_t<_Vp>, iterator_t<_Vp>>
    {
      return y.__end_ - __last_of(__x);
    }
  };

  _Vp __base_;
  range_difference_t<_Vp> __n_;
  [[no_unique_address]] __ycxx::__detail::__cache_if<__ycxx::__detail::__slide_caches_first<_Vp> ||
                                                    __ycxx::__detail::__slide_caches_last<_Vp>,
                                                iterator<false>>
      __cache_;

  template <class _Self>
  static constexpr auto size_of(_Self& __self) {
    auto __sz = ranges::distance(__self.__base_) - __self.__n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return ::__ycxx::__detail::__to_unsigned_like(__sz);
  }
  template <class _Self>
  static constexpr auto __hint_of(_Self& __self) {
    auto __sz = static_cast<range_difference_t<decltype((__self.__base_))>>(ranges::reserve_hint(__self.__base_)) - __self.__n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return ::__ycxx::__detail::__to_unsigned_like(__sz);
  }

public:
  constexpr explicit slide_view(_Vp base, range_difference_t<_Vp> n) : __base_(std::move(base)), __n_(n) {
    ::__ycxx::__detail::__precondition(n > 0, "slide_view: the window size must be positive");
  }

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }

  constexpr auto begin()
    requires(!(__ycxx::__detail::__simple_view<_Vp> && __ycxx::__detail::__slide_caches_nothing<const _Vp>))
  {
    if constexpr (__ycxx::__detail::__slide_caches_first<_Vp>) {
      if (!__cache_.has_value())
        __cache_.emplace(iterator<false>(ranges::begin(__base_), ranges::next(ranges::begin(__base_), __n_ - 1, ranges::end(__base_)),
                                       __n_));
      return *__cache_;
    } else {
      return iterator<false>(ranges::begin(__base_), __n_);
    }
  }
  constexpr auto begin() const
    requires __ycxx::__detail::__slide_caches_nothing<const _Vp>
  {
    return iterator<true>(ranges::begin(__base_), __n_);
  }
  constexpr auto end()
    requires(!(__ycxx::__detail::__simple_view<_Vp> && __ycxx::__detail::__slide_caches_nothing<const _Vp>))
  {
    if constexpr (__ycxx::__detail::__slide_caches_nothing<_Vp>) {
      return iterator<false>(ranges::begin(__base_) + range_difference_t<_Vp>(size()), __n_);
    } else if constexpr (__ycxx::__detail::__slide_caches_last<_Vp>) {
      if (!__cache_.has_value())
        __cache_.emplace(iterator<false>(ranges::prev(ranges::end(__base_), __n_ - 1, ranges::begin(__base_)), __n_));
      return *__cache_;
    } else if constexpr (common_range<_Vp>) {
      return iterator<false>(ranges::end(__base_), ranges::end(__base_), __n_);
    } else {
      return sentinel(ranges::end(__base_));
    }
  }
  constexpr auto end() const
    requires __ycxx::__detail::__slide_caches_nothing<const _Vp>
  {
    return begin() + range_difference_t<const _Vp>(size());
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return size_of(*this);
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return size_of(*this);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    return __hint_of(*this);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    return __hint_of(*this);
  }
};
template <class _Rp>
slide_view(_Rp&&, range_difference_t<_Rp>) -> slide_view<views::all_t<_Rp>>;
template <class _Vp>
constexpr bool enable_borrowed_range<slide_view<_Vp>> = enable_borrowed_range<_Vp>;

// =============================================================================================
// [range.chunk.by]
// =============================================================================================
template <forward_range _Vp, indirect_binary_predicate<iterator_t<_Vp>, iterator_t<_Vp>> _Pred>
  requires view<_Vp> && is_object_v<_Pred>
class chunk_by_view : public view_interface<chunk_by_view<_Vp, _Pred>> {
  class iterator {
    friend chunk_by_view;
    chunk_by_view* __parent_ = nullptr;
    iterator_t<_Vp> __current_ = iterator_t<_Vp>();
    iterator_t<_Vp> __next_ = iterator_t<_Vp>();

    constexpr iterator(chunk_by_view& __parent, iterator_t<_Vp> current, iterator_t<_Vp> next)
        : __parent_(__builtin_addressof(__parent)), __current_(current), __next_(next) {}

  public:
    using value_type = subrange<iterator_t<_Vp>>;
    using difference_type = range_difference_t<_Vp>;
    using iterator_category = input_iterator_tag;
    using iterator_concept = conditional_t<bidirectional_range<_Vp>, bidirectional_iterator_tag, forward_iterator_tag>;

    iterator() = default;
    constexpr value_type operator*() const {
      ::__ycxx::__detail::__precondition(__current_ != __next_, "chunk_by_view: dereference of the end iterator");
      return subrange(__current_, __next_);
    }
    constexpr iterator& operator++() {
      ::__ycxx::__detail::__precondition(__current_ != __next_, "chunk_by_view: increment of the end iterator");
      __current_ = __next_;
      __next_ = __parent_->__find_next(__current_);
      return *this;
    }
    constexpr iterator operator++(int) {
      auto __tmp = *this;
      ++*this;
      return __tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<_Vp>
    {
      __next_ = __current_;
      __current_ = __parent_->__find_prev(__next_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<_Vp>
    {
      auto __tmp = *this;
      --*this;
      return __tmp;
    }
    friend constexpr bool operator==(const iterator& __x, const iterator& y) { return __x.__current_ == y.__current_; }
    friend constexpr bool operator==(const iterator& __x, default_sentinel_t) { return __x.__current_ == __x.__next_; }
  };

  _Vp __base_ = _Vp();
  [[no_unique_address]] __ycxx::__detail::__movable_box<_Pred> __pred_;
  __ycxx::__detail::__non_propagating_cache<iterator> __begin_;

  constexpr iterator_t<_Vp> __find_next(iterator_t<_Vp> current) {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "chunk_by_view: no predicate");
    return ranges::next(ranges::adjacent_find(current, ranges::end(__base_), std::not_fn(std::ref(*__pred_))), 1,
                        ranges::end(__base_));
  }
  constexpr iterator_t<_Vp> __find_prev(iterator_t<_Vp> current)
    requires bidirectional_range<_Vp>
  {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "chunk_by_view: no predicate");
    const auto first = ranges::begin(__base_);
    ::__ycxx::__detail::__precondition(current != first, "chunk_by_view: decrement of the begin iterator");
    auto i = ranges::prev(current);
    while (i != first) {
      auto p = ranges::prev(i);
      if (!bool(::__ycxx::__detail::invoke(*__pred_, *p, *i)))
        break;
      i = p;
    }
    return i;
  }

public:
  chunk_by_view()
    requires default_initializable<_Vp> && default_initializable<_Pred>
  = default;
  constexpr explicit chunk_by_view(_Vp base, _Pred pred) : __base_(std::move(base)), __pred_(in_place, std::move(pred)) {}

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
  constexpr const _Pred& pred() const { return *__pred_; }

  constexpr iterator begin() {
    ::__ycxx::__detail::__precondition(__pred_.has_value(), "chunk_by_view: no predicate");
    if (!__begin_.has_value())
      __begin_.emplace(iterator(*this, ranges::begin(__base_), __find_next(ranges::begin(__base_))));
    return *__begin_;
  }
  constexpr auto end() {
    if constexpr (common_range<_Vp>)
      return iterator(*this, ranges::end(__base_), ranges::end(__base_));
    else
      return default_sentinel;
  }
};
template <class _Rp, class _Pred>
chunk_by_view(_Rp&&, _Pred) -> chunk_by_view<views::all_t<_Rp>, _Pred>;

// =============================================================================================
// [range.stride]
// =============================================================================================
template <input_range _Vp>
  requires view<_Vp>
class stride_view : public view_interface<stride_view<_Vp>> {
  template <bool _Const>
  static consteval auto category() {
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;
    if constexpr (!forward_range<_Base>) {
      return type_identity<void>{};
    } else {
      using _Cp = __ycxx::__detail::__iter_category_t<iterator_t<_Base>>;
      if constexpr (derived_from<_Cp, random_access_iterator_tag>)
        return type_identity<random_access_iterator_tag>{};
      else
        return type_identity<_Cp>{};
    }
  }

  template <bool _Const>
  class iterator : public __ycxx::__detail::__category_base<typename decltype(category<_Const>())::type> {
    friend stride_view;
    friend iterator<!_Const>;
    using _Parent = __ycxx::__detail::__maybe_const<_Const, stride_view>;
    using _Base = __ycxx::__detail::__maybe_const<_Const, _Vp>;

    iterator_t<_Base> __current_ = iterator_t<_Base>();
    sentinel_t<_Base> __end_ = sentinel_t<_Base>();
    range_difference_t<_Base> __stride_ = 0;
    range_difference_t<_Base> __missing_ = 0;

    constexpr iterator(_Parent* __parent, iterator_t<_Base> current, range_difference_t<_Base> __missing = 0)
        : __current_(std::move(current)), __end_(ranges::end(__parent->__base_)), __stride_(__parent->__stride_), __missing_(__missing) {}

  public:
    using difference_type = range_difference_t<_Base>;
    using value_type = range_value_t<_Base>;
    using iterator_concept = __ycxx::__detail::__range_strength_t<_Base>;

    iterator()
      requires default_initializable<iterator_t<_Base>>
    = default;
    constexpr iterator(iterator<!_Const> other)
      requires _Const && convertible_to<iterator_t<_Vp>, iterator_t<_Base>> && convertible_to<sentinel_t<_Vp>, sentinel_t<_Base>>
        : __current_(std::move(other.__current_)), __end_(std::move(other.__end_)), __stride_(other.__stride_),
          __missing_(other.__missing_) {}

    constexpr iterator_t<_Base> base() && { return std::move(__current_); }
    constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
    constexpr decltype(auto) operator*() const { return *__current_; }
    constexpr iterator& operator++() {
      ::__ycxx::__detail::__precondition(__current_ != __end_, "stride_view: increment of the end iterator");
      __missing_ = ranges::advance(__current_, __stride_, __end_);
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
      ranges::advance(__current_, __missing_ - __stride_);
      __missing_ = 0;
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
      if (n > 0) {
        ::__ycxx::__detail::__precondition(ranges::distance(__current_, __end_) > __stride_ * (n - 1),
                                     "stride_view: advance past the end");
        ranges::advance(__current_, __stride_ * (n - 1));
        __missing_ = ranges::advance(__current_, __stride_, __end_);
      } else if (n < 0) {
        ranges::advance(__current_, __stride_ * n + __missing_);
        __missing_ = 0;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<_Base>
    {
      return *this += -n;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<_Base>
    {
      return *(*this + n);
    }

    friend constexpr bool operator==(const iterator& __x, default_sentinel_t) { return __x.__current_ == __x.__end_; }
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
      auto n = __x.__current_ - y.__current_;
      if constexpr (forward_range<_Base>)
        return (n + __x.__missing_ - y.__missing_) / __x.__stride_;
      else if (n < 0)
        return -__ycxx::__detail::__div_ceil(-n, __x.__stride_);
      else
        return __ycxx::__detail::__div_ceil(n, __x.__stride_);
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& __x)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return __ycxx::__detail::__div_ceil(__x.__end_ - __x.__current_, __x.__stride_);
    }
    friend constexpr difference_type operator-(const iterator& __x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
    {
      return -(y - __x);
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

  _Vp __base_;
  range_difference_t<_Vp> __stride_;

public:
  constexpr explicit stride_view(_Vp base, range_difference_t<_Vp> stride) : __base_(std::move(base)), __stride_(stride) {
    ::__ycxx::__detail::__precondition(stride > 0, "stride_view: the stride must be positive");
  }

  constexpr _Vp base() const&
    requires copy_constructible<_Vp>
  {
    return __base_;
  }
  constexpr _Vp base() && { return std::move(__base_); }
  constexpr range_difference_t<_Vp> stride() const noexcept { return __stride_; }

  constexpr auto begin()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    return iterator<false>(this, ranges::begin(__base_));
  }
  constexpr auto begin() const
    requires range<const _Vp>
  {
    return iterator<true>(this, ranges::begin(__base_));
  }
  constexpr auto end()
    requires(!__ycxx::__detail::__simple_view<_Vp>)
  {
    if constexpr (common_range<_Vp> && sized_range<_Vp> && forward_range<_Vp>) {
      auto __missing = (__stride_ - ranges::distance(__base_) % __stride_) % __stride_;
      return iterator<false>(this, ranges::end(__base_), __missing);
    } else if constexpr (common_range<_Vp> && !bidirectional_range<_Vp>) {
      return iterator<false>(this, ranges::end(__base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto end() const
    requires range<const _Vp>
  {
    if constexpr (common_range<const _Vp> && sized_range<const _Vp> && forward_range<const _Vp>) {
      auto __missing = (__stride_ - ranges::distance(__base_) % __stride_) % __stride_;
      return iterator<true>(this, ranges::end(__base_), __missing);
    } else if constexpr (common_range<const _Vp> && !bidirectional_range<const _Vp>) {
      return iterator<true>(this, ranges::end(__base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto size()
    requires sized_range<_Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __stride_));
  }
  constexpr auto size() const
    requires sized_range<const _Vp>
  {
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(ranges::distance(__base_), __stride_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<_Vp>
  {
    auto s = static_cast<range_difference_t<_Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __stride_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const _Vp>
  {
    auto s = static_cast<range_difference_t<const _Vp>>(ranges::reserve_hint(__base_));
    return ::__ycxx::__detail::__to_unsigned_like(__ycxx::__detail::__div_ceil(s, __stride_));
  }
};
template <class _Rp>
stride_view(_Rp&&, range_difference_t<_Rp>) -> stride_view<views::all_t<_Rp>>;
template <class _Vp>
constexpr bool enable_borrowed_range<stride_view<_Vp>> = enable_borrowed_range<_Vp>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__view_fn {
// views::X(E, N) is X_view(E, N); views::X(N) binds N.
template <template <class> class _View>
struct __count_fn {
  template <class _Ep, class _Np>
    requires requires { _View(std::declval<_Ep>(), std::declval<_Np>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Np&& n) const {
    return _View(static_cast<_Ep&&>(e), static_cast<_Np&&>(n));
  }
  template <class _Np>
  [[nodiscard]] constexpr auto operator()(_Np&& n) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Np&&>(n))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Np&&>(n)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Np&&>(n));
  }
};

struct __chunk_by_fn {
  template <class _Ep, class _Pp>
    requires requires { std::ranges::chunk_by_view(std::declval<_Ep>(), std::declval<_Pp>()); }
  [[nodiscard]] constexpr auto operator()(_Ep&& e, _Pp&& p) const {
    return std::ranges::chunk_by_view(static_cast<_Ep&&>(e), static_cast<_Pp&&>(p));
  }
  template <class _Pp>
  [[nodiscard]] constexpr auto operator()(_Pp&& p) const
      noexcept(noexcept(::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p))))
    requires requires { ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p)); }
  {
    return ::__ycxx::__detail::__bind_adaptor(*this, static_cast<_Pp&&>(p));
  }
};
}} // namespace __ycxx::__detail::__view_fn

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges::views {
inline constexpr __ycxx::__detail::__view_fn::__count_fn<chunk_view> chunk{};
inline constexpr __ycxx::__detail::__view_fn::__count_fn<slide_view> slide{};
inline constexpr __ycxx::__detail::__view_fn::__chunk_by_fn chunk_by{};
inline constexpr __ycxx::__detail::__view_fn::__count_fn<stride_view> stride{};
}} // namespace std::ranges::views
