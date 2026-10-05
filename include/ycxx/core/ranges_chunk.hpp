// libycxx core: the range adaptors that group or skip elements: chunk, slide, chunk_by and
// stride.
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/bind.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class I>
constexpr I div_ceil(I num, I denom) {
  I r = num / denom;
  if (num % denom)
    ++r;
  return r;
}

template <class V>
concept slide_caches_nothing = std::ranges::random_access_range<V> && std::ranges::sized_range<V>;
template <class V>
concept slide_caches_last =
    !slide_caches_nothing<V> && std::ranges::bidirectional_range<V> && std::ranges::common_range<V>;
template <class V>
concept slide_caches_first = !slide_caches_nothing<V> && !slide_caches_last<V>;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace ranges {

// =============================================================================================
// [range.chunk]
// =============================================================================================
template <view V>
  requires input_range<V>
class chunk_view : public view_interface<chunk_view<V>> {
  V base_;
  range_difference_t<V> n_;
  range_difference_t<V> remainder_ = 0;
  ycxx::detail::non_propagating_cache<iterator_t<V>> current_;

  class inner_iterator {
    friend chunk_view;
    chunk_view* parent_;
    constexpr explicit inner_iterator(chunk_view& parent) noexcept : parent_(__builtin_addressof(parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = range_difference_t<V>;
    using value_type = range_value_t<V>;

    inner_iterator(inner_iterator&&) = default;
    inner_iterator& operator=(inner_iterator&&) = default;

    constexpr const iterator_t<V>& base() const& { return *parent_->current_; }
    constexpr range_reference_t<V> operator*() const {
      ::ycxx::detail::precondition(!(*this == default_sentinel), "chunk_view: dereference past the chunk");
      return **parent_->current_;
    }
    constexpr inner_iterator& operator++() {
      ::ycxx::detail::precondition(!(*this == default_sentinel), "chunk_view: increment past the chunk");
      ++*parent_->current_;
      if (*parent_->current_ == ranges::end(parent_->base_))
        parent_->remainder_ = 0;
      else
        --parent_->remainder_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const inner_iterator& x, default_sentinel_t) { return x.parent_->remainder_ == 0; }
    friend constexpr difference_type operator-(default_sentinel_t, const inner_iterator& x)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return ranges::min(x.parent_->remainder_, ranges::end(x.parent_->base_) - *x.parent_->current_);
    }
    friend constexpr difference_type operator-(const inner_iterator& x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return -(y - x);
    }
    friend constexpr range_rvalue_reference_t<V> iter_move(const inner_iterator& i) noexcept(
        noexcept(ranges::iter_move(*i.parent_->current_))) {
      return ranges::iter_move(*i.parent_->current_);
    }
    friend constexpr void iter_swap(const inner_iterator& x, const inner_iterator& y) noexcept(
        noexcept(ranges::iter_swap(*x.parent_->current_, *y.parent_->current_)))
      requires indirectly_swappable<iterator_t<V>>
    {
      ranges::iter_swap(*x.parent_->current_, *y.parent_->current_);
    }
  };

  class outer_iterator {
    friend chunk_view;
    chunk_view* parent_;
    constexpr explicit outer_iterator(chunk_view& parent) : parent_(__builtin_addressof(parent)) {}

  public:
    using iterator_concept = input_iterator_tag;
    using difference_type = range_difference_t<V>;

    struct value_type : view_interface<value_type> {
    private:
      friend outer_iterator;
      chunk_view* parent_;
      constexpr explicit value_type(chunk_view& parent) : parent_(__builtin_addressof(parent)) {}

    public:
      constexpr inner_iterator begin() const noexcept { return inner_iterator(*parent_); }
      constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
      constexpr auto size() const
        requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
      {
        return ::ycxx::detail::to_unsigned_like(
            ranges::min(parent_->remainder_, ranges::end(parent_->base_) - *parent_->current_));
      }
      constexpr auto reserve_hint() const noexcept { return ::ycxx::detail::to_unsigned_like(parent_->remainder_); }
    };

    outer_iterator(outer_iterator&&) = default;
    outer_iterator& operator=(outer_iterator&&) = default;

    constexpr value_type operator*() const {
      ::ycxx::detail::precondition(!(*this == default_sentinel), "chunk_view: dereference of the end iterator");
      return value_type(*parent_);
    }
    constexpr outer_iterator& operator++() {
      ::ycxx::detail::precondition(!(*this == default_sentinel), "chunk_view: increment of the end iterator");
      ranges::advance(*parent_->current_, parent_->remainder_, ranges::end(parent_->base_));
      parent_->remainder_ = parent_->n_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const outer_iterator& x, default_sentinel_t) {
      return *x.parent_->current_ == ranges::end(x.parent_->base_) && x.parent_->remainder_ != 0;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const outer_iterator& x)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      const auto dist = ranges::end(x.parent_->base_) - *x.parent_->current_;
      if (dist < x.parent_->remainder_)
        return dist == 0 ? 0 : 1;
      return ycxx::detail::div_ceil(dist - x.parent_->remainder_, x.parent_->n_) + 1;
    }
    friend constexpr difference_type operator-(const outer_iterator& x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return -(y - x);
    }
  };

public:
  constexpr explicit chunk_view(V base, range_difference_t<V> n) : base_(std::move(base)), n_(n) {
    ::ycxx::detail::precondition(n > 0, "chunk_view: the chunk size must be positive");
  }

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr outer_iterator begin() {
    current_.emplace(ranges::begin(base_));
    remainder_ = n_;
    return outer_iterator(*this);
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
  constexpr auto size()
    requires sized_range<V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), n_));
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), n_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    auto s = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, n_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    auto s = static_cast<range_difference_t<const V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, n_));
  }
};

template <view V>
  requires forward_range<V>
class chunk_view<V> : public view_interface<chunk_view<V>> {
  template <bool Const>
  class iterator {
    friend chunk_view;
    friend iterator<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, chunk_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();
    sentinel_t<Base> end_ = sentinel_t<Base>();
    range_difference_t<Base> n_ = 0;
    range_difference_t<Base> missing_ = 0;

    constexpr iterator(Parent* parent, iterator_t<Base> current, range_difference_t<Base> missing = 0)
        : current_(current), end_(ranges::end(parent->base_)), n_(parent->n_), missing_(missing) {}

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = decltype(views::take(subrange(current_, end_), n_));
    using difference_type = range_difference_t<Base>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>> && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : current_(std::move(i.current_)), end_(std::move(i.end_)), n_(i.n_), missing_(i.missing_) {}

    constexpr iterator_t<Base> base() const { return current_; }
    constexpr value_type operator*() const {
      ::ycxx::detail::precondition(current_ != end_, "chunk_view: dereference of the end iterator");
      return views::take(subrange(current_, end_), n_);
    }
    constexpr iterator& operator++() {
      ::ycxx::detail::precondition(current_ != end_, "chunk_view: increment of the end iterator");
      missing_ = ranges::advance(current_, n_, end_);
      return *this;
    }
    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<Base>
    {
      ranges::advance(current_, missing_ - n_);
      missing_ = 0;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<Base>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type x)
      requires random_access_range<Base>
    {
      if (x > 0) {
        ::ycxx::detail::precondition(ranges::distance(current_, end_) > n_ * (x - 1),
                                     "chunk_view: advance past the end");
        ranges::advance(current_, n_ * (x - 1));
        missing_ = ranges::advance(current_, n_, end_);
      } else if (x < 0) {
        ranges::advance(current_, n_ * x + missing_);
        missing_ = 0;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires random_access_range<Base>
    {
      return *this += -x;
    }
    constexpr value_type operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return *(*this + n);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) { return x.current_ == y.current_; }
    friend constexpr bool operator==(const iterator& x, default_sentinel_t) { return x.current_ == x.end_; }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ < y.current_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires random_access_range<Base> && three_way_comparable<iterator_t<Base>>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
      return (x.current_ - y.current_ + x.missing_ - y.missing_) / x.n_;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& x)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return ycxx::detail::div_ceil(x.end_ - x.current_, x.n_);
    }
    friend constexpr difference_type operator-(const iterator& x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return -(y - x);
    }
  };

  V base_;
  range_difference_t<V> n_;

public:
  constexpr explicit chunk_view(V base, range_difference_t<V> n) : base_(std::move(base)), n_(n) {
    ::ycxx::detail::precondition(n > 0, "chunk_view: the chunk size must be positive");
  }

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return iterator<false>(this, ranges::begin(base_));
  }
  constexpr auto begin() const
    requires forward_range<const V>
  {
    return iterator<true>(this, ranges::begin(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (common_range<V> && sized_range<V>) {
      auto missing = (n_ - ranges::distance(base_) % n_) % n_;
      return iterator<false>(this, ranges::end(base_), missing);
    } else if constexpr (common_range<V> && !bidirectional_range<V>) {
      return iterator<false>(this, ranges::end(base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto end() const
    requires forward_range<const V>
  {
    if constexpr (common_range<const V> && sized_range<const V>) {
      auto missing = (n_ - ranges::distance(base_) % n_) % n_;
      return iterator<true>(this, ranges::end(base_), missing);
    } else if constexpr (common_range<const V> && !bidirectional_range<const V>) {
      return iterator<true>(this, ranges::end(base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), n_));
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), n_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    auto s = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, n_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    auto s = static_cast<range_difference_t<const V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, n_));
  }
};
template <class R>
chunk_view(R&&, range_difference_t<R>) -> chunk_view<views::all_t<R>>;
template <class V>
constexpr bool enable_borrowed_range<chunk_view<V>> = forward_range<V> && enable_borrowed_range<V>;

// =============================================================================================
// [range.slide]
// =============================================================================================
template <forward_range V>
  requires view<V>
class slide_view : public view_interface<slide_view<V>> {
  class sentinel;

  template <bool Const>
  class iterator {
    friend slide_view;
    friend iterator<!Const>;
    friend sentinel;
    using Base = ycxx::detail::maybe_const<Const, V>;
    static constexpr bool has_last = ycxx::detail::slide_caches_first<Base>;

    iterator_t<Base> current_ = iterator_t<Base>();
    [[no_unique_address]] conditional_t<has_last, iterator_t<Base>, ycxx::detail::empty_cache> last_ele_ =
        conditional_t<has_last, iterator_t<Base>, ycxx::detail::empty_cache>();
    range_difference_t<Base> n_ = 0;

    constexpr iterator(iterator_t<Base> current, range_difference_t<Base> n)
      requires(!ycxx::detail::slide_caches_first<Base>)
        : current_(current), n_(n) {}
    constexpr iterator(iterator_t<Base> current, iterator_t<Base> last_ele, range_difference_t<Base> n)
      requires ycxx::detail::slide_caches_first<Base>
        : current_(current), last_ele_(last_ele), n_(n) {}

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = decltype(views::counted(current_, n_));
    using difference_type = range_difference_t<Base>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)), n_(i.n_) {}

    constexpr auto operator*() const { return views::counted(current_, n_); }
    constexpr iterator& operator++() {
      ++current_;
      if constexpr (has_last)
        ++last_ele_;
      return *this;
    }
    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<Base>
    {
      --current_;
      if constexpr (has_last)
        --last_ele_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<Base>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type x)
      requires random_access_range<Base>
    {
      current_ += x;
      if constexpr (has_last)
        last_ele_ += x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires random_access_range<Base>
    {
      current_ -= x;
      if constexpr (has_last)
        last_ele_ -= x;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return views::counted(current_ + n, n_);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) {
      if constexpr (has_last)
        return x.last_ele_ == y.last_ele_;
      else
        return x.current_ == y.current_;
    }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ < y.current_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires random_access_range<Base> && three_way_comparable<iterator_t<Base>>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
      if constexpr (has_last)
        return x.last_ele_ - y.last_ele_;
      else
        return x.current_ - y.current_;
    }
  };

  class sentinel {
    friend slide_view;
    sentinel_t<V> end_ = sentinel_t<V>();
    constexpr explicit sentinel(sentinel_t<V> end) : end_(end) {}

    static constexpr const auto& last_of(const iterator<false>& x) { return x.last_ele_; }

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator<false>& x, const sentinel& y) { return last_of(x) == y.end_; }
    friend constexpr range_difference_t<V> operator-(const iterator<false>& x, const sentinel& y)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return last_of(x) - y.end_;
    }
    friend constexpr range_difference_t<V> operator-(const sentinel& y, const iterator<false>& x)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return y.end_ - last_of(x);
    }
  };

  V base_;
  range_difference_t<V> n_;
  [[no_unique_address]] ycxx::detail::cache_if<ycxx::detail::slide_caches_first<V> ||
                                                    ycxx::detail::slide_caches_last<V>,
                                                iterator<false>>
      cache_;

  template <class Self>
  static constexpr auto size_of(Self& self) {
    auto sz = ranges::distance(self.base_) - self.n_ + 1;
    if (sz < 0)
      sz = 0;
    return ::ycxx::detail::to_unsigned_like(sz);
  }
  template <class Self>
  static constexpr auto hint_of(Self& self) {
    auto sz = static_cast<range_difference_t<decltype((self.base_))>>(ranges::reserve_hint(self.base_)) - self.n_ + 1;
    if (sz < 0)
      sz = 0;
    return ::ycxx::detail::to_unsigned_like(sz);
  }

public:
  constexpr explicit slide_view(V base, range_difference_t<V> n) : base_(std::move(base)), n_(n) {
    ::ycxx::detail::precondition(n > 0, "slide_view: the window size must be positive");
  }

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!(ycxx::detail::simple_view<V> && ycxx::detail::slide_caches_nothing<const V>))
  {
    if constexpr (ycxx::detail::slide_caches_first<V>) {
      if (!cache_.has_value())
        cache_.emplace(iterator<false>(ranges::begin(base_), ranges::next(ranges::begin(base_), n_ - 1, ranges::end(base_)),
                                       n_));
      return *cache_;
    } else {
      return iterator<false>(ranges::begin(base_), n_);
    }
  }
  constexpr auto begin() const
    requires ycxx::detail::slide_caches_nothing<const V>
  {
    return iterator<true>(ranges::begin(base_), n_);
  }
  constexpr auto end()
    requires(!(ycxx::detail::simple_view<V> && ycxx::detail::slide_caches_nothing<const V>))
  {
    if constexpr (ycxx::detail::slide_caches_nothing<V>) {
      return iterator<false>(ranges::begin(base_) + range_difference_t<V>(size()), n_);
    } else if constexpr (ycxx::detail::slide_caches_last<V>) {
      if (!cache_.has_value())
        cache_.emplace(iterator<false>(ranges::prev(ranges::end(base_), n_ - 1, ranges::begin(base_)), n_));
      return *cache_;
    } else if constexpr (common_range<V>) {
      return iterator<false>(ranges::end(base_), ranges::end(base_), n_);
    } else {
      return sentinel(ranges::end(base_));
    }
  }
  constexpr auto end() const
    requires ycxx::detail::slide_caches_nothing<const V>
  {
    return begin() + range_difference_t<const V>(size());
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return size_of(*this);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return size_of(*this);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return hint_of(*this);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return hint_of(*this);
  }
};
template <class R>
slide_view(R&&, range_difference_t<R>) -> slide_view<views::all_t<R>>;
template <class V>
constexpr bool enable_borrowed_range<slide_view<V>> = enable_borrowed_range<V>;

// =============================================================================================
// [range.chunk.by]
// =============================================================================================
template <forward_range V, indirect_binary_predicate<iterator_t<V>, iterator_t<V>> Pred>
  requires view<V> && is_object_v<Pred>
class chunk_by_view : public view_interface<chunk_by_view<V, Pred>> {
  class iterator {
    friend chunk_by_view;
    chunk_by_view* parent_ = nullptr;
    iterator_t<V> current_ = iterator_t<V>();
    iterator_t<V> next_ = iterator_t<V>();

    constexpr iterator(chunk_by_view& parent, iterator_t<V> current, iterator_t<V> next)
        : parent_(__builtin_addressof(parent)), current_(current), next_(next) {}

  public:
    using value_type = subrange<iterator_t<V>>;
    using difference_type = range_difference_t<V>;
    using iterator_category = input_iterator_tag;
    using iterator_concept = conditional_t<bidirectional_range<V>, bidirectional_iterator_tag, forward_iterator_tag>;

    iterator() = default;
    constexpr value_type operator*() const {
      ::ycxx::detail::precondition(current_ != next_, "chunk_by_view: dereference of the end iterator");
      return subrange(current_, next_);
    }
    constexpr iterator& operator++() {
      ::ycxx::detail::precondition(current_ != next_, "chunk_by_view: increment of the end iterator");
      current_ = next_;
      next_ = parent_->find_next(current_);
      return *this;
    }
    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<V>
    {
      next_ = current_;
      current_ = parent_->find_prev(next_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<V>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    friend constexpr bool operator==(const iterator& x, const iterator& y) { return x.current_ == y.current_; }
    friend constexpr bool operator==(const iterator& x, default_sentinel_t) { return x.current_ == x.next_; }
  };

  V base_ = V();
  [[no_unique_address]] ycxx::detail::movable_box<Pred> pred_;
  ycxx::detail::non_propagating_cache<iterator> begin_;

  constexpr iterator_t<V> find_next(iterator_t<V> current) {
    ::ycxx::detail::precondition(pred_.has_value(), "chunk_by_view: no predicate");
    return ranges::next(ranges::adjacent_find(current, ranges::end(base_), std::not_fn(std::ref(*pred_))), 1,
                        ranges::end(base_));
  }
  constexpr iterator_t<V> find_prev(iterator_t<V> current)
    requires bidirectional_range<V>
  {
    ::ycxx::detail::precondition(pred_.has_value(), "chunk_by_view: no predicate");
    const auto first = ranges::begin(base_);
    ::ycxx::detail::precondition(current != first, "chunk_by_view: decrement of the begin iterator");
    auto i = ranges::prev(current);
    while (i != first) {
      auto p = ranges::prev(i);
      if (!bool(::ycxx::detail::invoke(*pred_, *p, *i)))
        break;
      i = p;
    }
    return i;
  }

public:
  chunk_by_view()
    requires default_initializable<V> && default_initializable<Pred>
  = default;
  constexpr explicit chunk_by_view(V base, Pred pred) : base_(std::move(base)), pred_(in_place, std::move(pred)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
  constexpr const Pred& pred() const { return *pred_; }

  constexpr iterator begin() {
    ::ycxx::detail::precondition(pred_.has_value(), "chunk_by_view: no predicate");
    if (!begin_.has_value())
      begin_.emplace(iterator(*this, ranges::begin(base_), find_next(ranges::begin(base_))));
    return *begin_;
  }
  constexpr auto end() {
    if constexpr (common_range<V>)
      return iterator(*this, ranges::end(base_), ranges::end(base_));
    else
      return default_sentinel;
  }
};
template <class R, class Pred>
chunk_by_view(R&&, Pred) -> chunk_by_view<views::all_t<R>, Pred>;

// =============================================================================================
// [range.stride]
// =============================================================================================
template <input_range V>
  requires view<V>
class stride_view : public view_interface<stride_view<V>> {
  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (derived_from<C, random_access_iterator_tag>)
        return type_identity<random_access_iterator_tag>{};
      else
        return type_identity<C>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend stride_view;
    friend iterator<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, stride_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();
    sentinel_t<Base> end_ = sentinel_t<Base>();
    range_difference_t<Base> stride_ = 0;
    range_difference_t<Base> missing_ = 0;

    constexpr iterator(Parent* parent, iterator_t<Base> current, range_difference_t<Base> missing = 0)
        : current_(std::move(current)), end_(ranges::end(parent->base_)), stride_(parent->stride_), missing_(missing) {}

  public:
    using difference_type = range_difference_t<Base>;
    using value_type = range_value_t<Base>;
    using iterator_concept = ycxx::detail::range_strength_t<Base>;

    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    constexpr iterator(iterator<!Const> other)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>> && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : current_(std::move(other.current_)), end_(std::move(other.end_)), stride_(other.stride_),
          missing_(other.missing_) {}

    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr decltype(auto) operator*() const { return *current_; }
    constexpr iterator& operator++() {
      ::ycxx::detail::precondition(current_ != end_, "stride_view: increment of the end iterator");
      missing_ = ranges::advance(current_, stride_, end_);
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires forward_range<Base>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires bidirectional_range<Base>
    {
      ranges::advance(current_, missing_ - stride_);
      missing_ = 0;
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<Base>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires random_access_range<Base>
    {
      if (n > 0) {
        ::ycxx::detail::precondition(ranges::distance(current_, end_) > stride_ * (n - 1),
                                     "stride_view: advance past the end");
        ranges::advance(current_, stride_ * (n - 1));
        missing_ = ranges::advance(current_, stride_, end_);
      } else if (n < 0) {
        ranges::advance(current_, stride_ * n + missing_);
        missing_ = 0;
      }
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<Base>
    {
      return *this += -n;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return *(*this + n);
    }

    friend constexpr bool operator==(const iterator& x, default_sentinel_t) { return x.current_ == x.end_; }
    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base>>
    {
      return x.current_ == y.current_;
    }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ < y.current_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires random_access_range<Base> && three_way_comparable<iterator_t<Base>>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<Base>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
      auto n = x.current_ - y.current_;
      if constexpr (forward_range<Base>)
        return (n + x.missing_ - y.missing_) / x.stride_;
      else if (n < 0)
        return -ycxx::detail::div_ceil(-n, x.stride_);
      else
        return ycxx::detail::div_ceil(n, x.stride_);
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& x)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return ycxx::detail::div_ceil(x.end_ - x.current_, x.stride_);
    }
    friend constexpr difference_type operator-(const iterator& x, default_sentinel_t y)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return -(y - x);
    }
    friend constexpr range_rvalue_reference_t<Base> iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(i.current_))) {
      return ranges::iter_move(i.current_);
    }
    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(x.current_, y.current_)))
      requires indirectly_swappable<iterator_t<Base>>
    {
      ranges::iter_swap(x.current_, y.current_);
    }
  };

  V base_;
  range_difference_t<V> stride_;

public:
  constexpr explicit stride_view(V base, range_difference_t<V> stride) : base_(std::move(base)), stride_(stride) {
    ::ycxx::detail::precondition(stride > 0, "stride_view: the stride must be positive");
  }

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
  constexpr range_difference_t<V> stride() const noexcept { return stride_; }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return iterator<false>(this, ranges::begin(base_));
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return iterator<true>(this, ranges::begin(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (common_range<V> && sized_range<V> && forward_range<V>) {
      auto missing = (stride_ - ranges::distance(base_) % stride_) % stride_;
      return iterator<false>(this, ranges::end(base_), missing);
    } else if constexpr (common_range<V> && !bidirectional_range<V>) {
      return iterator<false>(this, ranges::end(base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto end() const
    requires range<const V>
  {
    if constexpr (common_range<const V> && sized_range<const V> && forward_range<const V>) {
      auto missing = (stride_ - ranges::distance(base_) % stride_) % stride_;
      return iterator<true>(this, ranges::end(base_), missing);
    } else if constexpr (common_range<const V> && !bidirectional_range<const V>) {
      return iterator<true>(this, ranges::end(base_));
    } else {
      return default_sentinel;
    }
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), stride_));
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(ranges::distance(base_), stride_));
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    auto s = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, stride_));
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    auto s = static_cast<range_difference_t<const V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(ycxx::detail::div_ceil(s, stride_));
  }
};
template <class R>
stride_view(R&&, range_difference_t<R>) -> stride_view<views::all_t<R>>;
template <class V>
constexpr bool enable_borrowed_range<stride_view<V>> = enable_borrowed_range<V>;

}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::view_fn {
// views::X(E, N) is X_view(E, N); views::X(N) binds N.
template <template <class> class View>
struct count_fn {
  template <class E, class N>
    requires requires { View(std::declval<E>(), std::declval<N>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, N&& n) const {
    return View(static_cast<E&&>(e), static_cast<N&&>(n));
  }
  template <class N>
  [[nodiscard]] constexpr auto operator()(N&& n) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<N&&>(n))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<N&&>(n)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<N&&>(n));
  }
};

struct chunk_by_fn {
  template <class E, class P>
    requires requires { std::ranges::chunk_by_view(std::declval<E>(), std::declval<P>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, P&& p) const {
    return std::ranges::chunk_by_view(static_cast<E&&>(e), static_cast<P&&>(p));
  }
  template <class P>
  [[nodiscard]] constexpr auto operator()(P&& p) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p));
  }
};
}} // namespace ycxx::detail::view_fn

namespace [[gnu::visibility("hidden")]] std { namespace ranges::views {
inline constexpr ycxx::detail::view_fn::count_fn<chunk_view> chunk{};
inline constexpr ycxx::detail::view_fn::count_fn<slide_view> slide{};
inline constexpr ycxx::detail::view_fn::chunk_by_fn chunk_by{};
inline constexpr ycxx::detail::view_fn::count_fn<stride_view> stride{};
}} // namespace std::ranges::views
