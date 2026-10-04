// libycxx core: the range adaptors that produce tuples or combine several ranges: elements
// (keys, values), enumerate, zip, zip_transform, adjacent (pairwise), adjacent_transform
// (pairwise_transform) and cartesian_product.
#pragma once

#include <ycxx/core/ranges_adaptors.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/array.hpp>

namespace ycxx::detail {

// ---- [range.adaptor.helpers] -----------------------------------------------------------------
template <class F, class Tuple>
constexpr auto tuple_transform(F&& f, Tuple&& t) {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::tuple<std::invoke_result_t<F&, decltype(std::get<I>(static_cast<Tuple&&>(t)))>...>(
        ::ycxx::detail::invoke(f, std::get<I>(static_cast<Tuple&&>(t)))...);
  }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
}
template <class F, class Tuple>
constexpr void tuple_for_each(F&& f, Tuple&& t) {
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    (static_cast<void>(::ycxx::detail::invoke(f, std::get<I>(static_cast<Tuple&&>(t)))), ...);
  }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
}

template <bool Const, class... Views>
concept all_random_access = (std::ranges::random_access_range<maybe_const<Const, Views>> && ...);
template <bool Const, class... Views>
concept all_bidirectional = (std::ranges::bidirectional_range<maybe_const<Const, Views>> && ...);
template <bool Const, class... Views>
concept all_forward = (std::ranges::forward_range<maybe_const<Const, Views>> && ...);

template <bool Const, class... Views>
consteval auto all_strength() {
  if constexpr (all_random_access<Const, Views...>)
    return std::random_access_iterator_tag{};
  else if constexpr (all_bidirectional<Const, Views...>)
    return std::bidirectional_iterator_tag{};
  else if constexpr (all_forward<Const, Views...>)
    return std::forward_iterator_tag{};
  else
    return std::input_iterator_tag{};
}

// The value with the smallest absolute value.
template <class D>
constexpr D smallest_abs(std::initializer_list<D> ds) {
  auto best = *ds.begin();
  for (D d : ds)
    if ((d < 0 ? -d : d) < (best < 0 ? -best : best))
      best = d;
  return best;
}

// REPEAT(T, N) ([range.adjacent.overview]/3).
template <class T, std::size_t>
using repeat_type = T;
template <class F, class T, class Seq>
inline constexpr bool repeat_regular_invocable_v = false;
template <class F, class T, std::size_t... I>
inline constexpr bool repeat_regular_invocable_v<F, T, std::index_sequence<I...>> =
    std::regular_invocable<F, repeat_type<T, I>...>;
template <class F, class T, std::size_t N>
concept repeat_regular_invocable = repeat_regular_invocable_v<F, T, std::make_index_sequence<N>>;
template <class F, class T, class Seq>
struct repeat_invoke_result;
template <class F, class T, std::size_t... I>
struct repeat_invoke_result<F, T, std::index_sequence<I...>> {
  using type = std::invoke_result_t<F, repeat_type<T, I>...>;
};
template <class F, class T, std::size_t N>
using repeat_invoke_result_t = typename repeat_invoke_result<F, T, std::make_index_sequence<N>>::type;
template <class T, class Seq>
struct repeat_tuple;
template <class T, std::size_t... I>
struct repeat_tuple<T, std::index_sequence<I...>> {
  using type = std::tuple<repeat_type<T, I>...>;
};
template <class T, std::size_t N>
using repeat_tuple_t = typename repeat_tuple<T, std::make_index_sequence<N>>::type;

// ---- [range.elements.view] ---------------------------------------------------------------------
template <class T, std::size_t N>
concept has_tuple_element = tuple_like<T> && N < std::tuple_size_v<T> && requires(T t) {
  { std::get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T>&>;
};
template <class T, std::size_t N>
concept returnable_element = std::is_reference_v<T> || std::move_constructible<std::tuple_element_t<N, T>>;

} // namespace ycxx::detail

namespace std::ranges {

// =============================================================================================
// [range.elements]
// =============================================================================================
template <input_range V, size_t N>
  requires view<V> && ycxx::detail::has_tuple_element<range_value_t<V>, N> &&
           ycxx::detail::has_tuple_element<remove_reference_t<range_reference_t<V>>, N> &&
           ycxx::detail::returnable_element<range_reference_t<V>, N>
class elements_view : public view_interface<elements_view<V, N>> {
  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (!is_lvalue_reference_v<decltype(std::get<N>(*std::declval<iterator_t<Base>&>()))>)
        return type_identity<input_iterator_tag>{};
      else if constexpr (derived_from<C, random_access_iterator_tag>)
        return type_identity<random_access_iterator_tag>{};
      else
        return type_identity<C>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend elements_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Base = ycxx::detail::maybe_const<Const, V>;

    iterator_t<Base> current_ = iterator_t<Base>();

    static constexpr decltype(auto) get_element(const iterator_t<Base>& i) {
      if constexpr (is_reference_v<range_reference_t<Base>>) {
        return std::get<N>(*i);
      } else {
        using E = remove_cv_t<tuple_element_t<N, range_reference_t<Base>>>;
        return static_cast<E>(std::get<N>(*i));
      }
    }
    constexpr explicit iterator(iterator_t<Base> current) : current_(std::move(current)) {}

  public:
    using iterator_concept = ycxx::detail::range_strength_t<Base>;
    using value_type = remove_cvref_t<tuple_element_t<N, range_value_t<Base>>>;
    using difference_type = range_difference_t<Base>;

    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr decltype(auto) operator*() const { return get_element(current_); }

    constexpr iterator& operator++() {
      ++current_;
      return *this;
    }
    constexpr void operator++(int) { ++current_; }
    constexpr iterator operator++(int)
      requires forward_range<Base>
    {
      auto tmp = *this;
      ++current_;
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
      --current_;
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
      return get_element(current_ + n);
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
    friend constexpr iterator operator+(const iterator& x, difference_type y)
      requires random_access_range<Base>
    {
      return iterator{x} += y;
    }
    friend constexpr iterator operator+(difference_type x, const iterator& y)
      requires random_access_range<Base>
    {
      return y + x;
    }
    friend constexpr iterator operator-(const iterator& x, difference_type y)
      requires random_access_range<Base>
    {
      return iterator{x} -= y;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
      return x.current_ - y.current_;
    }
  };

  template <bool Const>
  class sentinel {
    friend elements_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(sentinel_t<Base> end) : end_(end) {}

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
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const iterator<OtherConst>& x,
                                                                                            const sentinel& y) {
      return ycxx::detail::view_access::current(x) - y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const sentinel& x,
                                                                                            const iterator<OtherConst>& y) {
      return x.end_ - ycxx::detail::view_access::current(y);
    }
  };

  V base_ = V();

public:
  elements_view()
    requires default_initializable<V>
  = default;
  constexpr explicit elements_view(V base) : base_(std::move(base)) {}

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
    requires(!ycxx::detail::simple_view<V> && !common_range<V>)
  {
    return sentinel<false>{ranges::end(base_)};
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V> && common_range<V>)
  {
    return iterator<false>{ranges::end(base_)};
  }
  constexpr auto end() const
    requires range<const V>
  {
    return sentinel<true>{ranges::end(base_)};
  }
  constexpr auto end() const
    requires common_range<const V>
  {
    return iterator<true>{ranges::end(base_)};
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
template <class T, size_t N>
constexpr bool enable_borrowed_range<elements_view<T, N>> = enable_borrowed_range<T>;
template <class R>
using keys_view = elements_view<R, 0>;
template <class R>
using values_view = elements_view<R, 1>;

// =============================================================================================
// [range.enumerate]
// =============================================================================================
template <view V>
  requires ycxx::detail::range_with_movable_references<V>
class enumerate_view : public view_interface<enumerate_view<V>> {
  template <bool Const>
  class iterator {
    friend enumerate_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Base = ycxx::detail::maybe_const<Const, V>;

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept = ycxx::detail::range_strength_t<Base>;
    using difference_type = range_difference_t<Base>;
    using value_type = tuple<difference_type, range_value_t<Base>>;

  private:
    using reference_type = tuple<difference_type, range_reference_t<Base>>;
    iterator_t<Base> current_ = iterator_t<Base>();
    difference_type pos_ = 0;

    constexpr explicit iterator(iterator_t<Base> current, difference_type pos)
        : current_(std::move(current)), pos_(pos) {}

  public:
    iterator()
      requires default_initializable<iterator_t<Base>>
    = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)), pos_(i.pos_) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr difference_type index() const noexcept { return pos_; }
    constexpr auto operator*() const { return reference_type(pos_, *current_); }

    constexpr iterator& operator++() {
      ++current_;
      ++pos_;
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
      --current_;
      --pos_;
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
      pos_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n)
      requires random_access_range<Base>
    {
      current_ -= n;
      pos_ -= n;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return reference_type(pos_ + n, current_[n]);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) noexcept { return x.pos_ == y.pos_; }
    friend constexpr strong_ordering operator<=>(const iterator& x, const iterator& y) noexcept {
      return x.pos_ <=> y.pos_;
    }
    friend constexpr iterator operator+(const iterator& x, difference_type y)
      requires random_access_range<Base>
    {
      auto temp = x;
      temp += y;
      return temp;
    }
    friend constexpr iterator operator+(difference_type x, const iterator& y)
      requires random_access_range<Base>
    {
      return y + x;
    }
    friend constexpr iterator operator-(const iterator& x, difference_type y)
      requires random_access_range<Base>
    {
      auto temp = x;
      temp -= y;
      return temp;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y) noexcept { return x.pos_ - y.pos_; }
    friend constexpr auto iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(i.current_)) &&
                                                                is_nothrow_move_constructible_v<range_rvalue_reference_t<Base>>) {
      return tuple<difference_type, range_rvalue_reference_t<Base>>(i.pos_, ranges::iter_move(i.current_));
    }
  };

  template <bool Const>
  class sentinel {
    friend enumerate_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(sentinel_t<Base> end) : end_(std::move(end)) {}

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
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const iterator<OtherConst>& x,
                                                                                            const sentinel& y) {
      return ycxx::detail::view_access::current(x) - y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const sentinel& x,
                                                                                            const iterator<OtherConst>& y) {
      return x.end_ - ycxx::detail::view_access::current(y);
    }
  };

  V base_ = V();

public:
  constexpr enumerate_view()
    requires default_initializable<V>
  = default;
  constexpr explicit enumerate_view(V base) : base_(std::move(base)) {}

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return iterator<false>(ranges::begin(base_), 0);
  }
  constexpr auto begin() const
    requires ycxx::detail::range_with_movable_references<const V>
  {
    return iterator<true>(ranges::begin(base_), 0);
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (forward_range<V> && common_range<V> && sized_range<V>)
      return iterator<false>(ranges::end(base_), ranges::distance(base_));
    else
      return sentinel<false>(ranges::end(base_));
  }
  constexpr auto end() const
    requires ycxx::detail::range_with_movable_references<const V>
  {
    if constexpr (forward_range<const V> && common_range<const V> && sized_range<const V>)
      return iterator<true>(ranges::end(base_), ranges::distance(base_));
    else
      return sentinel<true>(ranges::end(base_));
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
  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }
};
template <class R>
enumerate_view(R&&) -> enumerate_view<views::all_t<R>>;
template <class View>
constexpr bool enable_borrowed_range<enumerate_view<View>> = enable_borrowed_range<View>;

} // namespace std::ranges

namespace ycxx::detail {
template <class... Rs>
concept zip_is_common = (sizeof...(Rs) == 1 && (std::ranges::common_range<Rs> && ...)) ||
                        (!(std::ranges::bidirectional_range<Rs> && ...) && (std::ranges::common_range<Rs> && ...)) ||
                        ((std::ranges::random_access_range<Rs> && ...) && (std::ranges::sized_range<Rs> && ...));
} // namespace ycxx::detail

namespace std::ranges {

// =============================================================================================
// [range.zip]
// =============================================================================================
template <input_range... Views>
  requires(view<Views> && ...) && (sizeof...(Views) > 0)
class zip_view : public view_interface<zip_view<Views...>> {
  template <bool Const>
  class iterator : public ycxx::detail::category_base<
                       conditional_t<ycxx::detail::all_forward<Const, Views...>, input_iterator_tag, void>> {
    friend zip_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;

    tuple<iterator_t<ycxx::detail::maybe_const<Const, Views>>...> current_;
    constexpr explicit iterator(tuple<iterator_t<ycxx::detail::maybe_const<Const, Views>>...> current)
        : current_(std::move(current)) {}

  public:
    using iterator_concept = decltype(ycxx::detail::all_strength<Const, Views...>());
    using value_type = tuple<range_value_t<ycxx::detail::maybe_const<Const, Views>>...>;
    using difference_type = common_type_t<range_difference_t<ycxx::detail::maybe_const<Const, Views>>...>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && (convertible_to<iterator_t<Views>, iterator_t<const Views>> && ...)
        : current_(std::move(i.current_)) {}

    constexpr auto operator*() const {
      return ycxx::detail::tuple_transform([](auto& i) -> decltype(auto) { return *i; }, current_);
    }
    constexpr iterator& operator++() {
      ycxx::detail::tuple_for_each([](auto& i) { ++i; }, current_);
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
      requires ycxx::detail::all_bidirectional<Const, Views...>
    {
      ycxx::detail::tuple_for_each([](auto& i) { --i; }, current_);
      return *this;
    }
    constexpr iterator operator--(int)
      requires ycxx::detail::all_bidirectional<Const, Views...>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type x)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      ycxx::detail::tuple_for_each([&]<class I>(I& i) { i += iter_difference_t<I>(x); }, current_);
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      ycxx::detail::tuple_for_each([&]<class I>(I& i) { i -= iter_difference_t<I>(x); }, current_);
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return ycxx::detail::tuple_transform(
          [&]<class I>(I& i) -> decltype(auto) { return i[iter_difference_t<I>(n)]; }, current_);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires(equality_comparable<iterator_t<ycxx::detail::maybe_const<Const, Views>>> && ...)
    {
      if constexpr (ycxx::detail::all_bidirectional<Const, Views...>) {
        return x.current_ == y.current_;
      } else {
        return [&]<size_t... I>(index_sequence<I...>) {
          return (bool(std::get<I>(x.current_) == std::get<I>(y.current_)) || ...);
        }(index_sequence_for<Views...>{});
      }
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      auto r = i;
      r += n;
      return r;
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires ycxx::detail::all_random_access<Const, Views...>
    {
      auto r = i;
      r -= n;
      return r;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires(sized_sentinel_for<iterator_t<ycxx::detail::maybe_const<Const, Views>>,
                                  iterator_t<ycxx::detail::maybe_const<Const, Views>>> &&
               ...)
    {
      return [&]<size_t... I>(index_sequence<I...>) {
        return ycxx::detail::smallest_abs<difference_type>(
            {difference_type(std::get<I>(x.current_) - std::get<I>(y.current_))...});
      }(index_sequence_for<Views...>{});
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        (noexcept(ranges::iter_move(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>())) && ...) &&
        (is_nothrow_move_constructible_v<range_rvalue_reference_t<ycxx::detail::maybe_const<Const, Views>>> && ...)) {
      return ycxx::detail::tuple_transform(ranges::iter_move, i.current_);
    }
    friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>(),
                                    std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>())) &&
         ...))
      requires(indirectly_swappable<iterator_t<ycxx::detail::maybe_const<Const, Views>>> && ...)
    {
      [&]<size_t... I>(index_sequence<I...>) {
        (ranges::iter_swap(std::get<I>(l.current_), std::get<I>(r.current_)), ...);
      }(index_sequence_for<Views...>{});
    }
  };

  template <bool Const>
  class sentinel {
    friend zip_view;
    friend sentinel<!Const>;

    tuple<sentinel_t<ycxx::detail::maybe_const<Const, Views>>...> end_;
    constexpr explicit sentinel(tuple<sentinel_t<ycxx::detail::maybe_const<Const, Views>>...> end) : end_(end) {}

    template <bool OtherConst>
    static constexpr auto distance(const iterator<OtherConst>& x, const sentinel& y) {
      using D = common_type_t<range_difference_t<ycxx::detail::maybe_const<OtherConst, Views>>...>;
      return [&]<size_t... I>(index_sequence<I...>) {
        return ycxx::detail::smallest_abs<D>(
            {D(std::get<I>(ycxx::detail::view_access::current(x)) - std::get<I>(y.end_))...});
      }(index_sequence_for<Views...>{});
    }

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> i)
      requires Const && (convertible_to<sentinel_t<Views>, sentinel_t<const Views>> && ...)
        : end_(std::move(i.end_)) {}

    template <bool OtherConst>
      requires(sentinel_for<sentinel_t<ycxx::detail::maybe_const<Const, Views>>,
                            iterator_t<ycxx::detail::maybe_const<OtherConst, Views>>> &&
               ...)
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return [&]<size_t... I>(index_sequence<I...>) {
        return (bool(std::get<I>(ycxx::detail::view_access::current(x)) == std::get<I>(y.end_)) || ...);
      }(index_sequence_for<Views...>{});
    }
    template <bool OtherConst>
      requires(sized_sentinel_for<sentinel_t<ycxx::detail::maybe_const<Const, Views>>,
                                  iterator_t<ycxx::detail::maybe_const<OtherConst, Views>>> &&
               ...)
    friend constexpr common_type_t<range_difference_t<ycxx::detail::maybe_const<OtherConst, Views>>...> operator-(
        const iterator<OtherConst>& x, const sentinel& y) {
      return distance(x, y);
    }
    template <bool OtherConst>
      requires(sized_sentinel_for<sentinel_t<ycxx::detail::maybe_const<Const, Views>>,
                                  iterator_t<ycxx::detail::maybe_const<OtherConst, Views>>> &&
               ...)
    friend constexpr common_type_t<range_difference_t<ycxx::detail::maybe_const<OtherConst, Views>>...> operator-(
        const sentinel& y, const iterator<OtherConst>& x) {
      return -distance(x, y);
    }
  };

  tuple<Views...> views_;

  template <bool Const>
  static constexpr auto size_of(ycxx::detail::maybe_const<Const, tuple<Views...>>& views) {
    return std::apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return ranges::min({CT(sizes)...});
        },
        ycxx::detail::tuple_transform(ranges::size, views));
  }

public:
  zip_view() = default;
  constexpr explicit zip_view(Views... views) : views_(std::move(views)...) {}

  constexpr auto begin()
    requires(!(ycxx::detail::simple_view<Views> && ...))
  {
    return iterator<false>(ycxx::detail::tuple_transform(ranges::begin, views_));
  }
  constexpr auto begin() const
    requires(range<const Views> && ...)
  {
    return iterator<true>(ycxx::detail::tuple_transform(ranges::begin, views_));
  }
  constexpr auto end()
    requires(!(ycxx::detail::simple_view<Views> && ...))
  {
    if constexpr (!ycxx::detail::zip_is_common<Views...>)
      return sentinel<false>(ycxx::detail::tuple_transform(ranges::end, views_));
    else if constexpr ((random_access_range<Views> && ...))
      return begin() + iter_difference_t<iterator<false>>(size());
    else
      return iterator<false>(ycxx::detail::tuple_transform(ranges::end, views_));
  }
  constexpr auto end() const
    requires(range<const Views> && ...)
  {
    if constexpr (!ycxx::detail::zip_is_common<const Views...>)
      return sentinel<true>(ycxx::detail::tuple_transform(ranges::end, views_));
    else if constexpr ((random_access_range<const Views> && ...))
      return begin() + iter_difference_t<iterator<true>>(size());
    else
      return iterator<true>(ycxx::detail::tuple_transform(ranges::end, views_));
  }
  constexpr auto size()
    requires(sized_range<Views> && ...)
  {
    return size_of<false>(views_);
  }
  constexpr auto size() const
    requires(sized_range<const Views> && ...)
  {
    return size_of<true>(views_);
  }
};
template <class... Rs>
zip_view(Rs&&...) -> zip_view<views::all_t<Rs>...>;
template <class... Views>
constexpr bool enable_borrowed_range<zip_view<Views...>> = (enable_borrowed_range<Views> && ...);

// =============================================================================================
// [range.zip.transform]
// =============================================================================================
template <move_constructible F, input_range... Views>
  requires(view<Views> && ...) && (sizeof...(Views) > 0) && is_object_v<F> &&
          regular_invocable<F&, range_reference_t<Views>...> &&
          ycxx::detail::can_reference<invoke_result_t<F&, range_reference_t<Views>...>>
class zip_transform_view : public view_interface<zip_transform_view<F, Views...>> {
  using InnerView = zip_view<Views...>;
  template <bool Const>
  using ziperator = iterator_t<ycxx::detail::maybe_const<Const, InnerView>>;
  template <bool Const>
  using zentinel = sentinel_t<ycxx::detail::maybe_const<Const, InnerView>>;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, InnerView>;
    if constexpr (!forward_range<Base>) {
      return type_identity<void>{};
    } else if constexpr (!is_reference_v<invoke_result_t<ycxx::detail::maybe_const<Const, F>&,
                                                          range_reference_t<ycxx::detail::maybe_const<Const, Views>>...>>) {
      return type_identity<input_iterator_tag>{};
    } else {
      using namespace ycxx::detail;
      if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>, random_access_iterator_tag> &&
                     ...))
        return type_identity<random_access_iterator_tag>{};
      else if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>,
                                       bidirectional_iterator_tag> &&
                          ...))
        return type_identity<bidirectional_iterator_tag>{};
      else if constexpr ((derived_from<iter_category_t<iterator_t<maybe_const<Const, Views>>>, forward_iterator_tag> &&
                          ...))
        return type_identity<forward_iterator_tag>{};
      else
        return type_identity<input_iterator_tag>{};
    }
  }

  template <bool Const>
  class iterator : public ycxx::detail::category_base<typename decltype(category<Const>())::type> {
    friend zip_transform_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, zip_transform_view>;
    using Base = ycxx::detail::maybe_const<Const, InnerView>;

    Parent* parent_ = nullptr;
    ziperator<Const> current_;

    constexpr iterator(Parent& parent, ziperator<Const> inner)
        : parent_(__builtin_addressof(parent)), current_(std::move(inner)) {}

  public:
    using iterator_concept = typename ziperator<Const>::iterator_concept;
    using value_type = remove_cvref_t<
        invoke_result_t<ycxx::detail::maybe_const<Const, F>&, range_reference_t<ycxx::detail::maybe_const<Const, Views>>...>>;
    using difference_type = range_difference_t<Base>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<ziperator<false>, ziperator<Const>>
        : parent_(i.parent_), current_(std::move(i.current_)) {}

    constexpr decltype(auto) operator*() const
        noexcept(noexcept(::ycxx::detail::invoke(std::declval<ycxx::detail::maybe_const<Const, F>&>(),
                                                 *std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Views>>&>()...))) {
      return std::apply(
          [&](const auto&... iters) -> decltype(auto) { return ::ycxx::detail::invoke(*parent_->fun_, *iters...); },
          ycxx::detail::view_access::current(current_));
    }
    constexpr iterator& operator++() {
      ++current_;
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
    constexpr iterator& operator+=(difference_type x)
      requires random_access_range<Base>
    {
      current_ += x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires random_access_range<Base>
    {
      current_ -= x;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return std::apply(
          [&]<class... Is>(const Is&... iters) -> decltype(auto) {
            return ::ycxx::detail::invoke(*parent_->fun_, iters[iter_difference_t<Is>(n)]...);
          },
          ycxx::detail::view_access::current(current_));
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires equality_comparable<ziperator<Const>>
    {
      return x.current_ == y.current_;
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ + n);
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ + n);
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ - n);
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<ziperator<Const>, ziperator<Const>>
    {
      return x.current_ - y.current_;
    }
  };

  template <bool Const>
  class sentinel {
    friend zip_transform_view;
    friend sentinel<!Const>;

    zentinel<Const> inner_;
    constexpr explicit sentinel(zentinel<Const> inner) : inner_(inner) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> i)
      requires Const && convertible_to<zentinel<false>, zentinel<Const>>
        : inner_(std::move(i.inner_)) {}

    template <bool OtherConst>
      requires sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.inner_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, InnerView>> operator-(
        const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) - y.inner_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, InnerView>> operator-(
        const sentinel& x, const iterator<OtherConst>& y) {
      return x.inner_ - ycxx::detail::view_access::current(y);
    }
  };

  [[no_unique_address]] ycxx::detail::movable_box<F> fun_;
  InnerView zip_;

public:
  zip_transform_view() = default;
  constexpr explicit zip_transform_view(F fun, Views... views) : fun_(in_place, std::move(fun)), zip_(std::move(views)...) {}

  constexpr auto begin() { return iterator<false>(*this, zip_.begin()); }
  constexpr auto begin() const
    requires range<const InnerView> && regular_invocable<const F&, range_reference_t<const Views>...>
  {
    return iterator<true>(*this, zip_.begin());
  }
  constexpr auto end() {
    if constexpr (common_range<InnerView>)
      return iterator<false>(*this, zip_.end());
    else
      return sentinel<false>(zip_.end());
  }
  constexpr auto end() const
    requires range<const InnerView> && regular_invocable<const F&, range_reference_t<const Views>...>
  {
    if constexpr (common_range<const InnerView>)
      return iterator<true>(*this, zip_.end());
    else
      return sentinel<true>(zip_.end());
  }
  constexpr auto size()
    requires sized_range<InnerView>
  {
    return zip_.size();
  }
  constexpr auto size() const
    requires sized_range<const InnerView>
  {
    return zip_.size();
  }
};
template <class F, class... Rs>
zip_transform_view(F, Rs&&...) -> zip_transform_view<F, views::all_t<Rs>...>;

// =============================================================================================
// [range.adjacent]
// =============================================================================================
template <forward_range V, size_t N>
  requires view<V> && (N > 0)
class adjacent_view : public view_interface<adjacent_view<V, N>> {
  struct as_sentinel {};

  template <bool Const>
  class iterator {
    friend adjacent_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Base = ycxx::detail::maybe_const<Const, V>;

    array<iterator_t<Base>, N> current_ = array<iterator_t<Base>, N>();

    constexpr iterator(iterator_t<Base> first, sentinel_t<Base> last) {
      current_[0] = first;
      for (size_t i = 1; i < N; ++i)
        current_[i] = ranges::next(current_[i - 1], 1, last);
    }
    constexpr iterator(as_sentinel, iterator_t<Base> first, iterator_t<Base> last) {
      if constexpr (!bidirectional_range<Base>) {
        for (auto& it : current_)
          it = last;
      } else {
        current_[N - 1] = last;
        for (size_t i = N - 1; i-- > 0;)
          current_[i] = ranges::prev(current_[i + 1], 1, first);
      }
    }

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept =
        conditional_t<random_access_range<Base>, random_access_iterator_tag,
                      conditional_t<bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type = ycxx::detail::repeat_tuple_t<range_value_t<Base>, N>;
    using difference_type = range_difference_t<Base>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
    {
      for (size_t k = 0; k < N; ++k)
        current_[k] = std::move(i.current_[k]);
    }

    constexpr auto operator*() const {
      return ycxx::detail::tuple_transform([](auto& i) -> decltype(auto) { return *i; }, current_);
    }
    constexpr iterator& operator++() {
      for (auto& i : current_)
        ++i;
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
      for (auto& i : current_)
        --i;
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
      for (auto& i : current_)
        i += x;
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires random_access_range<Base>
    {
      for (auto& i : current_)
        i -= x;
      return *this;
    }
    constexpr auto operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return ycxx::detail::tuple_transform([&](auto& i) -> decltype(auto) { return i[n]; }, current_);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) {
      return x.current_.back() == y.current_.back();
    }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_.back() < y.current_.back();
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
      return x.current_.back() <=> y.current_.back();
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
      return x.current_.back() - y.current_.back();
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        noexcept(ranges::iter_move(std::declval<const iterator_t<Base>&>())) &&
        is_nothrow_move_constructible_v<range_rvalue_reference_t<Base>>) {
      return ycxx::detail::tuple_transform(ranges::iter_move, i.current_);
    }
    friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
        noexcept(ranges::iter_swap(std::declval<iterator_t<Base>>(), std::declval<iterator_t<Base>>())))
      requires indirectly_swappable<iterator_t<Base>>
    {
      for (size_t i = 0; i < N; ++i)
        ranges::iter_swap(l.current_[i], r.current_[i]);
    }
  };

  template <bool Const>
  class sentinel {
    friend adjacent_view;
    friend sentinel<!Const>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    sentinel_t<Base> end_ = sentinel_t<Base>();
    constexpr explicit sentinel(sentinel_t<Base> end) : end_(end) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> i)
      requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(i.end_)) {}

    template <bool OtherConst>
      requires sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x).back() == y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const iterator<OtherConst>& x,
                                                                                            const sentinel& y) {
      return ycxx::detail::view_access::current(x).back() - y.end_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<sentinel_t<Base>, iterator_t<ycxx::detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, V>> operator-(const sentinel& y,
                                                                                            const iterator<OtherConst>& x) {
      return y.end_ - ycxx::detail::view_access::current(x).back();
    }
  };

  V base_ = V();

  template <class R>
  static constexpr auto size_of(R& r) {
    using ST = decltype(ranges::size(r));
    using CT = common_type_t<ST, size_t>;
    auto sz = static_cast<CT>(ranges::size(r));
    sz -= std::min<CT>(sz, N - 1);
    return static_cast<ST>(sz);
  }
  template <class R>
  static constexpr auto hint_of(R& r) {
    using DT = range_difference_t<R>;
    using CT = common_type_t<DT, size_t>;
    auto sz = static_cast<CT>(ranges::reserve_hint(r));
    sz -= std::min<CT>(sz, N - 1);
    return ::ycxx::detail::to_unsigned_like(sz);
  }

public:
  adjacent_view()
    requires default_initializable<V>
  = default;
  constexpr explicit adjacent_view(V base) : base_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return base_;
  }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
    requires(!ycxx::detail::simple_view<V>)
  {
    return iterator<false>(ranges::begin(base_), ranges::end(base_));
  }
  constexpr auto begin() const
    requires range<const V>
  {
    return iterator<true>(ranges::begin(base_), ranges::end(base_));
  }
  constexpr auto end()
    requires(!ycxx::detail::simple_view<V>)
  {
    if constexpr (common_range<V>)
      return iterator<false>(as_sentinel{}, ranges::begin(base_), ranges::end(base_));
    else
      return sentinel<false>(ranges::end(base_));
  }
  constexpr auto end() const
    requires range<const V>
  {
    if constexpr (common_range<const V>)
      return iterator<true>(as_sentinel{}, ranges::begin(base_), ranges::end(base_));
    else
      return sentinel<true>(ranges::end(base_));
  }
  constexpr auto size()
    requires sized_range<V>
  {
    return size_of(base_);
  }
  constexpr auto size() const
    requires sized_range<const V>
  {
    return size_of(base_);
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<V>
  {
    return hint_of(base_);
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const V>
  {
    return hint_of(base_);
  }
};
template <class V, size_t N>
constexpr bool enable_borrowed_range<adjacent_view<V, N>> = enable_borrowed_range<V>;

// =============================================================================================
// [range.adjacent.transform]
// =============================================================================================
template <forward_range V, move_constructible F, size_t N>
  requires view<V> && (N > 0) && is_object_v<F> &&
           ycxx::detail::repeat_regular_invocable<F&, range_reference_t<V>, N> &&
           ycxx::detail::can_reference<ycxx::detail::repeat_invoke_result_t<F&, range_reference_t<V>, N>>
class adjacent_transform_view : public view_interface<adjacent_transform_view<V, F, N>> {
  using InnerView = adjacent_view<V, N>;
  template <bool Const>
  using inner_iterator = iterator_t<ycxx::detail::maybe_const<Const, InnerView>>;
  template <bool Const>
  using inner_sentinel = sentinel_t<ycxx::detail::maybe_const<Const, InnerView>>;

  template <bool Const>
  static consteval auto category() {
    using Base = ycxx::detail::maybe_const<Const, V>;
    if constexpr (!is_reference_v<ycxx::detail::repeat_invoke_result_t<ycxx::detail::maybe_const<Const, F>&,
                                                                        range_reference_t<Base>, N>>) {
      return input_iterator_tag{};
    } else {
      using C = ycxx::detail::iter_category_t<iterator_t<Base>>;
      if constexpr (derived_from<C, random_access_iterator_tag>)
        return random_access_iterator_tag{};
      else if constexpr (derived_from<C, bidirectional_iterator_tag>)
        return bidirectional_iterator_tag{};
      else if constexpr (derived_from<C, forward_iterator_tag>)
        return forward_iterator_tag{};
      else
        return input_iterator_tag{};
    }
  }

  template <bool Const>
  class iterator {
    friend adjacent_transform_view;
    friend iterator<!Const>;
    friend ycxx::detail::view_access;
    using Parent = ycxx::detail::maybe_const<Const, adjacent_transform_view>;
    using Base = ycxx::detail::maybe_const<Const, V>;

    Parent* parent_ = nullptr;
    inner_iterator<Const> current_;

    constexpr iterator(Parent& parent, inner_iterator<Const> inner)
        : parent_(__builtin_addressof(parent)), current_(std::move(inner)) {}

    template <class Fn>
    constexpr decltype(auto) apply_inner(Fn&& fn) const {
      auto& its = ycxx::detail::view_access::current(current_);
      return [&]<size_t... I>(index_sequence<I...>) -> decltype(auto) {
        return fn(its[I]...);
      }(make_index_sequence<N>{});
    }

  public:
    using iterator_category = decltype(category<Const>());
    using iterator_concept = typename inner_iterator<Const>::iterator_concept;
    using value_type = remove_cvref_t<
        ycxx::detail::repeat_invoke_result_t<ycxx::detail::maybe_const<Const, F>&, range_reference_t<Base>, N>>;
    using difference_type = range_difference_t<Base>;

    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && convertible_to<inner_iterator<false>, inner_iterator<Const>>
        : parent_(i.parent_), current_(std::move(i.current_)) {}

    static consteval bool deref_nothrow() {
      return []<size_t... I>(index_sequence<I...>) {
        return noexcept(::ycxx::detail::invoke(
            std::declval<ycxx::detail::maybe_const<Const, F>&>(),
            *std::declval<ycxx::detail::repeat_type<const iterator_t<Base>&, I>>()...));
      }(make_index_sequence<N>{});
    }
    constexpr decltype(auto) operator*() const noexcept(deref_nothrow()) {
      return apply_inner(
          [&](const auto&... iters) -> decltype(auto) { return ::ycxx::detail::invoke(*parent_->fun_, *iters...); });
    }
    constexpr iterator& operator++() {
      ++current_;
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
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires random_access_range<Base>
    {
      current_ -= x;
      return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
      requires random_access_range<Base>
    {
      return apply_inner(
          [&](const auto&... iters) -> decltype(auto) { return ::ycxx::detail::invoke(*parent_->fun_, iters[n]...); });
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) { return x.current_ == y.current_; }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ < y.current_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ > y.current_;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ <= y.current_;
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
      requires random_access_range<Base>
    {
      return x.current_ >= y.current_;
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires random_access_range<Base> && three_way_comparable<inner_iterator<Const>>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ + n);
    }
    friend constexpr iterator operator+(difference_type n, const iterator& i)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ + n);
    }
    friend constexpr iterator operator-(const iterator& i, difference_type n)
      requires random_access_range<Base>
    {
      return iterator(*i.parent_, i.current_ - n);
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires sized_sentinel_for<inner_iterator<Const>, inner_iterator<Const>>
    {
      return x.current_ - y.current_;
    }
  };

  template <bool Const>
  class sentinel {
    friend adjacent_transform_view;
    friend sentinel<!Const>;

    inner_sentinel<Const> inner_;
    constexpr explicit sentinel(inner_sentinel<Const> inner) : inner_(inner) {}

  public:
    sentinel() = default;
    constexpr sentinel(sentinel<!Const> i)
      requires Const && convertible_to<inner_sentinel<false>, inner_sentinel<Const>>
        : inner_(std::move(i.inner_)) {}

    template <bool OtherConst>
      requires sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) == y.inner_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, InnerView>> operator-(
        const iterator<OtherConst>& x, const sentinel& y) {
      return ycxx::detail::view_access::current(x) - y.inner_;
    }
    template <bool OtherConst>
      requires sized_sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr range_difference_t<ycxx::detail::maybe_const<OtherConst, InnerView>> operator-(
        const sentinel& x, const iterator<OtherConst>& y) {
      return x.inner_ - ycxx::detail::view_access::current(y);
    }
  };

  [[no_unique_address]] ycxx::detail::movable_box<F> fun_;
  InnerView inner_;

public:
  adjacent_transform_view() = default;
  constexpr explicit adjacent_transform_view(V base, F fun) : fun_(in_place, std::move(fun)), inner_(std::move(base)) {}

  constexpr V base() const&
    requires copy_constructible<V>
  {
    return inner_.base();
  }
  constexpr V base() && { return std::move(inner_).base(); }

  constexpr auto begin() { return iterator<false>(*this, inner_.begin()); }
  constexpr auto begin() const
    requires range<const InnerView> && ycxx::detail::repeat_regular_invocable<const F&, range_reference_t<const V>, N>
  {
    return iterator<true>(*this, inner_.begin());
  }
  constexpr auto end() {
    if constexpr (common_range<InnerView>)
      return iterator<false>(*this, inner_.end());
    else
      return sentinel<false>(inner_.end());
  }
  constexpr auto end() const
    requires range<const InnerView> && ycxx::detail::repeat_regular_invocable<const F&, range_reference_t<const V>, N>
  {
    if constexpr (common_range<const InnerView>)
      return iterator<true>(*this, inner_.end());
    else
      return sentinel<true>(inner_.end());
  }
  constexpr auto size()
    requires sized_range<InnerView>
  {
    return inner_.size();
  }
  constexpr auto size() const
    requires sized_range<const InnerView>
  {
    return inner_.size();
  }
  constexpr auto reserve_hint()
    requires approximately_sized_range<InnerView>
  {
    return inner_.reserve_hint();
  }
  constexpr auto reserve_hint() const
    requires approximately_sized_range<const InnerView>
  {
    return inner_.reserve_hint();
  }
};

} // namespace std::ranges

// =============================================================================================
// [range.cartesian]
// =============================================================================================
namespace ycxx::detail {

template <bool Const, class First, class... Vs>
concept cartesian_product_is_random_access =
    (std::ranges::random_access_range<maybe_const<Const, First>> && ... &&
     (std::ranges::random_access_range<maybe_const<Const, Vs>> && std::ranges::sized_range<maybe_const<Const, Vs>>));
template <class R>
concept cartesian_product_common_arg =
    std::ranges::common_range<R> || (std::ranges::sized_range<R> && std::ranges::random_access_range<R>);
template <bool Const, class First, class... Vs>
concept cartesian_product_is_bidirectional =
    (std::ranges::bidirectional_range<maybe_const<Const, First>> && ... &&
     (std::ranges::bidirectional_range<maybe_const<Const, Vs>> && cartesian_product_common_arg<maybe_const<Const, Vs>>));
template <class First, class...>
concept cartesian_product_is_common = cartesian_product_common_arg<First>;
template <class... Vs>
concept cartesian_product_is_sized = (std::ranges::sized_range<Vs> && ...);
template <bool Const, template <class> class FirstSent, class First, class... Vs>
concept cartesian_is_sized_sentinel =
    (std::sized_sentinel_for<FirstSent<maybe_const<Const, First>>, std::ranges::iterator_t<maybe_const<Const, First>>> &&
     ... &&
     (std::ranges::sized_range<maybe_const<Const, Vs>> &&
      std::sized_sentinel_for<std::ranges::iterator_t<maybe_const<Const, Vs>>,
                              std::ranges::iterator_t<maybe_const<Const, Vs>>>));

template <cartesian_product_common_arg R>
constexpr auto cartesian_common_arg_end(R& r) {
  if constexpr (std::ranges::common_range<R>)
    return std::ranges::end(r);
  else
    return std::ranges::begin(r) + std::ranges::distance(r);
}

template <class R>
using cartesian_sentinel_t = std::ranges::sentinel_t<R>;
template <class R>
using cartesian_iterator_t = std::ranges::iterator_t<R>;

// The difference type: the first range's alone; for a product of several ranges a 128-bit type
// where available, wide enough for the product of two 64-bit sizes.
template <class... Ds>
consteval auto cartesian_difference() {
  if constexpr (sizeof...(Ds) > 1 && cfg::has_int128)
    return std::type_identity<int128>{};
  else
    return std::type_identity<std::common_type_t<std::ptrdiff_t, Ds...>>{};
}

} // namespace ycxx::detail

namespace std::ranges {

template <input_range First, forward_range... Vs>
  requires(view<First> && ... && view<Vs>)
class cartesian_product_view : public view_interface<cartesian_product_view<First, Vs...>> {
  template <bool Const>
  class iterator {
    friend cartesian_product_view;
    friend iterator<!Const>;
    using Parent = ycxx::detail::maybe_const<Const, cartesian_product_view>;

  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept = conditional_t<
        ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>, random_access_iterator_tag,
        conditional_t<ycxx::detail::cartesian_product_is_bidirectional<Const, First, Vs...>, bidirectional_iterator_tag,
                      conditional_t<forward_range<ycxx::detail::maybe_const<Const, First>>, forward_iterator_tag,
                                    input_iterator_tag>>>;
    using value_type =
        tuple<range_value_t<ycxx::detail::maybe_const<Const, First>>, range_value_t<ycxx::detail::maybe_const<Const, Vs>>...>;
    using reference = tuple<range_reference_t<ycxx::detail::maybe_const<Const, First>>,
                            range_reference_t<ycxx::detail::maybe_const<Const, Vs>>...>;
    using difference_type = typename decltype(ycxx::detail::cartesian_difference<
                                              range_difference_t<ycxx::detail::maybe_const<Const, First>>,
                                              range_difference_t<ycxx::detail::maybe_const<Const, Vs>>...>())::type;

  private:
    Parent* parent_ = nullptr;
    tuple<iterator_t<ycxx::detail::maybe_const<Const, First>>, iterator_t<ycxx::detail::maybe_const<Const, Vs>>...>
        current_;

    template <size_t N = sizeof...(Vs)>
    constexpr void next() {
      auto& it = std::get<N>(current_);
      ++it;
      if constexpr (N > 0) {
        if (it == ranges::end(std::get<N>(parent_->bases_))) {
          it = ranges::begin(std::get<N>(parent_->bases_));
          next<N - 1>();
        }
      }
    }
    template <size_t N = sizeof...(Vs)>
    constexpr void prev() {
      auto& it = std::get<N>(current_);
      if constexpr (N > 0) {
        if (it == ranges::begin(std::get<N>(parent_->bases_))) {
          it = ycxx::detail::cartesian_common_arg_end(std::get<N>(parent_->bases_));
          prev<N - 1>();
        }
      }
      --it;
    }
    // Moves the Nth iterator by x positions with carry into the (N-1)th (random access).
    template <size_t N = sizeof...(Vs)>
    constexpr void advance(difference_type x) {
      auto& it = std::get<N>(current_);
      if constexpr (N == 0) {
        it += static_cast<range_difference_t<decltype(std::get<N>(parent_->bases_))>>(x);
      } else {
        auto& base = std::get<N>(parent_->bases_);
        auto size = static_cast<difference_type>(ranges::size(base));
        if (size == 0)
          return;
        auto first = ranges::begin(base);
        difference_type pos = static_cast<difference_type>(it - first) + x;
        difference_type carry = pos / size;
        pos %= size;
        if (pos < 0) {
          pos += size;
          --carry;
        }
        it = first + static_cast<range_difference_t<decltype(base)>>(pos);
        if (carry != 0)
          advance<N - 1>(carry);
      }
    }
    template <class Tuple>
    constexpr difference_type distance_from(const Tuple& t) const {
      return [&]<size_t... I>(index_sequence<I...>) {
        difference_type sum = 0;
        ((sum += static_cast<difference_type>(std::get<I>(current_) - std::get<I>(t)) * scaled_size<I + 1>()), ...);
        return sum;
      }(make_index_sequence<1 + sizeof...(Vs)>{});
    }
    template <size_t N>
    constexpr difference_type scaled_size() const {
      if constexpr (N <= sizeof...(Vs))
        return static_cast<difference_type>(ranges::size(std::get<N>(parent_->bases_))) * scaled_size<N + 1>();
      else
        return static_cast<difference_type>(1);
    }

    constexpr iterator(Parent& parent,
                       tuple<iterator_t<ycxx::detail::maybe_const<Const, First>>,
                             iterator_t<ycxx::detail::maybe_const<Const, Vs>>...>
                           current)
        : parent_(__builtin_addressof(parent)), current_(std::move(current)) {}

  public:
    iterator() = default;
    constexpr iterator(iterator<!Const> i)
      requires Const && (convertible_to<iterator_t<First>, iterator_t<const First>> && ... &&
                         convertible_to<iterator_t<Vs>, iterator_t<const Vs>>)
        : parent_(i.parent_), current_(std::move(i.current_)) {}

    constexpr auto operator*() const {
      return ycxx::detail::tuple_transform([](auto& i) -> decltype(auto) { return *i; }, current_);
    }
    constexpr iterator& operator++() {
      next();
      return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
      requires forward_range<ycxx::detail::maybe_const<Const, First>>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr iterator& operator--()
      requires ycxx::detail::cartesian_product_is_bidirectional<Const, First, Vs...>
    {
      prev();
      return *this;
    }
    constexpr iterator operator--(int)
      requires ycxx::detail::cartesian_product_is_bidirectional<Const, First, Vs...>
    {
      auto tmp = *this;
      --*this;
      return tmp;
    }
    constexpr iterator& operator+=(difference_type x)
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      if (x != 0)
        advance(x);
      return *this;
    }
    constexpr iterator& operator-=(difference_type x)
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      *this += -x;
      return *this;
    }
    constexpr reference operator[](difference_type n) const
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      return *((*this) + n);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<ycxx::detail::maybe_const<Const, First>>>
    {
      return x.current_ == y.current_;
    }
    friend constexpr bool operator==(const iterator& x, default_sentinel_t) {
      return [&]<size_t... I>(index_sequence<I...>) {
        return ((std::get<I>(x.current_) == ranges::end(std::get<I>(x.parent_->bases_))) || ...);
      }(make_index_sequence<1 + sizeof...(Vs)>{});
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
      requires ycxx::detail::all_random_access<Const, First, Vs...>
    {
      return x.current_ <=> y.current_;
    }
    friend constexpr iterator operator+(const iterator& x, difference_type y)
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      return iterator(x) += y;
    }
    friend constexpr iterator operator+(difference_type x, const iterator& y)
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      return y + x;
    }
    friend constexpr iterator operator-(const iterator& x, difference_type y)
      requires ycxx::detail::cartesian_product_is_random_access<Const, First, Vs...>
    {
      return iterator(x) -= y;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
      requires ycxx::detail::cartesian_is_sized_sentinel<Const, ycxx::detail::cartesian_iterator_t, First, Vs...>
    {
      return x.distance_from(y.current_);
    }
    friend constexpr difference_type operator-(const iterator& i, default_sentinel_t)
      requires ycxx::detail::cartesian_is_sized_sentinel<Const, ycxx::detail::cartesian_sentinel_t, First, Vs...>
    {
      auto end_tuple = [&]<size_t... I>(index_sequence<I...>) {
        return tuple(ranges::end(std::get<0>(i.parent_->bases_)), ranges::begin(std::get<I + 1>(i.parent_->bases_))...);
      }(index_sequence_for<Vs...>{});
      return i.distance_from(end_tuple);
    }
    friend constexpr difference_type operator-(default_sentinel_t s, const iterator& i)
      requires ycxx::detail::cartesian_is_sized_sentinel<Const, ycxx::detail::cartesian_sentinel_t, First, Vs...>
    {
      return -(i - s);
    }
    friend constexpr auto iter_move(const iterator& i) noexcept(
        (noexcept(ranges::iter_move(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, First>>&>())) && ... &&
         noexcept(ranges::iter_move(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Vs>>&>()))) &&
        (is_nothrow_move_constructible_v<range_rvalue_reference_t<ycxx::detail::maybe_const<Const, First>>> && ... &&
         is_nothrow_move_constructible_v<range_rvalue_reference_t<ycxx::detail::maybe_const<Const, Vs>>>)) {
      return ycxx::detail::tuple_transform(ranges::iter_move, i.current_);
    }
    friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
        (noexcept(ranges::iter_swap(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, First>>&>(),
                                    std::declval<const iterator_t<ycxx::detail::maybe_const<Const, First>>&>())) &&
         ... &&
         noexcept(ranges::iter_swap(std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Vs>>&>(),
                                    std::declval<const iterator_t<ycxx::detail::maybe_const<Const, Vs>>&>()))))
      requires(indirectly_swappable<iterator_t<ycxx::detail::maybe_const<Const, First>>> && ... &&
               indirectly_swappable<iterator_t<ycxx::detail::maybe_const<Const, Vs>>>)
    {
      [&]<size_t... I>(index_sequence<I...>) {
        (ranges::iter_swap(std::get<I>(l.current_), std::get<I>(r.current_)), ...);
      }(make_index_sequence<1 + sizeof...(Vs)>{});
    }
  };

  tuple<First, Vs...> bases_;

  template <bool Const, class Self>
  static constexpr iterator<Const> end_impl(Self& self) {
    bool is_empty = [&]<size_t... I>(index_sequence<I...>) {
      return (ranges::empty(std::get<I + 1>(self.bases_)) || ...);
    }(index_sequence_for<Vs...>{});
    auto& first = std::get<0>(self.bases_);
    return [&]<size_t... I>(index_sequence<I...>) {
      return iterator<Const>(self, tuple<iterator_t<ycxx::detail::maybe_const<Const, First>>,
                                         iterator_t<ycxx::detail::maybe_const<Const, Vs>>...>(
                                       is_empty ? ranges::begin(first) : ycxx::detail::cartesian_common_arg_end(first),
                                       ranges::begin(std::get<I + 1>(self.bases_))...));
    }(index_sequence_for<Vs...>{});
  }
  template <class Self>
  static constexpr auto size_impl(Self& self) {
    using D = typename iterator<is_const_v<Self>>::difference_type;
    using U = make_unsigned_t<D>;
    return std::apply([](auto&... bases) { return (U(1) * ... * static_cast<U>(ranges::size(bases))); }, self.bases_);
  }

public:
  constexpr cartesian_product_view() = default;
  constexpr explicit cartesian_product_view(First first_base, Vs... bases)
      : bases_(std::move(first_base), std::move(bases)...) {}

  constexpr iterator<false> begin()
    requires(!ycxx::detail::simple_view<First> || ... || !ycxx::detail::simple_view<Vs>)
  {
    return iterator<false>(*this, ycxx::detail::tuple_transform(ranges::begin, bases_));
  }
  constexpr iterator<true> begin() const
    requires(range<const First> && ... && range<const Vs>)
  {
    return iterator<true>(*this, ycxx::detail::tuple_transform(ranges::begin, bases_));
  }
  constexpr iterator<false> end()
    requires((!ycxx::detail::simple_view<First> || ... || !ycxx::detail::simple_view<Vs>) &&
             ycxx::detail::cartesian_product_is_common<First, Vs...>)
  {
    return end_impl<false>(*this);
  }
  constexpr iterator<true> end() const
    requires ycxx::detail::cartesian_product_is_common<const First, const Vs...>
  {
    return end_impl<true>(*this);
  }
  constexpr default_sentinel_t end() const noexcept { return default_sentinel; }
  constexpr auto size()
    requires ycxx::detail::cartesian_product_is_sized<First, Vs...>
  {
    return size_impl(*this);
  }
  constexpr auto size() const
    requires ycxx::detail::cartesian_product_is_sized<const First, const Vs...>
  {
    return size_impl(*this);
  }
};
template <class... Vs>
cartesian_product_view(Vs&&...) -> cartesian_product_view<views::all_t<Vs>...>;

} // namespace std::ranges

// =============================================================================================
// The adaptor objects
// =============================================================================================
namespace ycxx::detail::view_fn {

template <std::size_t N>
struct elements_fn : std::ranges::range_adaptor_closure<elements_fn<N>> {
  template <class E>
    requires requires { std::ranges::elements_view<std::views::all_t<E>, N>{std::declval<E>()}; }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    return std::ranges::elements_view<std::views::all_t<E>, N>{static_cast<E&&>(e)};
  }
};

struct enumerate_fn : std::ranges::range_adaptor_closure<enumerate_fn> {
  template <class E>
    requires requires { std::ranges::enumerate_view<std::views::all_t<E>>(std::declval<E>()); }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    return std::ranges::enumerate_view<std::views::all_t<E>>(static_cast<E&&>(e));
  }
};

struct zip_fn {
  [[nodiscard]] constexpr auto operator()() const noexcept { return auto(std::views::empty<std::tuple<>>); }
  template <class... Es>
    requires(sizeof...(Es) > 0) &&
            requires { std::ranges::zip_view<std::views::all_t<Es>...>(std::declval<Es>()...); }
  [[nodiscard]] constexpr auto operator()(Es&&... es) const {
    return std::ranges::zip_view<std::views::all_t<Es>...>(static_cast<Es&&>(es)...);
  }
};

struct zip_transform_fn {
  template <class F>
    requires std::move_constructible<std::decay_t<F>> && std::regular_invocable<std::decay_t<F>&> &&
             std::is_object_v<std::decay_t<std::invoke_result_t<std::decay_t<F>&>>>
  [[nodiscard]] constexpr auto operator()(F&& f) const {
    (void)f;
    return auto(std::views::empty<std::decay_t<std::invoke_result_t<std::decay_t<F>&>>>);
  }
  template <class F, class... Es>
    requires(sizeof...(Es) > 0) && requires { std::ranges::zip_transform_view(std::declval<F>(), std::declval<Es>()...); }
  [[nodiscard]] constexpr auto operator()(F&& f, Es&&... es) const {
    return std::ranges::zip_transform_view(static_cast<F&&>(f), static_cast<Es&&>(es)...);
  }
};

template <std::size_t N>
struct adjacent_fn : std::ranges::range_adaptor_closure<adjacent_fn<N>> {
  template <class E>
    requires(N == 0 && std::ranges::forward_range<E>) ||
            requires { std::ranges::adjacent_view<std::views::all_t<E>, N>(std::declval<E>()); }
  [[nodiscard]] constexpr auto operator()(E&& e) const {
    if constexpr (N == 0 && std::ranges::forward_range<E>)
      return ((void)e, auto(std::views::empty<std::tuple<>>));
    else
      return std::ranges::adjacent_view<std::views::all_t<E>, N>(static_cast<E&&>(e));
  }
};

template <std::size_t N>
struct adjacent_transform_fn {
  template <class E, class F>
    requires(N == 0 && std::ranges::forward_range<E> && requires { zip_transform_fn{}(std::declval<F>()); }) ||
            requires {
              std::ranges::adjacent_transform_view<std::views::all_t<E>, std::decay_t<F>, N>(std::declval<E>(),
                                                                                            std::declval<F>());
            }
  [[nodiscard]] constexpr auto operator()(E&& e, F&& f) const {
    if constexpr (N == 0 && std::ranges::forward_range<E>)
      return ((void)e, zip_transform_fn{}(static_cast<F&&>(f)));
    else
      return std::ranges::adjacent_transform_view<std::views::all_t<E>, std::decay_t<F>, N>(static_cast<E&&>(e),
                                                                                           static_cast<F&&>(f));
  }
  template <class F>
  [[nodiscard]] constexpr auto operator()(F&& f) const
      noexcept(noexcept(::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f))))
    requires requires { ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f)); }
  {
    return ::ycxx::detail::bind_adaptor(*this, static_cast<F&&>(f));
  }
};

struct cartesian_product_fn {
  [[nodiscard]] constexpr auto operator()() const { return std::views::single(std::tuple()); }
  template <class... Es>
    requires(sizeof...(Es) > 0) &&
            requires { std::ranges::cartesian_product_view<std::views::all_t<Es>...>(std::declval<Es>()...); }
  [[nodiscard]] constexpr auto operator()(Es&&... es) const {
    return std::ranges::cartesian_product_view<std::views::all_t<Es>...>(static_cast<Es&&>(es)...);
  }
};

} // namespace ycxx::detail::view_fn

namespace std::ranges::views {
template <size_t N>
constexpr ycxx::detail::view_fn::elements_fn<N> elements{};
inline constexpr auto keys = elements<0>;
inline constexpr auto values = elements<1>;
inline constexpr ycxx::detail::view_fn::enumerate_fn enumerate{};
inline constexpr ycxx::detail::view_fn::zip_fn zip{};
inline constexpr ycxx::detail::view_fn::zip_transform_fn zip_transform{};
template <size_t N>
constexpr ycxx::detail::view_fn::adjacent_fn<N> adjacent{};
inline constexpr auto pairwise = adjacent<2>;
template <size_t N>
constexpr ycxx::detail::view_fn::adjacent_transform_fn<N> adjacent_transform{};
inline constexpr auto pairwise_transform = adjacent_transform<2>;
inline constexpr ycxx::detail::view_fn::cartesian_product_fn cartesian_product{};
} // namespace std::ranges::views
