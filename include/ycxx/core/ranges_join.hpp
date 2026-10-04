// libycxx core: the range adaptors that join or split sequences: join, join_with, lazy_split,
// split and concat.
#pragma once

#include <ycxx/core/ranges_zip.hpp>
#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/variant.hpp>

namespace ycxx::detail {

// Calls f(integral_constant<size_t, I>{}) for the I in [0, N) equal to i.
template <std::size_t N, class F>
constexpr void with_index(std::size_t i, F&& f) {
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    (void)((i == I ? (f(std::integral_constant<std::size_t, I>{}), true) : false) || ...);
  }(std::make_index_sequence<N>{});
}

template <class R>
concept bidirectional_common = std::ranges::bidirectional_range<R> && std::ranges::common_range<R>;

// ---- [range.concat.view] ------------------------------------------------------------------------
template <class... Rs>
using concat_reference_t = std::common_reference_t<std::ranges::range_reference_t<Rs>...>;
template <class... Rs>
using concat_value_t = std::common_type_t<std::ranges::range_value_t<Rs>...>;
template <class... Rs>
using concat_rvalue_reference_t = std::common_reference_t<std::ranges::range_rvalue_reference_t<Rs>...>;

template <class Ref, class RRef, class It>
concept concat_indirectly_readable_impl = requires(const It it) {
  { *it } -> std::convertible_to<Ref>;
  { std::ranges::iter_move(it) } -> std::convertible_to<RRef>;
};
template <class... Rs>
concept concat_indirectly_readable =
    std::common_reference_with<concat_reference_t<Rs...>&&, concat_value_t<Rs...>&> &&
    std::common_reference_with<concat_reference_t<Rs...>&&, concat_rvalue_reference_t<Rs...>&&> &&
    std::common_reference_with<concat_rvalue_reference_t<Rs...>&&, const concat_value_t<Rs...>&> &&
    (concat_indirectly_readable_impl<concat_reference_t<Rs...>, concat_rvalue_reference_t<Rs...>,
                                     std::ranges::iterator_t<Rs>> &&
     ...);
template <class... Rs>
concept concatable = requires {
  typename concat_reference_t<Rs...>;
  typename concat_value_t<Rs...>;
  typename concat_rvalue_reference_t<Rs...>;
} && concat_indirectly_readable<Rs...>;

// The pack without its last element, as a check over each of them.
template <bool Const, class... Rs>
consteval bool all_but_last_common() {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return (std::ranges::common_range<maybe_const<Const, Rs...[I]>> && ...);
  }(std::make_index_sequence<sizeof...(Rs) - 1>{});
}
template <bool Const, class... Rs>
concept concat_is_random_access = all_random_access<Const, Rs...> && all_but_last_common<Const, Rs...>();
template <bool Const, class... Rs>
concept concat_is_bidirectional = all_bidirectional<Const, Rs...> && all_but_last_common<Const, Rs...>();

template <bool Const, class... Rs>
consteval bool all_but_first_sized() {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return (std::ranges::sized_range<maybe_const<Const, Rs...[I + 1]>> && ...);
  }(std::make_index_sequence<sizeof...(Rs) - 1>{});
}
template <bool Const, class... Rs>
concept concat_sized_sentinel = (std::sized_sentinel_for<std::ranges::sentinel_t<maybe_const<Const, Rs>>,
                                                         std::ranges::iterator_t<maybe_const<Const, Rs>>> &&
                                 ...) &&
                                all_but_first_sized<Const, Rs...>();

// ---- [range.lazy.split.view] ----------------------------------------------------------------------
template <auto>
struct require_constant;
template <class R>
concept tiny_range = std::ranges::sized_range<R> &&
                     requires { typename require_constant<std::remove_reference_t<R>::size()>; } &&
                     (std::remove_reference_t<R>::size() <= 1);

} // namespace ycxx::detail

namespace std::ranges {

// =============================================================================================
// [range.join]
// =============================================================================================
template <input_range V>
  requires view<V> && input_range<range_reference_t<V>>
class join_view : public view_interface<join_view<V>> {
  using InnerRng = range_reference_t<V>;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!(is_reference_v<range_reference_t<Base>> && forward_range<Base> &&
                    forward_range<range_reference_t<Base>>)) {
      return type_identity<void>{};
    } else {
      using OUTERC = ycxx::detail::iter_category_t<iterator_t<Base>>;
      using INNERC = ycxx::detail::iter_category_t<iterator_t<range_reference_t<Base>>>;
      if constexpr (derived_from<OUTERC, bidirectional_iterator_tag> && derived_from<INNERC, bidirectional_iterator_tag> &&
                    common_range<range_reference_t<Base>>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<OUTERC, forward_iterator_tag> && derived_from<INNERC, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend join_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, join_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;
    using OuterIter = iterator_t<Base>;
    using InnerIter = iterator_t<range_reference_t<Base>>;
    static constexpr bool ref_is_glvalue = is_reference_v<range_reference_t<Base>>;

    [[no_unique_address]] conditional_t<forward_range<Base>, OuterIter, ycxx::detail::empty_cache> current_ =
        conditional_t<forward_range<Base>, OuterIter, ycxx::detail::empty_cache>();
    optional<InnerIter> inner_;
    Parent* parent_ = nullptr;

    constexpr OuterIter& outer() {
      if constexpr (forward_range<Base>)
        return current_;
      else
        return *parent_->outer_;
    }
    constexpr const OuterIter& outer() const {
      if constexpr (forward_range<Base>)
        return current_;
      else
        return *parent_->outer_;
    }
    constexpr void satisfy() {
      auto update_inner = [this](const iterator_t<Base>& x) -> auto&& {
        if constexpr (ref_is_glvalue)
          return *x;
        else
          return parent_->inner_.emplace_deref(x);
      };
      for (; outer() != ranges::end(parent_->base_); ++outer()) {
        auto&& inner = update_inner(outer());
        inner_ = ranges::begin(inner);
        if (*inner_ != ranges::end(inner))
          return;
      }
      if constexpr (ref_is_glvalue)
        inner_.reset();
    }
    constexpr iterator(Parent& parent, OuterIter outer)
      requires forward_range<Base>
        : current_(std::move(outer)), parent_(__builtin_addressof(parent)) {
      satisfy();
    }
    constexpr explicit iterator(Parent& parent)
      requires(!forward_range<Base>)
        : parent_(__builtin_addressof(parent)) {
      satisfy();
    }

  public:
    using iterator_concept = conditional_t<
        ref_is_glvalue && bidirectional_range<Base> && ycxx::detail::bidirectional_common<range_reference_t<Base>>,
        bidirectional_iterator_tag,
        conditional_t<ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>,
                      forward_iterator_tag, input_iterator_tag>>;
    using value_type = range_value_t<range_reference_t<Base>>;
    using difference_type = common_type_t<range_difference_t<Base>, range_difference_t<range_reference_t<Base>>>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, OuterIter> && convertible_to<iterator_t<InnerRng>, InnerIter>
        : current_(std::move(i.current_)), inner_(std::move(i.inner_)), parent_(i.parent_) {}

    constexpr decltype(auto) operator*() const { return **inner_; }
    constexpr InnerIter operator->() const
      requires ycxx::detail::has_arrow<InnerIter> && copyable<InnerIter>
    {
      return *inner_;
    }

    constexpr iterator& operator++() {
      auto&& inner_range = [&]() -> auto&& {
        if constexpr (ref_is_glvalue)
          return *outer();
        else
          return *parent_->inner_;
      }();
      if (++*inner_ == ranges::end(ycxx::detail::as_lvalue(inner_range))) {
        ++outer();
        satisfy();
      }
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires ref_is_glvalue && bidirectional_range<Base> && bidirectional_range<range_reference_t<Base>> &&
               common_range<range_reference_t<Base>>
    {
      if (current_ == ranges::end(parent_->base_))
        inner_ = ranges::end(ycxx::detail::as_lvalue(*--current_));
      while (*inner_ == ranges::begin(ycxx::detail::as_lvalue(*current_)))
        *inner_ = ranges::end(ycxx::detail::as_lvalue(*--current_));
      --*inner_;
      return *this;
    }
    constexpr iterator operator--(int)
      requires ref_is_glvalue && bidirectional_range<Base> && bidirectional_range<range_reference_t<Base>> &&
               common_range<range_reference_t<Base>>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires ref_is_glvalue && forward_range<Base> && equality_comparable<iterator_t<range_reference_t<Base>>>
    {
      return x.current_ == y.current_ && x.inner_ == y.inner_;
    }
    friend constexpr decltype(auto) iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(*i.inner_))) {
      return ranges::iter_move(*i.inner_);
    }
    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(
        noexcept(ranges::iter_swap(*x.inner_, *y.inner_)))
      requires indirectly_swappable<InnerIter>
    {
      ranges::iter_swap(*x.inner_, *y.inner_);
    }
  };

  template <bool Const>
  class sentinel {
    friend join_view;
    friend sentinel<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, join_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(Parent& parent) : end_(ranges::end(parent.base_)) {}

    template <bool OtherConst>
    static constexpr decltype(auto) outer_of(const iterator<OtherConst>& x) {
      if constexpr (forward_range<ycxx::detail::maybe_const<OtherConst, V>>)
        return (ycxx::detail::view_access::current(x));
      else
        return *ycxx::detail::view_access::parent(x)->outer_;
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> s)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    template <bool OtherConst>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return outer_of(x) == y.end_;
    }
  };

  V base_ = V();
  [[no_unique_address]] ycxx::detail::cache_if<!forward_range<V>, iterator_t<V>> outer_;
  [[no_unique_address]] ycxx::detail::cache_if<!is_reference_v<InnerRng>, remove_cv_t<InnerRng>> inner_;

public:
  join_view()
    requires default_initializable<V>
  = default;
  constexpr explicit join_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin() {
    if constexpr (forward_range<V>) {
      constexpr bool use_const = ycxx::detail::simple_view<V> && is_reference_v<InnerRng>;
      return iterator<use_const>{*this, ranges::begin(base_)};
    } else {
      outer_.emplace(ranges::begin(base_));
      return iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<const V> && is_reference_v<range_reference_t<const V>> &&
             input_range<range_reference_t<const V>>
  {
    return iterator<true>{*this, ranges::begin(base_)};
  }
  constexpr auto end() {
    if constexpr (forward_range<V> && is_reference_v<InnerRng> && forward_range<InnerRng> && common_range<V> &&
                  common_range<InnerRng>)
      return iterator<ycxx::detail::simple_view<V>>{*this, ranges::end(base_)};
    else
      return sentinel<ycxx::detail::simple_view<V>>{*this};
  }
  constexpr auto end() const
    requires forward_range<const V> && is_reference_v<range_reference_t<const V>> &&
             input_range<range_reference_t<const V>>
  {
    if constexpr (forward_range<range_reference_t<const V>> && common_range<const V> &&
                  common_range<range_reference_t<const V>>)
      return iterator<true>{*this, ranges::end(base_)};
    else
      return sentinel<true>{*this};
  }
};
template <class R>
explicit join_view(R&&) -> join_view<views::all_t<R>>;

// =============================================================================================
// [range.join.with]
// =============================================================================================
template <input_range V, forward_range Pattern>
  requires view<V> && input_range<range_reference_t<V>> && view<Pattern> &&
           ycxx::detail::concatable<range_reference_t<V>, Pattern>
class join_with_view : public view_interface<join_with_view<V, Pattern>> {
  using InnerRng = range_reference_t<V>;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    using InnerBase = range_reference_t<Base>;
    using PatternBase = ycxx::detail::maybe_const<Const, Pattern>;
    if constexpr (!(is_reference_v<InnerBase> && forward_range<Base> && forward_range<InnerBase>)) {
      return type_identity<void>{};
    } else {
      using OUTERC = ycxx::detail::iter_category_t<iterator_t<Base>>;
      using INNERC = ycxx::detail::iter_category_t<iterator_t<InnerBase>>;
      using PATTERNC = ycxx::detail::iter_category_t<iterator_t<PatternBase>>;
      if constexpr (!is_reference_v<common_reference_t<iter_reference_t<iterator_t<InnerBase>>,
                                                       iter_reference_t<iterator_t<PatternBase>>>>)
        return type_identity<input_iterator_tag>{};
      else if constexpr (derived_from<OUTERC, bidirectional_iterator_tag> &&
                         derived_from<INNERC, bidirectional_iterator_tag> &&
                         derived_from<PATTERNC, bidirectional_iterator_tag> && common_range<InnerBase> &&
                         common_range<PatternBase>)
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr (derived_from<OUTERC, forward_iterator_tag> && derived_from<INNERC, forward_iterator_tag> &&
                         derived_from<PATTERNC, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend join_with_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, join_with_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;
    using InnerBase = range_reference_t<Base>;
    using PatternBase = ycxx::detail::maybe_const<Const, Pattern>;
    using OuterIter = iterator_t<Base>;
    using InnerIter = iterator_t<InnerBase>;
    using PatternIter = iterator_t<PatternBase>;
    static constexpr bool ref_is_glvalue = is_reference_v<InnerBase>;

    Parent* parent_ = nullptr;
    [[no_unique_address]] conditional_t<forward_range<Base>, OuterIter, ycxx::detail::empty_cache> current_ =
        conditional_t<forward_range<Base>, OuterIter, ycxx::detail::empty_cache>();
    variant<PatternIter, InnerIter> inner_it_;

    constexpr OuterIter& outer() {
      if constexpr (forward_range<Base>)
        return current_;
      else
        return *parent_->outer_it_;
    }
    constexpr const OuterIter& outer() const {
      if constexpr (forward_range<Base>)
        return current_;
      else
        return *parent_->outer_it_;
    }
    constexpr auto& update_inner() {
      if constexpr (ref_is_glvalue)
        return ycxx::detail::as_lvalue(*outer());
      else
        return parent_->inner_.emplace_deref(outer());
    }
    constexpr auto& get_inner() {
      if constexpr (ref_is_glvalue)
        return ycxx::detail::as_lvalue(*outer());
      else
        return *parent_->inner_;
    }
    constexpr void satisfy() {
      while (true) {
        if (inner_it_.index() == 0) {
          if (std::get<0>(inner_it_) != ranges::end(parent_->pattern_))
            break;
          inner_it_.template emplace<1>(ranges::begin(update_inner()));
        } else {
          if (std::get<1>(inner_it_) != ranges::end(get_inner()))
            break;
          if (++outer() == ranges::end(parent_->base_)) {
            if constexpr (ref_is_glvalue)
              inner_it_.template emplace<0>();
            break;
          }
          inner_it_.template emplace<0>(ranges::begin(parent_->pattern_));
        }
      }
    }
    constexpr void start() {
      if (outer() != ranges::end(parent_->base_)) {
        inner_it_.template emplace<1>(ranges::begin(update_inner()));
        satisfy();
      }
    }
    constexpr iterator(Parent& parent, OuterIter outer)
      requires forward_range<Base>
        : parent_(__builtin_addressof(parent)), current_(std::move(outer)) {
      start();
    }
    constexpr explicit iterator(Parent& parent)
      requires(!forward_range<Base>)
        : parent_(__builtin_addressof(parent)) {
      start();
    }

  public:
    using iterator_concept = conditional_t<
        ref_is_glvalue && bidirectional_range<Base> && ycxx::detail::bidirectional_common<InnerBase> &&
            ycxx::detail::bidirectional_common<PatternBase>,
        bidirectional_iterator_tag,
        conditional_t<ref_is_glvalue && forward_range<Base> && forward_range<InnerBase>, forward_iterator_tag,
                      input_iterator_tag>>;
    using value_type = common_type_t<iter_value_t<InnerIter>, iter_value_t<PatternIter>>;
    using difference_type =
        common_type_t<iter_difference_t<OuterIter>, iter_difference_t<InnerIter>, iter_difference_t<PatternIter>>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, OuterIter> && convertible_to<iterator_t<InnerRng>, InnerIter> &&
               convertible_to<iterator_t<Pattern>, PatternIter>
        : parent_(i.parent_), current_(std::move(i.current_)) {
      if (i.inner_it_.index() == 0)
        inner_it_.template emplace<0>(std::get<0>(std::move(i.inner_it_)));
      else
        inner_it_.template emplace<1>(std::get<1>(std::move(i.inner_it_)));
    }

    constexpr decltype(auto) operator*() const {
      using reference = common_reference_t<iter_reference_t<InnerIter>, iter_reference_t<PatternIter>>;
      return std::visit([](auto& it) -> reference { return *it; }, inner_it_);
    }
    constexpr iterator& operator++() {
      std::visit([](auto& it) { ++it; }, inner_it_);
      satisfy();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires ref_is_glvalue && forward_iterator<OuterIter> && forward_iterator<InnerIter>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires ref_is_glvalue && bidirectional_range<Base> && ycxx::detail::bidirectional_common<InnerBase> &&
               ycxx::detail::bidirectional_common<PatternBase>
    {
      if (current_ == ranges::end(parent_->base_)) {
        auto&& inner = *--current_;
        inner_it_.template emplace<1>(ranges::end(inner));
      }
      while (true) {
        if (inner_it_.index() == 0) {
          auto& it = std::get<0>(inner_it_);
          if (it == ranges::begin(parent_->pattern_)) {
            auto&& inner = *--current_;
            inner_it_.template emplace<1>(ranges::end(inner));
          } else {
            break;
          }
        } else {
          auto& it = std::get<1>(inner_it_);
          auto&& inner = *current_;
          if (it == ranges::begin(inner))
            inner_it_.template emplace<0>(ranges::end(parent_->pattern_));
          else
            break;
        }
      }
      std::visit([](auto& it) { --it; }, inner_it_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires ref_is_glvalue && bidirectional_range<Base> && ycxx::detail::bidirectional_common<InnerBase> &&
               ycxx::detail::bidirectional_common<PatternBase>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires ref_is_glvalue && forward_range<Base> && equality_comparable<InnerIter>
    {
      return x.current_ == y.current_ && x.inner_it_ == y.inner_it_;
    }
    friend constexpr decltype(auto) iter_move(const iterator& x) {
      using rvalue_reference = common_reference_t<iter_rvalue_reference_t<InnerIter>, iter_rvalue_reference_t<PatternIter>>;
      return std::visit<rvalue_reference>(ranges::iter_move, x.inner_it_);
    }
    friend constexpr void iter_swap(const iterator& x, const iterator& y)
      requires indirectly_swappable<InnerIter, PatternIter>
    {
      std::visit(ranges::iter_swap, x.inner_it_, y.inner_it_);
    }
  };

  template <bool Const>
  class sentinel {
    friend join_with_view;
    friend sentinel<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, join_with_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(Parent& parent) : end_(ranges::end(parent.base_)) {}

    template <bool OtherConst>
    static constexpr decltype(auto) outer_of(const iterator<OtherConst>& x) {
      if constexpr (forward_range<ycxx::detail::maybe_const<OtherConst, V>>)
        return (ycxx::detail::view_access::current(x));
      else
        return *ycxx::detail::view_access::parent(x)->outer_it_;
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> s)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    template <bool OtherConst>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return outer_of(x) == y.end_;
    }
  };

  V base_ = V();
  [[no_unique_address]] ycxx::detail::cache_if<!forward_range<V>, iterator_t<V>> outer_it_;
  [[no_unique_address]] ycxx::detail::cache_if<!is_reference_v<InnerRng>, remove_cv_t<InnerRng>> inner_;
  Pattern pattern_ = Pattern();

public:
  join_with_view()
    requires default_initializable<V> && default_initializable<Pattern>
  = default;
  constexpr explicit join_with_view(V base, Pattern pattern) : base_(std::move(base)), pattern_(std::move(pattern)) {}
  template <input_range R>
    requires constructible_from<V, views::all_t<R>> && constructible_from<Pattern, single_view<range_value_t<InnerRng>>>
  constexpr explicit join_with_view(R&& r, range_value_t<InnerRng> e)
      : base_(views::all(static_cast<R&&>(r))), pattern_(views::single(std::move(e))) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin() {
    if constexpr (forward_range<V>) {
      constexpr bool use_const =
          ycxx::detail::simple_view<V> && is_reference_v<InnerRng> && ycxx::detail::simple_view<Pattern>;
      return iterator<use_const>{*this, ranges::begin(base_)};
    } else {
      outer_it_.emplace(ranges::begin(base_));
      return iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<const V> && forward_range<const Pattern> && is_reference_v<range_reference_t<const V>> &&
             input_range<range_reference_t<const V>> &&
             ycxx::detail::concatable<range_reference_t<const V>, const Pattern>
  {
    return iterator<true>{*this, ranges::begin(base_)};
  }
  constexpr auto end() {
    constexpr bool c = ycxx::detail::simple_view<V> && ycxx::detail::simple_view<Pattern>;
    if constexpr (forward_range<V> && is_reference_v<InnerRng> && forward_range<InnerRng> && common_range<V> &&
                  common_range<InnerRng>)
      return iterator<c>{*this, ranges::end(base_)};
    else
      return sentinel<c>{*this};
  }
  constexpr auto end() const
    requires forward_range<const V> && forward_range<const Pattern> && is_reference_v<range_reference_t<const V>> &&
             input_range<range_reference_t<const V>> &&
             ycxx::detail::concatable<range_reference_t<const V>, const Pattern>
  {
    using InnerConstRng = range_reference_t<const V>;
    if constexpr (forward_range<InnerConstRng> && common_range<const V> && common_range<InnerConstRng>)
      return iterator<true>{*this, ranges::end(base_)};
    else
      return sentinel<true>{*this};
  }
};
template <class R, class P>
join_with_view(R&&, P&&) -> join_with_view<views::all_t<R>, views::all_t<P>>;
template <input_range R>
join_with_view(R&&, range_value_t<range_reference_t<R>>)
    -> join_with_view<views::all_t<R>, single_view<range_value_t<range_reference_t<R>>>>;

// =============================================================================================
// [range.lazy.split]
// =============================================================================================
template <input_range V, forward_range Pattern>
  requires view<V> && view<Pattern> && indirectly_comparable<iterator_t<V>, iterator_t<Pattern>, ranges::equal_to> &&
           (forward_range<V> || ycxx::detail::tiny_range<Pattern>)
class lazy_split_view : public view_interface<lazy_split_view<V, Pattern>> {
  template <bool Const>
  class inner_iterator;

  template <bool Const>
  class outer_iterator
      : public ycxx::detail::category_base<conditional_t<forward_range<ycxx::detail::maybe_const<Const, V>>,
                                                         input_iterator_tag, void>> {
    friend lazy_split_view;
    friend outer_iterator<!Const>;
    friend inner_iterator<Const>;
    using Parent = ycxx::detail::maybe_const<Const, lazy_split_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    Parent* parent_ = nullptr;
    [[no_unique_address]] conditional_t<forward_range<V>, iterator_t<Base>, ycxx::detail::empty_cache> current_ =
        conditional_t<forward_range<V>, iterator_t<Base>, ycxx::detail::empty_cache>();
    bool trailing_empty_ = false;

    // The notional member "current" ([range.lazy.split.outer]/1).
    constexpr iterator_t<Base>& current() const noexcept {
      if constexpr (forward_range<V>)
        return const_cast<iterator_t<Base>&>(current_);
      else
        return *parent_->current_;
    }

    constexpr explicit outer_iterator(Parent& parent)
      requires(!forward_range<Base>)
        : parent_(__builtin_addressof(parent)) {}
    constexpr outer_iterator(Parent& parent, iterator_t<Base> current)
      requires forward_range<Base>
        : parent_(__builtin_addressof(parent)), current_(std::move(current)) {}

  public:
    using iterator_concept = conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>;
    struct value_type : view_interface<value_type> {
    private:
      friend outer_iterator;
      outer_iterator i_ = outer_iterator();
      constexpr explicit value_type(outer_iterator i) : i_(std::move(i)) {}

    public:
      constexpr inner_iterator<Const> begin() const { return inner_iterator<Const>{i_}; }
      constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
    };
    using difference_type = range_difference_t<Base>;

    outer_iterator() = default;
    constexpr outer_iterator(outer_iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : parent_(i.parent_), current_(std::move(i.current_)), trailing_empty_(i.trailing_empty_) {}

    constexpr value_type operator*() const { return value_type{*this}; }
    constexpr outer_iterator& operator++() {
      const auto end = ranges::end(parent_->base_);
      auto& cur = current();
      if (cur == end) {
        trailing_empty_ = false;
        return *this;
      }
      const auto [pbegin, pend] = subrange{parent_->pattern_};
      if (pbegin == pend) {
        ++cur;
      } else if constexpr (ycxx::detail::tiny_range<Pattern>) {
        cur = ranges::find(std::move(cur), end, *pbegin);
        if (cur != end) {
          ++cur;
          if (cur == end)
            trailing_empty_ = true;
          else if constexpr (!forward_range<V>)
            trailing_empty_ = true;
        }
      } else {
        do {
          auto [b, p] = ranges::mismatch(cur, end, pbegin, pend);
          if (p == pend) {
            cur = b;
            if (cur == end)
              trailing_empty_ = true;
            break;
          }
        } while (++cur != end);
      }
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr outer_iterator operator++(int)
      requires forward_range<Base>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool operator==(const outer_iterator& x, const outer_iterator& y)
      requires forward_range<Base>
    {
      return x.current_ == y.current_ && x.trailing_empty_ == y.trailing_empty_;
    }
    friend constexpr bool operator==(const outer_iterator& x, default_sentinel_t) {
      return x.current() == ranges::end(x.parent_->base_) && !x.trailing_empty_;
    }
  };

  template <bool Const>
  static consteval auto inner_category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (derived_from<C, forward_iterator_tag>)
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<C>{};
    }
  }

  template <bool Const>
  class inner_iterator : public ycxx::detail::category_base<typename decltype(inner_category<Const>())::type> {
    friend outer_iterator<Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    outer_iterator<Const> i_ = outer_iterator<Const>();
    bool incremented_ = false;
    constexpr explicit inner_iterator(outer_iterator<Const> i) : i_(std::move(i)) {}

  public:
    using iterator_concept = typename outer_iterator<Const>::iterator_concept;
    using value_type = range_value_t<Base>;
    using difference_type = range_difference_t<Base>;

    inner_iterator() = default;
    constexpr const iterator_t<Base>& base() const& noexcept { return i_.current(); }
    constexpr iterator_t<Base> base() &&
      requires forward_range<V>
    {
      return std::move(i_.current());
    }
    constexpr decltype(auto) operator*() const { return *i_.current(); }
    constexpr inner_iterator& operator++() {
      incremented_ = true;
      if constexpr (!forward_range<Base>) {
        if constexpr (Pattern::size() == 0)
          return *this;
      }
      ++i_.current();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr inner_iterator operator++(int)
      requires forward_range<Base>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool operator==(const inner_iterator& x, const inner_iterator& y)
      requires forward_range<Base>
    {
      return x.i_.current() == y.i_.current();
    }
    friend constexpr bool operator==(const inner_iterator& x, default_sentinel_t) {
      auto [pcur, pend] = subrange{x.i_.parent_->pattern_};
      auto end = ranges::end(x.i_.parent_->base_);
      if constexpr (ycxx::detail::tiny_range<Pattern>) {
        const auto& cur = x.i_.current();
        if (cur == end)
          return true;
        if (pcur == pend)
          return x.incremented_;
        return bool(*cur == *pcur);
      } else {
        auto cur = x.i_.current();
        if (cur == end)
          return true;
        if (pcur == pend)
          return x.incremented_;
        do {
          if (!bool(*cur == *pcur))
            return false;
          if (++pcur == pend)
            return true;
        } while (++cur != end);
        return false;
      }
    }
    friend constexpr decltype(auto) iter_move(const inner_iterator& i) noexcept(
        noexcept(ranges::iter_move(i.i_.current()))) {
      return ranges::iter_move(i.i_.current());
    }
    friend constexpr void iter_swap(const inner_iterator& x, const inner_iterator& y) noexcept(
        noexcept(ranges::iter_swap(x.i_.current(), y.i_.current())))
      requires indirectly_swappable<iterator_t<Base>>
    {
      ranges::iter_swap(x.i_.current(), y.i_.current());
    }
  };

  V base_ = V();
  Pattern pattern_ = Pattern();
  [[no_unique_address]] ycxx::detail::cache_if<!forward_range<V>, iterator_t<V>> current_;

public:
  lazy_split_view()
    requires default_initializable<V> && default_initializable<Pattern>
  = default;
  constexpr explicit lazy_split_view(V base, Pattern pattern) : base_(std::move(base)), pattern_(std::move(pattern)) {}
  template <input_range R>
    requires constructible_from<V, views::all_t<R>> && constructible_from<Pattern, single_view<range_value_t<R>>>
  constexpr explicit lazy_split_view(R&& r, range_value_t<R> e)
      : base_(views::all(static_cast<R&&>(r))), pattern_(views::single(std::move(e))) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin() {
    if constexpr (forward_range<V>) {
      return outer_iterator<ycxx::detail::simple_view<V> && ycxx::detail::simple_view<Pattern>>{*this,
                                                                                                ranges::begin(base_)};
    } else {
      current_.emplace(ranges::begin(base_));
      return outer_iterator<false>{*this};
    }
  }
  constexpr auto begin() const
    requires forward_range<V> && forward_range<const V> && forward_range<const Pattern>
  {
    return outer_iterator<true>{*this, ranges::begin(base_)};
  }
  constexpr auto end()
    requires forward_range<V> && common_range<V>
  {
    return outer_iterator<ycxx::detail::simple_view<V> && ycxx::detail::simple_view<Pattern>>{*this, ranges::end(base_)};
  }
  constexpr auto end() const {
    if constexpr (forward_range<V> && forward_range<const V> && common_range<const V> && forward_range<const Pattern>)
      return outer_iterator<true>{*this, ranges::end(base_)};
    else
      return default_sentinel;
  }
};
template <class R, class P>
lazy_split_view(R&&, P&&) -> lazy_split_view<views::all_t<R>, views::all_t<P>>;
template <input_range R>
lazy_split_view(R&&, range_value_t<R>) -> lazy_split_view<views::all_t<R>, single_view<range_value_t<R>>>;

// =============================================================================================
// [range.split]
// =============================================================================================
template <forward_range V, forward_range Pattern>
  requires view<V> && view<Pattern> && indirectly_comparable<iterator_t<V>, iterator_t<Pattern>, ranges::equal_to>
class split_view : public view_interface<split_view<V, Pattern>> {
  class sentinel;
  class iterator {
    friend split_view;
    friend sentinel;
    friend ycxx::detail::view_access;
    split_view* parent_ = nullptr;
    iterator_t<V> current_ = iterator_t<V>();
    subrange<iterator_t<V>> next_ = subrange<iterator_t<V>>();
    bool trailing_empty_ = false;

    constexpr iterator(split_view& parent, iterator_t<V> current, subrange<iterator_t<V>> next)
        : parent_(__builtin_addressof(parent)), current_(std::move(current)), next_(std::move(next)) {}

  public:
    using iterator_concept = forward_iterator_tag;
    using iterator_category = input_iterator_tag;
    using value_type = subrange<iterator_t<V>>;
    using difference_type = range_difference_t<V>;

    iterator() = default;
    constexpr iterator_t<V> base() const { return current_; }
    constexpr value_type operator*() const { return {current_, next_.begin()}; }
    constexpr iterator& operator++() {
      current_ = next_.begin();
      if (current_ != ranges::end(parent_->base_)) {
        current_ = next_.end();
        if (current_ == ranges::end(parent_->base_)) {
          trailing_empty_ = true;
          next_ = {current_, current_};
        } else {
          next_ = parent_->find_next(current_);
        }
      } else {
        trailing_empty_ = false;
      }
      return *this;
    }
    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    friend constexpr bool operator==(const iterator& x, const iterator& y) {
      return x.current_ == y.current_ && x.trailing_empty_ == y.trailing_empty_;
    }
  };

  class sentinel {
    friend split_view;
    sentinel_t<V> end_ = sentinel_t<V>();
    constexpr explicit sentinel(split_view& parent) : end_(ranges::end(parent.base_)) {}

  public:
    sentinel() = default;
    friend constexpr bool operator==(const iterator& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.end_ && !x.trailing_empty_;
    }
  };

  V base_ = V();
  Pattern pattern_ = Pattern();
  ycxx::detail::non_propagating_cache<subrange<iterator_t<V>>> next_;

  constexpr subrange<iterator_t<V>> find_next(iterator_t<V> it) {
    auto [b, e] = ranges::search(subrange(it, ranges::end(base_)), pattern_);
    if (b != ranges::end(base_) && ranges::empty(pattern_)) {
      ++b;
      ++e;
    }
    return {b, e};
  }

public:
  split_view()
    requires default_initializable<V> && default_initializable<Pattern>
  = default;
  constexpr explicit split_view(V base, Pattern pattern) : base_(std::move(base)), pattern_(std::move(pattern)) {}
  template <forward_range R>
    requires constructible_from<V, views::all_t<R>> && constructible_from<Pattern, single_view<range_value_t<R>>>
  constexpr explicit split_view(R&& r, range_value_t<R> e)
      : base_(views::all(static_cast<R&&>(r))), pattern_(views::single(std::move(e))) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr iterator begin() {
    if (!next_.has_value())
      next_.emplace(find_next(ranges::begin(base_)));
    return {*this, ranges::begin(base_), *next_};
  }
  constexpr auto end() {
    if constexpr (common_range<V>)
      return iterator{*this, ranges::end(base_), {}};
    else
      return sentinel{*this};
  }
};
template <class R, class P>
split_view(R&&, P&&) -> split_view<views::all_t<R>, views::all_t<P>>;
template <forward_range R>
split_view(R&&, range_value_t<R>) -> split_view<views::all_t<R>, single_view<range_value_t<R>>>;

// =============================================================================================
// [range.concat]
// =============================================================================================
template <input_range... Views>
  requires(view<Views> && ...) && (sizeof...(Views) > 0) && ycxx::detail::concatable<Views...>
class concat_view : public view_interface<concat_view<Views...>> {
  static constexpr size_t N = sizeof...(Views);

  template <bool Const>
  static consteval auto category() {
    using namespace ycxx::detail;
    if constexpr (!all_forward<Const, Views...>) {
      return type_identity<void>{};
    } else if constexpr (!is_reference_v<concat_reference_t<maybe_const<Const, Views>...>>) {
      return type_identity<input_iterator_tag>{};
    } else if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>, random_access_iterator_tag> &&
                          ...) &&
                         concat_is_random_access<Const, Views...>) {
      return type_identity<random_access_iterator_tag>{};
    } else if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>,
                                       bidirectional_iterator_tag> &&
                          ...) &&
                         concat_is_bidirectional<Const, Views...>) {
      return type_identity<bidirectional_iterator_tag>{};
    } else if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>, forward_iterator_tag> &&
                          ...)) {
      return type_identity<forward_iterator_tag>{};
    } else {
      return type_identity<input_iterator_tag>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend concat_view;
    friend iterator<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, concat_view>;

  public:
    using iterator_concept =
        conditional_t<ycxx::detail::concat_is_random_access<Const, Views...>, random_access_iterator_tag,
                      conditional_t<ycxx::detail::concat_is_bidirectional<Const, Views...>, bidirectional_iterator_tag,
                                    conditional_t<ycxx::detail::all_forward<Const, Views...>, forward_iterator_tag,
                                                  input_iterator_tag>>>;
    using value_type = ycxx::detail::concat_value_t<ycxx::detail::maybe_const<Const, Views>...>;
    using difference_type = common_type_t<range_difference_t<ycxx::detail::maybe_const<Const, Views>>...>;

  private:
    using base_iter = variant<iterator_t<ycxx::detail::maybe_const<Const, Views>>...>;
    using reference = ycxx::detail::concat_reference_t<ycxx::detail::maybe_const<Const, Views>...>;
    using rvalue_reference = ycxx::detail::concat_rvalue_reference_t<ycxx::detail::maybe_const<Const, Views>...>;

    Parent* parent_ = nullptr;
    base_iter it_;

    template <size_t I>
    constexpr void satisfy() {
      if constexpr (I < N - 1) {
        if (std::get<I>(it_) == ranges::end(std::get<I>(parent_->views_))) {
          it_.template emplace<I + 1>(ranges::begin(std::get<I + 1>(parent_->views_)));
          satisfy<I + 1>();
        }
      }
    }
    template <size_t I>
    constexpr void prev() {
      if constexpr (I == 0) {
        --std::get<0>(it_);
      } else {
        if (std::get<I>(it_) == ranges::begin(std::get<I>(parent_->views_))) {
          it_.template emplace<I - 1>(ranges::end(std::get<I - 1>(parent_->views_)));
          prev<I - 1>();
        } else {
          --std::get<I>(it_);
        }
      }
    }
    template <size_t I>
    constexpr void advance_fwd(difference_type offset, difference_type steps) {
      using underlying_diff_type = iter_difference_t<variant_alternative_t<I, base_iter>>;
      if constexpr (I == N - 1) {
        std::get<I>(it_) += static_cast<underlying_diff_type>(steps);
      } else {
        difference_type n_size = ranges::distance(std::get<I>(parent_->views_));
        if (offset + steps < n_size) {
          std::get<I>(it_) += static_cast<underlying_diff_type>(steps);
        } else {
          it_.template emplace<I + 1>(ranges::begin(std::get<I + 1>(parent_->views_)));
          advance_fwd<I + 1>(0, offset + steps - n_size);
        }
      }
    }
    template <size_t I>
    constexpr void advance_bwd(difference_type offset, difference_type steps) {
      using underlying_diff_type = iter_difference_t<variant_alternative_t<I, base_iter>>;
      if constexpr (I == 0) {
        std::get<I>(it_) -= static_cast<underlying_diff_type>(steps);
      } else {
        if (offset >= steps) {
          std::get<I>(it_) -= static_cast<underlying_diff_type>(steps);
        } else {
          difference_type prev_size = ranges::distance(std::get<I - 1>(parent_->views_));
          it_.template emplace<I - 1>(ranges::end(std::get<I - 1>(parent_->views_)));
          advance_bwd<I - 1>(prev_size, steps - offset);
        }
      }
    }
    // The sum of the sizes of the underlying ranges with indices in [from, to).
    constexpr difference_type size_between(size_t from, size_t to) const {
      difference_type s = 0;
      [&]<size_t... I>(index_sequence<I...>) {
        ((I >= from && I < to ? (void)(s += static_cast<difference_type>(ranges::distance(std::get<I>(parent_->views_))))
                              : (void)0),
         ...);
      }(make_index_sequence<N>{});
      return s;
    }

    template <class... Args>
    constexpr explicit iterator(Parent* parent, Args&&... args)
      requires constructible_from<base_iter, Args&&...>
        : parent_(parent), it_(static_cast<Args&&>(args)...) {}

  public:
    iterator() = default;
    constexpr iterator(iterator<!Const> it)
      requires Const && (convertible_to<iterator_t<Views>, iterator_t<const Views>> && ...)
        : parent_(it.parent_) {
      ycxx::detail::with_index<N>(it.it_.index(), [&](auto i) {
        it_.template emplace<i>(std::get<i>(std::move(it.it_)));
      });
    }

    constexpr decltype(auto) operator*() const {
      return std::visit([](auto&& it) -> reference { return *it; }, it_);
    }
    constexpr iterator& operator++() {
      ycxx::detail::with_index<N>(it_.index(), [&](auto i) {
        ++std::get<i>(it_);
        satisfy<i>();
      });
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires ycxx::detail::all_forward<Const, Views...>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires ycxx::detail::concat_is_bidirectional<Const, Views...>
    {
      ycxx::detail::with_index<N>(it_.index(), [&](auto i) { prev<i>(); });
      return *this;
    }
    constexpr iterator operator--(int)
      requires ycxx::detail::concat_is_bidirectional<Const, Views...>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type n)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      ycxx::detail::with_index<N>(it_.index(), [&](auto i) {
        difference_type offset = std::get<i>(it_) - ranges::begin(std::get<i>(parent_->views_));
        if (n > 0)
          advance_fwd<i>(offset, n);
        else if (n < 0)
          advance_bwd<i>(offset, -n);
      });
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      *this += -n;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      return *((*this) + n);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires(equality_comparable<iterator_t<ycxx::detail::maybe_const<Const, Views>>> && ...)
    {
      return x.it_ == y.it_;
    }
    friend constexpr bool operator==(const iterator& it, default_sentinel_t) {
      constexpr auto last_idx = N - 1;
      return it.it_.index() == last_idx &&
             std::get<last_idx>(it.it_) == ranges::end(std::get<last_idx>(it.parent_->views_));
    }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return x.it_ < y.it_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return x.it_ > y.it_;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return x.it_ <= y.it_;
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return x.it_ >= y.it_;
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires(ycxx::detail::all_random_access<Const, Views...> &&
               (three_way_comparable<iterator_t<ycxx::detail::maybe_const<Const, Views>>> && ...))
    {
      return x.it_ <=> y.it_;
    }
    friend constexpr iterator operator+(const iterator& it, difference_type n)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      auto temp = it;
      temp += n;
      return temp;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& it)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      return it + n;
    }
    friend constexpr iterator operator-(const iterator& it, difference_type n)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      auto temp = it;
      temp -= n;
      return temp;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires ycxx::detail::concat_is_random_access<Const, Views...>
    {
      size_t ix = x.it_.index(), iy = y.it_.index();
      if (ix < iy)
        return -(y - x);
      difference_type result = 0;
      ycxx::detail::with_index<N>(ix, [&](auto i) {
        if (ix == iy) {
          result = std::get<i>(x.it_) - std::get<i>(y.it_);
        } else {
          difference_type dx = ranges::distance(ranges::begin(std::get<i>(x.parent_->views_)), std::get<i>(x.it_));
          ycxx::detail::with_index<N>(iy, [&](auto j) {
            difference_type dy = ranges::distance(std::get<j>(y.it_), ranges::end(std::get<j>(y.parent_->views_)));
            result = dy + x.size_between(iy + 1, ix) + dx;
          });
        }
      });
      return result;
    }
    friend constexpr difference_type operator-(const iterator& x, default_sentinel_t)
      requires ycxx::detail::concat_sized_sentinel<Const, Views...>
    {
      difference_type result = 0;
      ycxx::detail::with_index<N>(x.it_.index(), [&](auto i) {
        difference_type dx = ranges::distance(std::get<i>(x.it_), ranges::end(std::get<i>(x.parent_->views_)));
        difference_type s = 0;
        [&]<size_t... K>(index_sequence<K...>) {
          ((K > i ? (void)(s += static_cast<difference_type>(ranges::size(std::get<K>(x.parent_->views_)))) : (void)0),
           ...);
        }(make_index_sequence<N>{});
        result = -(dx + s);
      });
      return result;
    }
    friend constexpr difference_type operator-(default_sentinel_t, const iterator& x)
      requires ycxx::detail::concat_sized_sentinel<Const, Views...>
    {
      return -(x - default_sentinel);
    }
    friend constexpr decltype(auto) iter_move(const iterator& it) noexcept(
        ((is_nothrow_invocable_v<decltype(ranges::iter_move), const iterator_t<ycxx::detail::maybe_const<Const, Views>>&> &&
          is_nothrow_convertible_v<range_rvalue_reference_t<ycxx::detail::maybe_const<Const, Views>>, rvalue_reference>) &&
         ...)) {
      return std::visit([](const auto& i) -> rvalue_reference { return ranges::iter_move(i); }, it.it_);
    }
    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(
        noexcept(ranges::swap(*x, *y)) &&
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>(),
                                    std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>())) &&
         ...))
      requires swappable_with<reference, reference> &&
               (... && indirectly_swappable<iterator_t<ycxx::detail::maybe_const<Const, Views>>>)
    {
      std::visit(
          [&](const auto& it1, const auto& it2) {
            if constexpr (is_same_v<decltype(it1), decltype(it2)>)
              ranges::iter_swap(it1, it2);
            else
              ranges::swap(*x, *y);
          },
          x.it_, y.it_);
    }
  };

  tuple<Views...> views_;

  template <bool Const, class Self>
  static constexpr auto end_impl(Self* self) {
    if constexpr (ycxx::detail::all_forward<Const, Views...> &&
                  common_range<ycxx::detail::maybe_const<Const, Views...[N - 1]>>)
      return iterator<Const>(self, in_place_index<N - 1>, ranges::end(std::get<N - 1>(self->views_)));
    else
      return default_sentinel;
  }

public:
  constexpr concat_view() = default;
  constexpr explicit concat_view(Views... views) : views_(std::move(views)...) {}

  constexpr iterator<false> begin()
    requires(!(ycxx::detail::simple_view<Views> && ...))
  {
    iterator<false> it(this, in_place_index<0>, ranges::begin(std::get<0>(views_)));
    it.template satisfy<0>();
    return it;
  }
  constexpr iterator<true> begin() const
    requires(range<const Views> && ...) && ycxx::detail::concatable<const Views...>
  {
    iterator<true> it(this, in_place_index<0>, ranges::begin(std::get<0>(views_)));
    it.template satisfy<0>();
    return it;
  }
  constexpr auto end()
    requires(!(ycxx::detail::simple_view<Views> && ...))
  {
    return end_impl<false>(this);
  }
  constexpr auto end() const
    requires(range<const Views> && ...) && ycxx::detail::concatable<const Views...>
  {
    return end_impl<true>(this);
  }
  constexpr auto size()
    requires(sized_range<Views> && ...)
  {
    return std::apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT(sizes) + ...);
        },
        ycxx::detail::tuple_transform(ranges::size, views_));
  }
  constexpr auto size() const
    requires(sized_range<const Views> && ...)
  {
    return std::apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT(sizes) + ...);
        },
        ycxx::detail::tuple_transform(ranges::size, views_));
  }
  constexpr auto reserve_hint()
    requires(approximately_sized_range<Views> && ...)
  {
    return std::apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT(sizes) + ...);
        },
        ycxx::detail::tuple_transform(ranges::reserve_hint, views_));
  }
  constexpr auto reserve_hint() const
    requires(approximately_sized_range<const Views> && ...)
  {
    return std::apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT(sizes) + ...);
        },
        ycxx::detail::tuple_transform(ranges::reserve_hint, views_));
  }
};
template <class... R>
concat_view(R&&...) -> concat_view<views::all_t<R>...>;

} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace ycxx::detail::view_fn {

struct join_fn : std::ranges::range_adaptor_closure<join_fn> {
  template <class E>
    requires requires { std::ranges::join_view<std::views::all_t<E>>{std::declval<E>()}; }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    return std::ranges::join_view<std::views::all_t<E>>{static_cast<E&&>(e)};
  }
};

// The views taking a range and a pattern: views::X(E, F) is X_view(E, F).
template <template <class, class> class View>
struct pattern_fn {
  template <class E, class F>
    requires requires { View(std::declval<E>(), std::declval<F>()); }
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    return View(static_cast<E&&>(e), static_cast<F&&>(f));
  }
  template <class F>
  [[nodiscard]] constexpr auto operator()(F&& f) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f));
  }
};

struct concat_fn {
  template <class... Es>
    requires(sizeof...(Es) == 1 && (std::ranges::input_range<Es> && ...) &&
             requires { std::views::all(std::declval<Es>()...); }) ||
            requires { std::ranges::concat_view(std::declval<Es>()...); }
  [[nodiscard]] constexpr auto operator()(Es&&... es) const {
    if constexpr (sizeof...(Es) == 1 && (std::ranges::input_range<Es> && ...))
      return std::views::all(static_cast<Es&&>(es)...);
    else
      return std::ranges::concat_view(static_cast<Es&&>(es)...);
  }
};

} // namespace ycxx::detail::view_fn

namespace std::ranges::views {
inline constexpr ycxx::detail::view_fn::join_fn join{};
inline constexpr ycxx::detail::view_fn::pattern_fn<join_with_view> join_with{};
inline constexpr ycxx::detail::view_fn::pattern_fn<lazy_split_view> lazy_split{};
inline constexpr ycxx::detail::view_fn::pattern_fn<split_view> split{};
inline constexpr ycxx::detail::view_fn::concat_fn concat{};
} // namespace std::ranges::views
