// libycxx core: the range utilities the algorithms need ([range.utility]): the view concept,
// view_interface, subrange and dangling / borrowed_iterator_t / borrowed_subrange_t.
// Reachable through <ranges> and <algorithm>; the views themselves live elsewhere.
#pragma once

#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/error.hpp>

namespace ycxx::detail {
template <class T>
inline constexpr bool is_init_list_v = false;
template <class T>
inline constexpr bool is_init_list_v<std::initializer_list<T>> = true;
} // namespace ycxx::detail

namespace std::ranges {

// [range.view]
template <class T>
concept view = range<T> && movable<T> && enable_view<T>;

template <class T>
concept viewable_range =
    range<T> && ((view<remove_cvref_t<T>> && constructible_from<remove_cvref_t<T>, T>) ||
                 (!view<remove_cvref_t<T>> &&
                  (is_lvalue_reference_v<T> || (movable<remove_reference_t<T>> && !ycxx::detail::is_init_list_v<remove_cvref_t<T>>))));

} // namespace std::ranges

namespace ycxx::detail {
template <class R>
concept simple_view = std::ranges::view<R> && std::ranges::range<const R> &&
                      std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
                      std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;
} // namespace ycxx::detail

namespace std::ranges {

// [view.interface]
template <class D>
  requires is_class_v<D> && same_as<D, remove_cv_t<D>>
class view_interface {
  constexpr D& derived() noexcept { return static_cast<D&>(*this); }
  constexpr const D& derived() const noexcept { return static_cast<const D&>(*this); }

public:
  constexpr bool empty()
    requires sized_range<D> || forward_range<D>
  {
    if constexpr (sized_range<D>)
      return ranges::size(derived()) == 0;
    else
      return ranges::begin(derived()) == ranges::end(derived());
  }
  constexpr bool empty() const
    requires sized_range<const D> || forward_range<const D>
  {
    if constexpr (sized_range<const D>)
      return ranges::size(derived()) == 0;
    else
      return ranges::begin(derived()) == ranges::end(derived());
  }
  constexpr auto cbegin()
    requires input_range<D>
  {
    return ranges::cbegin(derived());
  }
  constexpr auto cbegin() const
    requires input_range<const D>
  {
    return ranges::cbegin(derived());
  }
  constexpr auto cend()
    requires input_range<D>
  {
    return ranges::cend(derived());
  }
  constexpr auto cend() const
    requires input_range<const D>
  {
    return ranges::cend(derived());
  }
  constexpr explicit operator bool()
    requires requires { ranges::empty(derived()); }
  {
    return !ranges::empty(derived());
  }
  constexpr explicit operator bool() const
    requires requires { ranges::empty(derived()); }
  {
    return !ranges::empty(derived());
  }
  constexpr auto data()
    requires contiguous_iterator<iterator_t<D>>
  {
    return std::to_address(ranges::begin(derived()));
  }
  constexpr auto data() const
    requires range<const D> && contiguous_iterator<iterator_t<const D>>
  {
    return std::to_address(ranges::begin(derived()));
  }
  constexpr auto size()
    requires forward_range<D> && sized_sentinel_for<sentinel_t<D>, iterator_t<D>>
  {
    return ::ycxx::detail::to_unsigned_like(ranges::end(derived()) - ranges::begin(derived()));
  }
  constexpr auto size() const
    requires forward_range<const D> && sized_sentinel_for<sentinel_t<const D>, iterator_t<const D>>
  {
    return ::ycxx::detail::to_unsigned_like(ranges::end(derived()) - ranges::begin(derived()));
  }
  constexpr decltype(auto) front()
    requires forward_range<D>
  {
    ::ycxx::detail::precondition(!empty(), "view_interface::front: empty view");
    return *ranges::begin(derived());
  }
  constexpr decltype(auto) front() const
    requires forward_range<const D>
  {
    ::ycxx::detail::precondition(!empty(), "view_interface::front: empty view");
    return *ranges::begin(derived());
  }
  constexpr decltype(auto) back()
    requires bidirectional_range<D> && common_range<D>
  {
    ::ycxx::detail::precondition(!empty(), "view_interface::back: empty view");
    return *ranges::prev(ranges::end(derived()));
  }
  constexpr decltype(auto) back() const
    requires bidirectional_range<const D> && common_range<const D>
  {
    ::ycxx::detail::precondition(!empty(), "view_interface::back: empty view");
    return *ranges::prev(ranges::end(derived()));
  }
  template <random_access_range R = D>
  constexpr decltype(auto) operator[](range_difference_t<R> n) {
    return ranges::begin(derived())[n];
  }
  template <random_access_range R = const D>
  constexpr decltype(auto) operator[](range_difference_t<R> n) const {
    return ranges::begin(derived())[n];
  }
  template <random_access_range R = D>
    requires sized_range<R>
  constexpr decltype(auto) at(range_difference_t<R> n) {
    if (n < 0 || n >= ranges::distance(derived()))
      ::ycxx::detail::throw_out_of_range("view_interface::at: index out of range");
    return (*this)[n];
  }
  template <random_access_range R = const D>
    requires sized_range<R>
  constexpr decltype(auto) at(range_difference_t<R> n) const {
    if (n < 0 || n >= ranges::distance(derived()))
      ::ycxx::detail::throw_out_of_range("view_interface::at: index out of range");
    return (*this)[n];
  }
};

// [range.subrange]
enum class subrange_kind : bool { unsized, sized };

} // namespace std::ranges

namespace ycxx::detail {
template <class From, class To>
concept uses_nonqualification_pointer_conversion =
    std::is_pointer_v<From> && std::is_pointer_v<To> &&
    !std::convertible_to<std::remove_pointer_t<From> (*)[], std::remove_pointer_t<To> (*)[]>;
template <class From, class To>
concept convertible_to_non_slicing =
    std::convertible_to<From, To> && !uses_nonqualification_pointer_conversion<std::decay_t<From>, std::decay_t<To>>;
template <class T, class U, class V>
concept pair_like_convertible_from = !std::ranges::range<T> && !std::is_reference_v<T> && pair_like<T> &&
                                     std::constructible_from<T, U, V> &&
                                     convertible_to_non_slicing<U, std::tuple_element_t<0, T>> &&
                                     std::convertible_to<V, std::tuple_element_t<1, T>>;

// The stored size of a subrange that is sized only through its constructor argument.
template <class D, bool Store>
struct subrange_size {
  constexpr subrange_size() = default;
  constexpr subrange_size(D) noexcept {}
};
template <class D>
struct subrange_size<D, true> {
  D value = 0;
};
} // namespace ycxx::detail

namespace std::ranges {

template <input_or_output_iterator I, sentinel_for<I> S = I,
          subrange_kind K = sized_sentinel_for<S, I> ? subrange_kind::sized : subrange_kind::unsized>
  requires(K == subrange_kind::sized || !sized_sentinel_for<S, I>)
class subrange : public view_interface<subrange<I, S, K>> {
  static constexpr bool StoreSize = K == subrange_kind::sized && !sized_sentinel_for<S, I>;
  using size_type = make_unsigned_t<iter_difference_t<I>>;

  [[no_unique_address]] I begin_ = I();
  [[no_unique_address]] S end_ = S();
  [[no_unique_address]] ycxx::detail::subrange_size<size_type, StoreSize> size_{};

public:
  subrange()
    requires default_initializable<I>
  = default;

  template <ycxx::detail::convertible_to_non_slicing<I> It>
  constexpr subrange(It i, S s)
    requires(!StoreSize)
      : begin_(std::move(i)), end_(std::move(s)) {}

  template <ycxx::detail::convertible_to_non_slicing<I> It>
  constexpr subrange(It i, S s, size_type n)
    requires(K == subrange_kind::sized)
      : begin_(std::move(i)), end_(std::move(s)) {
    if constexpr (StoreSize)
      size_.value = n;
  }

  template <ycxx::detail::different_from<subrange> R>
    requires borrowed_range<R> && ycxx::detail::convertible_to_non_slicing<iterator_t<R>, I> &&
             convertible_to<sentinel_t<R>, S>
  constexpr subrange(R&& r)
    requires(!StoreSize || sized_range<R>)
      : begin_(ranges::begin(r)), end_(ranges::end(r)) {
    if constexpr (StoreSize)
      size_.value = static_cast<size_type>(ranges::size(r));
  }

  template <borrowed_range R>
    requires ycxx::detail::convertible_to_non_slicing<iterator_t<R>, I> && convertible_to<sentinel_t<R>, S>
  constexpr subrange(R&& r, size_type n)
    requires(K == subrange_kind::sized)
      : subrange{ranges::begin(r), ranges::end(r), n} {}

  template <ycxx::detail::different_from<subrange> PairLike>
    requires ycxx::detail::pair_like_convertible_from<PairLike, const I&, const S&>
  constexpr operator PairLike() const {
    return PairLike(begin_, end_);
  }

  constexpr I begin() const
    requires copyable<I>
  {
    return begin_;
  }
  [[nodiscard]] constexpr I begin()
    requires(!copyable<I>)
  {
    return std::move(begin_);
  }
  constexpr S end() const { return end_; }
  constexpr bool empty() const { return begin_ == end_; }
  constexpr size_type size() const
    requires(K == subrange_kind::sized)
  {
    if constexpr (StoreSize)
      return size_.value;
    else
      return ::ycxx::detail::to_unsigned_like(end_ - begin_);
  }

  [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1) const&
    requires forward_iterator<I>
  {
    auto tmp = *this;
    tmp.advance(n);
    return tmp;
  }
  [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1) && {
    advance(n);
    return std::move(*this);
  }
  [[nodiscard]] constexpr subrange prev(iter_difference_t<I> n = 1) const
    requires bidirectional_iterator<I>
  {
    auto tmp = *this;
    tmp.advance(-n);
    return tmp;
  }
  constexpr subrange& advance(iter_difference_t<I> n) {
    if constexpr (bidirectional_iterator<I>) {
      if (n < 0) {
        ranges::advance(begin_, n);
        if constexpr (StoreSize)
          size_.value += ::ycxx::detail::to_unsigned_like(-n);
        return *this;
      }
    }
    auto d = n - ranges::advance(begin_, n, end_);
    if constexpr (StoreSize)
      size_.value -= ::ycxx::detail::to_unsigned_like(d);
    return *this;
  }
};

template <input_or_output_iterator I, sentinel_for<I> S>
subrange(I, S) -> subrange<I, S>;
template <input_or_output_iterator I, sentinel_for<I> S>
subrange(I, S, make_unsigned_t<iter_difference_t<I>>) -> subrange<I, S, subrange_kind::sized>;
template <borrowed_range R>
subrange(R&&) -> subrange<iterator_t<R>, sentinel_t<R>,
                          (sized_range<R> || sized_sentinel_for<sentinel_t<R>, iterator_t<R>>) ? subrange_kind::sized
                                                                                               : subrange_kind::unsized>;
template <borrowed_range R>
subrange(R&&, make_unsigned_t<range_difference_t<R>>) -> subrange<iterator_t<R>, sentinel_t<R>, subrange_kind::sized>;

template <size_t N, class I, class S, subrange_kind K>
  requires((N == 0 && copyable<I>) || N == 1)
constexpr auto get(const subrange<I, S, K>& r) {
  if constexpr (N == 0)
    return r.begin();
  else
    return r.end();
}
template <size_t N, class I, class S, subrange_kind K>
  requires(N < 2)
constexpr auto get(subrange<I, S, K>&& r) {
  if constexpr (N == 0)
    return r.begin();
  else
    return r.end();
}

template <class I, class S, subrange_kind K>
constexpr bool enable_borrowed_range<subrange<I, S, K>> = true;

// [range.dangling]
struct dangling {
  constexpr dangling() noexcept = default;
  template <class... Args>
  constexpr dangling(Args&&...) noexcept {}
};

template <range R>
using borrowed_iterator_t = conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;
template <range R>
using borrowed_subrange_t = conditional_t<borrowed_range<R>, subrange<iterator_t<R>>, dangling>;

} // namespace std::ranges

namespace std {
using ranges::get;

template <class I, class S, ranges::subrange_kind K>
struct tuple_size<ranges::subrange<I, S, K>> : integral_constant<size_t, 2> {};
template <class I, class S, ranges::subrange_kind K>
struct tuple_element<0, ranges::subrange<I, S, K>> {
  using type = I;
};
template <class I, class S, ranges::subrange_kind K>
struct tuple_element<1, ranges::subrange<I, S, K>> {
  using type = S;
};
template <class I, class S, ranges::subrange_kind K>
struct tuple_element<0, const ranges::subrange<I, S, K>> {
  using type = I;
};
template <class I, class S, ranges::subrange_kind K>
struct tuple_element<1, const ranges::subrange<I, S, K>> {
  using type = S;
};
} // namespace std

namespace ycxx::detail {
template <class I, class S, std::ranges::subrange_kind K>
inline constexpr bool is_tuple_like_impl<std::ranges::subrange<I, S, K>> = true;
// Excluded from pair's and tuple's pair-like/tuple-like constructors and from the pair-like
// uses_allocator_construction_args overload (pair.hpp).
template <class I, class S, std::ranges::subrange_kind K>
inline constexpr bool is_subrange<std::ranges::subrange<I, S, K>> = true;
} // namespace ycxx::detail
