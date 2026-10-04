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

namespace ycxx::detail {
// (is_span, is_optional and is_subrange come from span.hpp, optional.hpp and pair.hpp.)
template <class T>
inline constexpr bool is_string_view = false;
template <class C, class T>
inline constexpr bool is_string_view<std::basic_string_view<C, T>> = true;
template <class T>
inline constexpr bool is_empty_view = false;
template <class T>
inline constexpr bool is_empty_view<std::ranges::empty_view<T>> = true;
// subrange<I, S, K>::StoreSize ([range.subrange.general]): sized only through a stored size.
// filter_view offers const iteration only for input ranges, whose begin() needs no cache.
template <class V, class Pred>
concept filter_const_iterable = std::ranges::input_range<const V> && !std::ranges::forward_range<const V> &&
                                std::indirect_unary_predicate<const Pred, std::ranges::iterator_t<const V>>;
template <class T>
inline constexpr bool subrange_stores_size = false;
template <class I, class S, std::ranges::subrange_kind K>
inline constexpr bool subrange_stores_size<std::ranges::subrange<I, S, K>> =
    K == std::ranges::subrange_kind::sized && !std::sized_sentinel_for<S, I>;
} // namespace ycxx::detail

namespace std::ranges {

// =============================================================================================
// [range.as.rvalue]
// =============================================================================================
template <view V>
  requires input_range<V>
class as_rvalue_view : public view_interface<as_rvalue_view<V>> {
  V base_ = V();

public:
  as_rvalue_view()
    requires default_initializable<V>
  = default;
  constexpr explicit as_rvalue_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return move_iterator(ranges::begin(base_));
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return move_iterator(ranges::begin(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (common_range<V>)
      return move_iterator(ranges::end(base_));
    else
      return move_sentinel(ranges::end(base_));
  }
  constexpr auto end() const
    requires range<const V>
  {
    if constexpr (common_range<const V>)
      return move_iterator(ranges::end(base_));
    else
      return move_sentinel(ranges::end(base_));
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
as_rvalue_view(R&&) -> as_rvalue_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<as_rvalue_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.filter]
// =============================================================================================
template <input_range V, indirect_unary_predicate<iterator_t<V>> Pred>
  requires view<V> && is_object_v<Pred>
class filter_view : public view_interface<filter_view<V, Pred>> {
  template <bool Const>
  class iterator;
  template <bool Const>
  class sentinel;


  V base_ = V();
  [[no_unique_address]] ycxx::detail::movable_box<Pred> pred_;
  [[no_unique_address]] ycxx::detail::position_cache_if<forward_range<V>, V> begin_;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (derived_from<C, bidirectional_iterator_tag>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<C, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<C>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend filter_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, filter_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();
    Parent* parent_ = nullptr;

    constexpr iterator(Parent& parent, iterator_t<Base> current)
        : current_(std::move(current)), parent_(__builtin_addressof(parent)) {}

  public:
    using iterator_concept =
        conditional_t<Const, input_iterator_tag,
                      conditional_t<bidirectional_range<V>, bidirectional_iterator_tag,
                                    conditional_t<forward_range<V>, forward_iterator_tag, input_iterator_tag>>>;
    using value_type = range_value_t<Base>;
    using difference_type = range_difference_t<Base>;

    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)), parent_(i.parent_) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr range_reference_t<Base> operator*() const { return *current_; }
    constexpr iterator_t<Base> operator->() const
      requires ycxx::detail::has_arrow<iterator_t<Base>> && copyable<iterator_t<Base>>
    {
      return current_;
    }

    constexpr iterator& operator++() {
      current_ = ranges::find_if(std::move(++current_), ranges::end(parent_->base_), std::ref(*parent_->pred_));
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
      do
        --current_;
      while (!::ycxx::detail::invoke(*parent_->pred_, *current_));
      return *this;
    }
    constexpr iterator operator--(int)
      requires bidirectional_range<Base>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base>>
    {
      return x.current_ == y.current_;
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

  template <bool Const>
  class sentinel {
    friend filter_view;
    friend sentinel<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, filter_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(Parent& parent) : end_(ranges::end(parent.base_)) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> other)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(other.end_)) {}
    constexpr sentinel_t<Base> base() const { return end_; }

    template <bool OtherConst>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.end_;
    }
  };

public:
  filter_view()
    requires default_initializable<V> && default_initializable<Pred>
  = default;
  constexpr explicit filter_view(V base, Pred pred) : base_(std::move(base)), pred_(in_place, std::move(pred)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
  constexpr const Pred& pred() const { return *pred_; }

  constexpr iterator<false> begin() {
    ::ycxx::detail::precondition(pred_.has_value(), "filter_view::begin: no predicate");
    if constexpr (forward_range<V>) {
      if (!begin_.has_value())
        begin_.set(base_, ranges::find_if(base_, std::ref(*pred_)));
      return {*this, begin_.get(base_)};
    } else {
      return {*this, ranges::find_if(base_, std::ref(*pred_))};
    }
  }
  constexpr iterator<true> begin() const
    requires ycxx::detail::filter_const_iterable<V, Pred>
  {
    ::ycxx::detail::precondition(pred_.has_value(), "filter_view::begin: no predicate");
    return {*this, ranges::find_if(base_, std::ref(*pred_))};
  }
  constexpr auto end() {
    if constexpr (common_range<V>)
      return iterator<false>{*this, ranges::end(base_)};
    else
      return sentinel<false>{*this};
  }
  constexpr sentinel<true> end() const
    requires ycxx::detail::filter_const_iterable<V, Pred>
  {
    return sentinel<true>{*this};
  }
};
template <class R, class Pred>
filter_view(R&&, Pred) -> filter_view<views::all_t<R>, Pred>;

// =============================================================================================
// [range.transform]
// =============================================================================================
template <input_range V, move_constructible F>
  requires view<V> && is_object_v<F> && regular_invocable<F&, range_reference_t<V>> &&
           ycxx::detail::can_reference<invoke_result_t<F&, range_reference_t<V>>>
class transform_view : public view_interface<transform_view<V, F>> {
  template <bool Const>
  class iterator;
  template <bool Const>
  class sentinel;

  V base_ = V();
  [[no_unique_address]] ycxx::detail::movable_box<F> fun_;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (is_reference_v<invoke_result_t<ycxx::detail::maybe_const<Const, F>&, range_reference_t<Base>>>) {
        if constexpr (derived_from<C, contiguous_iterator_tag>)
          return type_identity<random_access_iterator_tag>{};
        else
          return type_identity<C>{};
      } else {
        return type_identity<input_iterator_tag>{};
      }
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend transform_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, transform_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();
    Parent* parent_ = nullptr;

    constexpr iterator(Parent& parent, iterator_t<Base> current)
        : current_(std::move(current)), parent_(__builtin_addressof(parent)) {}

  public:
    using iterator_concept = ycxx::detail::range_strength_t<Base>;
    using value_type = remove_cvref_t<invoke_result_t<ycxx::detail::maybe_const<Const, F>&, range_reference_t<Base>>>;
    using difference_type = range_difference_t<Base>;

    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)), parent_(i.parent_) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr decltype(auto) operator*() const noexcept(noexcept(::ycxx::detail::invoke(*parent_->fun_, *current_))) {
      return ::ycxx::detail::invoke(*parent_->fun_, *current_);
    }

    constexpr iterator& operator++() {
      ++current_;
      return *this;
    }
    constexpr void operator++(int) { ++current_; }
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
      --current_;
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
      current_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<Base>
    {
      current_ -= n;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return ::ycxx::detail::invoke(*parent_->fun_, current_[n]);
    }

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
    friend constexpr iterator operator+(iterator i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator{*i.parent_, i.current_ + n};
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
      requires random_access_range<Base>
    {
      return iterator{*i.parent_, i.current_ + n};
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator{*i.parent_, i.current_ - n};
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
      return x.current_ - y.current_;
    }
  };

  template <bool Const>
  class sentinel {
    friend transform_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(sentinel_t<Base> end) : end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> i)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(i.end_)) {}
    constexpr sentinel_t<Base> base() const { return end_; }

    template <bool OtherConst>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const iterator<OtherConst>& x,
                                                                                            const sentinel& y) {
      return ycxx::detail::view_access::current(x) - y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const sentinel& y,
                                                                                            const iterator<OtherConst>& x) {
      return y.end_ - ycxx::detail::view_access::current(x);
    }
  };

public:
  transform_view()
    requires default_initializable<V> && default_initializable<F>
  = default;
  constexpr explicit transform_view(V base, F fun) : base_(std::move(base)), fun_(in_place, std::move(fun)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr iterator<false> begin() { return iterator<false>{*this, ranges::begin(base_)}; }
  constexpr iterator<true> begin() const
    requires range<const V> && regular_invocable<const F&, range_reference_t<const V>>
  {
    return iterator<true>{*this, ranges::begin(base_)};
  }
  constexpr sentinel<false> end() { return sentinel<false>{ranges::end(base_)}; }
  constexpr iterator<false> end()
    requires common_range<V>
  {
    return iterator<false>{*this, ranges::end(base_)};
  }
  constexpr sentinel<true> end() const
    requires range<const V> && regular_invocable<const F&, range_reference_t<const V>>
  {
    return sentinel<true>{ranges::end(base_)};
  }
  constexpr iterator<true> end() const
    requires common_range<const V> && regular_invocable<const F&, range_reference_t<const V>>
  {
    return iterator<true>{*this, ranges::end(base_)};
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R, class F>
transform_view(R&&, F) -> transform_view<views::all_t<R>, F>;

// =============================================================================================
// [range.take]
// =============================================================================================
template <view V>
class take_view : public view_interface<take_view<V>> {
  template <bool Const>
  class sentinel {
    friend take_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;
    template <bool OtherConst>
    using CI = counted_iterator<iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(sentinel_t<Base> end) : end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> s)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}
    constexpr sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(const CI<Const>& y, const sentinel& x) {
      return y.count() == 0 || y.base() == x.end_;
    }
    template <bool OtherConst = !Const>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const CI<OtherConst>& y, const sentinel& x) {
      return y.count() == 0 || y.base() == x.end_;
    }
  };

  V base_ = V();
  range_difference_t<V> count_ = 0;

  template <class Self>
  static constexpr auto begin_impl(Self& self) {
    using B = ycxx::detail::maybe_const<is_const_v<Self>, V>;
    if constexpr (sized_range<B>) {
      if constexpr (random_access_range<B>) {
        return ranges::begin(self.base_);
      } else {
        auto sz = range_difference_t<B>(self.size());
        return counted_iterator(ranges::begin(self.base_), sz);
      }
    } else if constexpr (sized_sentinel_for<sentinel_t<B>, iterator_t<B>>) {
      auto it = ranges::begin(self.base_);
      auto sz = std::min(self.count_, ranges::end(self.base_) - it);
      return counted_iterator(std::move(it), sz);
    } else {
      return counted_iterator(ranges::begin(self.base_), self.count_);
    }
  }
  template <class Self>
  static constexpr auto end_impl(Self& self) {
    using B = ycxx::detail::maybe_const<is_const_v<Self>, V>;
    if constexpr (sized_range<B>) {
      if constexpr (random_access_range<B>)
        return ranges::begin(self.base_) + range_difference_t<B>(self.size());
      else
        return default_sentinel;
    } else if constexpr (sized_sentinel_for<sentinel_t<B>, iterator_t<B>>) {
      return default_sentinel;
    } else {
      return sentinel<is_const_v<Self>>{ranges::end(self.base_)};
    }
  }

public:
  take_view()
    requires default_initializable<V>
  = default;
  constexpr explicit take_view(V base, range_difference_t<V> count) : base_(std::move(base)), count_(count) {
    ::ycxx::detail::precondition(count >= 0, "take_view: negative count");
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
    return begin_impl(*this);
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return begin_impl(*this);
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    return end_impl(*this);
  }
  constexpr auto end() const
    requires range<const V>
  {
    return end_impl(*this);
  }
  constexpr auto size()
    requires sized_range<V>
  {
    auto n = ranges::size(base_);
    return ranges::min(n, static_cast<decltype(n)>(count_));
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    auto n = ranges::size(base_);
    return ranges::min(n, static_cast<decltype(n)>(count_));
  }
  constexpr auto reserve_hint() {
    if constexpr (approximately_sized_range<V>) {
      auto n = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
      return ::ycxx::detail::to_unsigned_like(ranges::min(n, count_));
    } else {
      return ::ycxx::detail::to_unsigned_like(count_);
    }
  }
  constexpr auto reserve_hint() const {
    if constexpr (approximately_sized_range<const V>) {
      auto n = static_cast<range_difference_t<const V>>(ranges::reserve_hint(base_));
      return ::ycxx::detail::to_unsigned_like(ranges::min(n, count_));
    } else {
      return ::ycxx::detail::to_unsigned_like(count_);
    }
  }
};
template <class R>
take_view(R&&, range_difference_t<R>) -> take_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<take_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.take.while]
// =============================================================================================
template <view V, class Pred>
  requires input_range<V> && is_object_v<Pred> && indirect_unary_predicate<const Pred, iterator_t<V>>
class take_while_view : public view_interface<take_while_view<V, Pred>> {
  template <bool Const>
  class sentinel {
    friend take_while_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    const Pred* pred_ = nullptr;
    constexpr explicit sentinel(sentinel_t<Base> end, const Pred* pred) : end_(end), pred_(pred) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> s)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)), pred_(s.pred_) {}
    constexpr sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(const iterator_t<Base>& x, const sentinel& y) {
      return y.end_ == x || !::ycxx::detail::invoke(*y.pred_, *x);
    }
    template <bool OtherConst = !Const>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator_t<ycxx::detail::maybe_const<OtherConst, V>>& x, const sentinel& y) {
      return y.end_ == x || !::ycxx::detail::invoke(*y.pred_, *x);
    }
  };

  V base_ = V();
  [[no_unique_address]] ycxx::detail::movable_box<Pred> pred_;

public:
  take_while_view()
    requires default_initializable<V> && default_initializable<Pred>
  = default;
  constexpr explicit take_while_view(V base, Pred pred) : base_(std::move(base)), pred_(in_place, std::move(pred)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
  constexpr const Pred& pred() const { return *pred_; }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return ranges::begin(base_);
  }
  constexpr auto begin() const
    requires range<const V> && indirect_unary_predicate<const Pred, iterator_t<const V>>
  {
    return ranges::begin(base_);
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    return sentinel<false>(ranges::end(base_), __builtin_addressof(*pred_));
  }
  constexpr auto end() const
    requires range<const V> && indirect_unary_predicate<const Pred, iterator_t<const V>>
  {
    return sentinel<true>(ranges::end(base_), __builtin_addressof(*pred_));
  }
};
template <class R, class Pred>
take_while_view(R&&, Pred) -> take_while_view<views::all_t<R>, Pred>;

// =============================================================================================
// [range.drop]
// =============================================================================================
template <view V>
class drop_view : public view_interface<drop_view<V>> {
  // begin() is cached for forward ranges whose start cannot be computed in O(1).
  static constexpr bool caches = forward_range<V> && !(random_access_range<V> && sized_range<V>);

  V base_ = V();
  range_difference_t<V> count_ = 0;
  [[no_unique_address]] ycxx::detail::position_cache_if<caches, V> begin_;

public:
  drop_view()
    requires default_initializable<V>
  = default;
  constexpr explicit drop_view(V base, range_difference_t<V> count) : base_(std::move(base)), count_(count) {
    ::ycxx::detail::precondition(count >= 0, "drop_view: negative count");
  }

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!(ycxx::detail::simple_view<V> && random_access_range<const V> && sized_range<const V>))
  {
    if constexpr (random_access_range<V> && sized_range<V>) {
      return ranges::begin(base_) + std::min<range_difference_t<V>>(count_, ranges::distance(base_));
    } else if constexpr (caches) {
      if (!begin_.has_value())
        begin_.set(base_, ranges::next(ranges::begin(base_), count_, ranges::end(base_)));
      return begin_.get(base_);
    } else {
      return ranges::next(ranges::begin(base_), count_, ranges::end(base_));
    }
  }
  // For sized random-access ranges ranges::next(begin, count_, end) is computed in O(1) without
  // comparing against the sentinel.
  constexpr auto begin() const
    requires random_access_range<const V> && sized_range<const V>
  {
    return ranges::begin(base_) + std::min<range_difference_t<const V>>(count_, ranges::distance(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    return ranges::end(base_);
  }
  constexpr auto end() const
    requires range<const V>
  {
    return ranges::end(base_);
  }
  constexpr auto size()
    requires sized_range<V>
  {
    const auto s = ranges::size(base_);
    const auto c = static_cast<decltype(s)>(count_);
    return s < c ? 0 : s - c;
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    const auto s = ranges::size(base_);
    const auto c = static_cast<decltype(s)>(count_);
    return s < c ? 0 : s - c;
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    const auto s = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(s < count_ ? 0 : s - count_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    const auto s = static_cast<range_difference_t<const V>>(ranges::reserve_hint(base_));
    return ::ycxx::detail::to_unsigned_like(s < count_ ? 0 : s - count_);
  }
};
template <class R>
drop_view(R&&, range_difference_t<R>) -> drop_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<drop_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.drop.while]
// =============================================================================================
template <view V, class Pred>
  requires input_range<V> && is_object_v<Pred> && indirect_unary_predicate<const Pred, iterator_t<V>>
class drop_while_view : public view_interface<drop_while_view<V, Pred>> {
  V base_ = V();
  [[no_unique_address]] ycxx::detail::movable_box<Pred> pred_;
  [[no_unique_address]] ycxx::detail::position_cache_if<forward_range<V>, V> begin_;

public:
  drop_while_view()
    requires default_initializable<V> && default_initializable<Pred>
  = default;
  constexpr explicit drop_while_view(V base, Pred pred) : base_(std::move(base)), pred_(in_place, std::move(pred)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
  constexpr const Pred& pred() const { return *pred_; }

  constexpr auto begin() {
    ::ycxx::detail::precondition(pred_.has_value(), "drop_while_view::begin: no predicate");
    if constexpr (forward_range<V>) {
      if (!begin_.has_value())
        begin_.set(base_, ranges::find_if_not(base_, std::cref(*pred_)));
      return begin_.get(base_);
    } else {
      return ranges::find_if_not(base_, std::cref(*pred_));
    }
  }
  constexpr auto end() { return ranges::end(base_); }
};
template <class R, class Pred>
drop_while_view(R&&, Pred) -> drop_while_view<views::all_t<R>, Pred>;
template <class T, class Pred>
constexpr bool enable_borrowed_range<drop_while_view<T, Pred>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.common]
// =============================================================================================
template <view V>
  requires(!common_range<V> && copyable<iterator_t<V>>)
class common_view : public view_interface<common_view<V>> {
  V base_ = V();

public:
  common_view()
    requires default_initializable<V>
  = default;
  constexpr explicit common_view(V r) : base_(std::move(r)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (random_access_range<V> && sized_range<V>)
      return ranges::begin(base_);
    else
      return common_iterator<iterator_t<V>, sentinel_t<V>>(ranges::begin(base_));
  }
  constexpr auto begin() const
    requires range<const V>
  {
    if constexpr (random_access_range<const V> && sized_range<const V>)
      return ranges::begin(base_);
    else
      return common_iterator<iterator_t<const V>, sentinel_t<const V>>(ranges::begin(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (random_access_range<V> && sized_range<V>)
      return ranges::begin(base_) + ranges::distance(base_);
    else
      return common_iterator<iterator_t<V>, sentinel_t<V>>(ranges::end(base_));
  }
  constexpr auto end() const
    requires range<const V>
  {
    if constexpr (random_access_range<const V> && sized_range<const V>)
      return ranges::begin(base_) + ranges::distance(base_);
    else
      return common_iterator<iterator_t<const V>, sentinel_t<const V>>(ranges::end(base_));
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
common_view(R&&) -> common_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<common_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.reverse]
// =============================================================================================
template <view V>
  requires bidirectional_range<V>
class reverse_view : public view_interface<reverse_view<V>> {
  V base_ = V();
  [[no_unique_address]] ycxx::detail::position_cache_if<!common_range<V>, V> begin_;

public:
  reverse_view()
    requires default_initializable<V>
  = default;
  constexpr explicit reverse_view(V r) : base_(std::move(r)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr reverse_iterator<iterator_t<V>> begin() {
    if (!begin_.has_value())
      begin_.set(base_, ranges::next(ranges::begin(base_), ranges::end(base_)));
    return std::make_reverse_iterator(begin_.get(base_));
  }
  constexpr reverse_iterator<iterator_t<V>> begin()
    requires common_range<V>
  {
    return std::make_reverse_iterator(ranges::end(base_));
  }
  constexpr auto begin() const
    requires common_range<const V>
  {
    return std::make_reverse_iterator(ranges::end(base_));
  }
  constexpr reverse_iterator<iterator_t<V>> end() { return std::make_reverse_iterator(ranges::begin(base_)); }
  constexpr auto end() const
    requires common_range<const V>
  {
    return std::make_reverse_iterator(ranges::begin(base_));
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
reverse_view(R&&) -> reverse_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<reverse_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.as.const]
// =============================================================================================
template <view V>
  requires input_range<V>
class as_const_view : public view_interface<as_const_view<V>> {
  V base_ = V();

public:
  as_const_view()
    requires default_initializable<V>
  = default;
  constexpr explicit as_const_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return ranges::cbegin(base_);
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return ranges::cbegin(base_);
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    return ranges::cend(base_);
  }
  constexpr auto end() const
    requires range<const V>
  {
    return ranges::cend(base_);
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
as_const_view(R&&) -> as_const_view<views::all_t<R>>;
template <class T>
constexpr bool enable_borrowed_range<as_const_view<T>> = enable_borrowed_range<T>;

// =============================================================================================
// [range.cache.latest]
// =============================================================================================
template <input_range V>
  requires view<V>
class cache_latest_view : public view_interface<cache_latest_view<V>> {
  using cache_t = conditional_t<is_reference_v<range_reference_t<V>>, add_pointer_t<range_reference_t<V>>,
                                range_reference_t<V>>;

  class sentinel;
  class iterator {
    friend cache_latest_view;
    friend ycxx::detail::view_access;
    cache_latest_view* parent_;
    iterator_t<V> current_;

    constexpr explicit iterator(cache_latest_view& parent)
        : parent_(__builtin_addressof(parent)), current_(ranges::begin(parent.base_)) {}

  public:
    using difference_type = range_difference_t<V>;
    using value_type = range_value_t<V>;
    using iterator_concept = input_iterator_tag;

    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;

    constexpr iterator_t<V> base() && { return std::move(current_); }
    constexpr const iterator_t<V>& base() const& noexcept { return current_; }

    constexpr range_reference_t<V>& operator*() const {
      if constexpr (is_reference_v<range_reference_t<V>>) {
        if (!parent_->cache_.has_value())
          parent_->cache_.emplace(__builtin_addressof(ycxx::detail::as_lvalue(*current_)));
        return **parent_->cache_;
      } else {
        if (!parent_->cache_.has_value())
          parent_->cache_.emplace_deref(current_);
        return *parent_->cache_;
      }
    }
    constexpr iterator& operator++() {
      parent_->cache_.reset();
      ++current_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr range_rvalue_reference_t<V> iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(i.current_))) {
      return ranges::iter_move(i.current_);
    }
    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(x.current_, y.current_)))
      requires indirectly_swappable<iterator_t<V>>
    {
      ranges::iter_swap(x.current_, y.current_);
    }
  };

  class sentinel {
    friend cache_latest_view;
    sentinel_t<V> end_ = sentinel_t<V>();
    constexpr explicit sentinel(cache_latest_view& parent) : end_(ranges::end(parent.base_)) {}

  public:
    sentinel() = default;
    constexpr sentinel_t<V> base() const { return end_; }
    friend constexpr bool operator==(const iterator& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.end_;
    }
    friend constexpr range_difference_t<V> operator-(const iterator& x, const sentinel& y)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return ycxx::detail::view_access::current(x) - y.end_;
    }
    friend constexpr range_difference_t<V> operator-(const sentinel& x, const iterator& y)
      requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>>
    {
      return x.end_ - ycxx::detail::view_access::current(y);
    }
  };

  V base_ = V();
  ycxx::detail::non_propagating_cache<cache_t> cache_;

public:
  cache_latest_view()
    requires default_initializable<V>
  = default;
  constexpr explicit cache_latest_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin() { return iterator(*this); }
  constexpr auto end() { return sentinel(*this); }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
cache_latest_view(R&&) -> cache_latest_view<views::all_t<R>>;

// =============================================================================================
// [range.as.input]
// =============================================================================================
template <input_range V>
  requires view<V>
class as_input_view : public view_interface<as_input_view<V>> {
  template <bool Const>
  class iterator {
    friend as_input_view;
    friend iterator<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();
    constexpr explicit iterator(iterator_t<Base> current) : current_(std::move(current)) {}

  public:
    using difference_type = range_difference_t<Base>;
    using value_type = range_value_t<Base>;
    using iterator_concept = input_iterator_tag;

    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)) {}

    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr decltype(auto) operator*() const { return *current_; }
    constexpr iterator& operator++() {
      ++current_;
      return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const iterator& x, const sentinel_t<Base>& y) { return x.current_ == y; }
    friend constexpr difference_type operator-(const sentinel_t<Base>& y, const iterator& x)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return y - x.current_;
    }
    friend constexpr difference_type operator-(const iterator& x, const sentinel_t<Base>& y)
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
      return x.current_ - y;
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

  V base_ = V();

public:
  as_input_view()
    requires default_initializable<V>
  = default;
  constexpr explicit as_input_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return iterator<false>(ranges::begin(base_));
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return iterator<true>(ranges::begin(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    return ranges::end(base_);
  }
  constexpr auto end() const
    requires range<const V>
  {
    return ranges::end(base_);
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return ranges::size(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return ranges::size(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return ranges::reserve_hint(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return ranges::reserve_hint(base_);
  }
};
template <class R>
as_input_view(R&&) -> as_input_view<views::all_t<R>>;
template <class V>
constexpr bool enable_borrowed_range<as_input_view<V>> = enable_borrowed_range<V>;

} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace ycxx::detail::view_fn {

struct as_rvalue_fn : std::ranges::range_adaptor_closure<as_rvalue_fn> {
  template <class E>
    requires requires { std::ranges::as_rvalue_view(std::declval<E>()); } ||
             (std::ranges::input_range<E> &&
              std::same_as<std::ranges::range_rvalue_reference_t<E>, std::ranges::range_reference_t<E>> &&
              requires { std::views::all(std::declval<E>()); })
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    if constexpr (std::ranges::input_range<E> &&
                  std::same_as<std::ranges::range_rvalue_reference_t<E>, std::ranges::range_reference_t<E>>)
      return std::views::all(static_cast<E&&>(e));
    else
      return std::ranges::as_rvalue_view(static_cast<E&&>(e));
  }
};

struct filter_fn {
  template <class E, class P>
    requires requires { std::ranges::filter_view(std::declval<E>(), std::declval<P>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, P&& p) const {
    return std::ranges::filter_view(static_cast<E&&>(e), static_cast<P&&>(p));
  }
  template <class P>
  [[nodiscard]] constexpr auto operator()(P&& p) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p));
  }
};

struct transform_fn {
  template <class E, class F>
    requires requires { std::ranges::transform_view(std::declval<E>(), std::declval<F>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    return std::ranges::transform_view(static_cast<E&&>(e), static_cast<F&&>(f));
  }
  template <class F>
  [[nodiscard]] constexpr auto operator()(F&& f) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f));
  }
};

// views::take / views::drop: the type-preserving cases of [range.take.overview]/2 and
// [range.drop.overview]/2.
template <class E, class F>
concept take_drop_args = std::ranges::viewable_range<E> && std::convertible_to<F, std::ranges::range_difference_t<E>>;

template <class T>
concept sized_ra = std::ranges::random_access_range<T> && std::ranges::sized_range<T>;

struct take_fn {
  template <class E, class F>
    requires take_drop_args<E, F>
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    using T = std::remove_cvref_t<E>;
    using D = std::ranges::range_difference_t<E>;
    if constexpr (is_empty_view<T>) {
      (void)f;
      return ::ycxx::detail::decay_copy(static_cast<E&&>(e));
    } else if constexpr (is_optional<T> && std::ranges::view<T>) {
      return static_cast<D>(static_cast<F&&>(f)) == D() ? ((void)e, T()) : ::ycxx::detail::decay_copy(static_cast<E&&>(e));
    } else if constexpr (sized_ra<T> && (is_span<T> || is_string_view<T> || is_subrange<T>)) {
      auto&& r = e;
      auto first = std::ranges::begin(r);
      auto n = std::min<D>(std::ranges::distance(r), static_cast<D>(static_cast<F&&>(f)));
      if constexpr (is_span<T>)
        return std::span<typename T::element_type>(first, first + n);
      else if constexpr (is_string_view<T>)
        return T(first, first + n);
      else
        return std::ranges::subrange<std::ranges::iterator_t<T>>(first, first + n);
    } else if constexpr (is_iota_view<T> && sized_ra<T>) {
      auto&& r = e;
      auto first = std::ranges::begin(r);
      auto n = std::min<D>(std::ranges::distance(r), static_cast<D>(static_cast<F&&>(f)));
      return std::ranges::iota_view(*first, *(first + n));
    } else if constexpr (is_repeat_view<T>) {
      if constexpr (std::ranges::sized_range<T>) {
        auto&& r = e;
        auto n = std::min<D>(std::ranges::distance(r), static_cast<D>(static_cast<F&&>(f)));
        return std::views::repeat(repeat_access::value(static_cast<E&&>(r)), n);
      } else {
        return std::views::repeat(repeat_access::value(static_cast<E&&>(e)), static_cast<D>(static_cast<F&&>(f)));
      }
    } else {
      return std::ranges::take_view(static_cast<E&&>(e), static_cast<F&&>(f));
    }
  }
  template <class F>
  [[nodiscard]] constexpr auto operator()(F&& f) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f));
  }
};

struct drop_fn {
  template <class E, class F>
    requires take_drop_args<E, F>
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    using T = std::remove_cvref_t<E>;
    using D = std::ranges::range_difference_t<E>;
    if constexpr (is_empty_view<T>) {
      (void)f;
      return ::ycxx::detail::decay_copy(static_cast<E&&>(e));
    } else if constexpr (is_optional<T> && std::ranges::view<T>) {
      return static_cast<D>(static_cast<F&&>(f)) == D() ? ::ycxx::detail::decay_copy(static_cast<E&&>(e)) : ((void)e, T());
    } else if constexpr (sized_ra<T> &&
                         (is_span<T> || is_string_view<T> || is_iota_view<T> ||
                          (is_subrange<T> && !subrange_stores_size<T>))) {
      auto&& r = e;
      auto n = std::min<D>(std::ranges::distance(r), static_cast<D>(static_cast<F&&>(f)));
      if constexpr (is_span<T>)
        return std::span<typename T::element_type>(std::ranges::begin(r) + n, std::ranges::end(r));
      else
        return T(std::ranges::begin(r) + n, std::ranges::end(r));
    } else if constexpr (is_subrange<T> && sized_ra<T>) {
      auto&& r = e;
      auto d = std::ranges::distance(r);
      auto n = std::min<D>(d, static_cast<D>(static_cast<F&&>(f)));
      return T(std::ranges::begin(r) + n, std::ranges::end(r), ::ycxx::detail::to_unsigned_like(d - n));
    } else if constexpr (is_repeat_view<T>) {
      if constexpr (std::ranges::sized_range<T>) {
        auto&& r = e;
        auto d = std::ranges::distance(r);
        return std::views::repeat(repeat_access::value(static_cast<E&&>(r)), d - std::min<D>(d, static_cast<D>(static_cast<F&&>(f))));
      } else {
        (void)f;
        return ::ycxx::detail::decay_copy(static_cast<E&&>(e));
      }
    } else {
      return std::ranges::drop_view(static_cast<E&&>(e), static_cast<F&&>(f));
    }
  }
  template <class F>
  [[nodiscard]] constexpr auto operator()(F&& f) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f));
  }
};

struct take_while_fn {
  template <class E, class P>
    requires requires { std::ranges::take_while_view(std::declval<E>(), std::declval<P>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, P&& p) const {
    return std::ranges::take_while_view(static_cast<E&&>(e), static_cast<P&&>(p));
  }
  template <class P>
  [[nodiscard]] constexpr auto operator()(P&& p) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p));
  }
};

struct drop_while_fn {
  template <class E, class P>
    requires requires { std::ranges::drop_while_view(std::declval<E>(), std::declval<P>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, P&& p) const {
    return std::ranges::drop_while_view(static_cast<E&&>(e), static_cast<P&&>(p));
  }
  template <class P>
  [[nodiscard]] constexpr auto operator()(P&& p) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<P&&>(p));
  }
};

// The selected form of views::counted(E, F) is well-formed.
template <class E, class F>
concept counted_ok =
    std::input_or_output_iterator<std::decay_t<E>> && std::convertible_to<F, std::iter_difference_t<std::decay_t<E>>> &&
    (std::contiguous_iterator<std::decay_t<E>> ||
     (std::random_access_iterator<std::decay_t<E>> && std::constructible_from<std::decay_t<E>, E>) ||
     requires(E&& e, std::iter_difference_t<std::decay_t<E>> n) { std::counted_iterator(static_cast<E&&>(e), n); });

struct counted_fn {
  template <class E, class F>
    requires counted_ok<E, F>
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    using T = std::decay_t<E>;
    using D = std::iter_difference_t<T>;
    if constexpr (std::contiguous_iterator<T>) {
      return std::span(std::to_address(e), static_cast<std::size_t>(static_cast<D>(static_cast<F&&>(f))));
    } else if constexpr (std::random_access_iterator<T>) {
      T it = static_cast<E&&>(e);
      auto last = it + static_cast<D>(static_cast<F&&>(f));
      return std::ranges::subrange(std::move(it), std::move(last));
    } else {
      return std::ranges::subrange(std::counted_iterator(static_cast<E&&>(e), static_cast<F&&>(f)), std::default_sentinel);
    }
  }
};

struct common_fn : std::ranges::range_adaptor_closure<common_fn> {
  template <class E>
    requires(std::ranges::common_range<E> && requires { std::views::all(std::declval<E>()); }) ||
            requires { std::ranges::common_view{std::declval<E>()}; }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    if constexpr (std::ranges::common_range<E> && requires { std::views::all(std::declval<E>()); })
      return std::views::all(static_cast<E&&>(e));
    else
      return std::ranges::common_view{static_cast<E&&>(e)};
  }
};

template <class T>
inline constexpr bool is_reverse_view = false;
template <class V>
inline constexpr bool is_reverse_view<std::ranges::reverse_view<V>> = true;
template <class T>
inline constexpr bool is_reversed_subrange = false;
template <class I, std::ranges::subrange_kind K>
inline constexpr bool is_reversed_subrange<std::ranges::subrange<std::reverse_iterator<I>, std::reverse_iterator<I>, K>> =
    true;
template <class T>
struct reversed_subrange;
template <class I, std::ranges::subrange_kind K>
struct reversed_subrange<std::ranges::subrange<std::reverse_iterator<I>, std::reverse_iterator<I>, K>> {
  using type = std::ranges::subrange<I, I, K>;
  static constexpr bool sized = K == std::ranges::subrange_kind::sized;
};

struct reverse_fn : std::ranges::range_adaptor_closure<reverse_fn> {
  template <class E>
    requires is_reverse_view<std::remove_cvref_t<E>> ||
             (is_optional<std::remove_cvref_t<E>> && std::ranges::view<std::remove_cvref_t<E>>) ||
             is_reversed_subrange<std::remove_cvref_t<E>> || requires { std::ranges::reverse_view{std::declval<E>()}; }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    using T = std::remove_cvref_t<E>;
    if constexpr (is_reverse_view<T>) {
      return static_cast<E&&>(e).base();
    } else if constexpr (is_optional<T> && std::ranges::view<T>) {
      return ::ycxx::detail::decay_copy(static_cast<E&&>(e));
    } else if constexpr (is_reversed_subrange<T>) {
      using R = reversed_subrange<T>;
      auto&& r = e;
      if constexpr (R::sized)
        return typename R::type(r.end().base(), r.begin().base(), r.size());
      else
        return typename R::type(r.end().base(), r.begin().base());
    } else {
      return std::ranges::reverse_view{static_cast<E&&>(e)};
    }
  }
};

template <class T>
struct empty_view_elem;
template <class X>
struct empty_view_elem<std::ranges::empty_view<X>> {
  using type = X;
};
template <class T>
inline constexpr bool is_optional_ref = false;
template <class X>
inline constexpr bool is_optional_ref<std::optional<X&>> = true;
template <class T>
inline constexpr bool is_ref_view = false;
template <class X>
inline constexpr bool is_ref_view<std::ranges::ref_view<X>> = true;

template <class T>
concept all_t_constant = std::ranges::viewable_range<T> && std::ranges::constant_range<std::views::all_t<T>>;
template <class U>
concept ref_view_of_constant =
    is_ref_view<U> && std::ranges::constant_range<const std::remove_reference_t<decltype(std::declval<U&>().base())>>;

struct as_const_fn : std::ranges::range_adaptor_closure<as_const_fn> {
  template <class E>
  static consteval int kind() {
    using T = E;
    using U = std::remove_cvref_t<T>;
    if constexpr (all_t_constant<T>)
      return 1;
    else if constexpr (is_empty_view<U>)
      return 2;
    else if constexpr (is_optional_ref<U>)
      return 3;
    else if constexpr (is_span<U>)
      return 4;
    else if constexpr (ref_view_of_constant<U>)
      return 5;
    else if constexpr (std::is_lvalue_reference_v<E> && std::ranges::constant_range<const U> && !std::ranges::view<U>)
      return 6;
    else if constexpr (requires { std::ranges::as_const_view(std::declval<E>()); })
      return 7;
    else
      return 0;
  }
  template <class E>
    requires(kind<E>() != 0)
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    using U = std::remove_cvref_t<E>;
    constexpr int k = kind<E>();
    if constexpr (k == 1) {
      return std::views::all(static_cast<E&&>(e));
    } else if constexpr (k == 2) {
      return auto(std::views::empty<const typename empty_view_elem<U>::type>);
    } else if constexpr (k == 3) {
      return std::optional<const std::remove_reference_t<decltype(*e)>&>(static_cast<E&&>(e));
    } else if constexpr (k == 4) {
      return std::span<const typename U::element_type, U::extent>(static_cast<E&&>(e));
    } else if constexpr (k == 5) {
      using X = std::remove_reference_t<decltype(e.base())>;
      return std::ranges::ref_view(static_cast<const X&>(e.base()));
    } else if constexpr (k == 6) {
      return std::ranges::ref_view(static_cast<const U&>(e));
    } else {
      return std::ranges::as_const_view(static_cast<E&&>(e));
    }
  }
};

struct cache_latest_fn : std::ranges::range_adaptor_closure<cache_latest_fn> {
  template <class E>
    requires requires { std::ranges::cache_latest_view(std::declval<E>()); }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    return std::ranges::cache_latest_view(static_cast<E&&>(e));
  }
};

struct as_input_fn : std::ranges::range_adaptor_closure<as_input_fn> {
  template <class E>
    requires(std::ranges::input_range<E> && !std::ranges::common_range<E> && !std::ranges::forward_range<E> &&
             requires { std::views::all(std::declval<E>()); }) ||
            requires { std::ranges::as_input_view(std::declval<E>()); }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    if constexpr (std::ranges::input_range<E> && !std::ranges::common_range<E> && !std::ranges::forward_range<E>)
      return std::views::all(static_cast<E&&>(e));
    else
      return std::ranges::as_input_view(static_cast<E&&>(e));
  }
};

} // namespace ycxx::detail::view_fn

namespace std::ranges::views {
inline constexpr ycxx::detail::view_fn::as_rvalue_fn as_rvalue{};
inline constexpr ycxx::detail::view_fn::filter_fn filter{};
inline constexpr ycxx::detail::view_fn::transform_fn transform{};
inline constexpr ycxx::detail::view_fn::take_fn take{};
inline constexpr ycxx::detail::view_fn::take_while_fn take_while{};
inline constexpr ycxx::detail::view_fn::drop_fn drop{};
inline constexpr ycxx::detail::view_fn::drop_while_fn drop_while{};
inline constexpr ycxx::detail::view_fn::counted_fn counted{};
inline constexpr ycxx::detail::view_fn::common_fn common{};
inline constexpr ycxx::detail::view_fn::reverse_fn reverse{};
inline constexpr ycxx::detail::view_fn::as_const_fn as_const{};
inline constexpr ycxx::detail::view_fn::cache_latest_fn cache_latest{};
inline constexpr ycxx::detail::view_fn::as_input_fn as_input{};
} // namespace std::ranges::views
