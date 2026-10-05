// libycxx core: iterator operations ([iterator.operations], [range.iter.ops]) and sentinels.
#pragma once

#include <ycxx/core/range_access.hpp>

namespace [[gnu::visibility("hidden")]] std {

// [iterator.operations]
template <class InputIt, class Distance>
constexpr void advance(InputIt& it, Distance n) {
  using cat = typename iterator_traits<InputIt>::iterator_category;
  auto d = static_cast<typename iterator_traits<InputIt>::difference_type>(n);
  if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
    it += d;
  } else if constexpr (is_base_of_v<bidirectional_iterator_tag, cat>) {
    for (; d > 0; --d)
      ++it;
    for (; d < 0; ++d)
      --it;
  } else {
    ycxx::detail::precondition(d >= 0, "std::advance: negative distance for a non-bidirectional iterator");
    for (; d > 0; --d)
      ++it;
  }
}

template <class InputIt>
constexpr typename iterator_traits<InputIt>::difference_type distance(InputIt first, InputIt last) {
  using cat = typename iterator_traits<InputIt>::iterator_category;
  if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
    return last - first;
  } else {
    typename iterator_traits<InputIt>::difference_type n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

template <class InputIt>
constexpr InputIt next(InputIt it, typename iterator_traits<InputIt>::difference_type n = 1) {
  std::advance(it, n);
  return it;
}
template <class BidirIt>
constexpr BidirIt prev(BidirIt it, typename iterator_traits<BidirIt>::difference_type n = 1) {
  std::advance(it, -n);
  return it;
}

// [default.sentinel], [unreachable.sentinel]
struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel{};

struct unreachable_sentinel_t {
  template <weakly_incrementable I>
  friend constexpr bool operator==(unreachable_sentinel_t, const I&) noexcept {
    return false;
  }
};
inline constexpr unreachable_sentinel_t unreachable_sentinel{};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::iter_ops {

struct advance_fn {
  template <std::input_or_output_iterator I>
  constexpr void operator()(I& i, std::iter_difference_t<I> n) const {
    if constexpr (std::random_access_iterator<I>) {
      i += n;
    } else {
      if constexpr (std::bidirectional_iterator<I>) {
        for (; n < 0; ++n)
          --i;
      } else {
        precondition(n >= 0, "ranges::advance: negative n for a non-bidirectional iterator");
      }
      for (; n > 0; --n)
        ++i;
    }
  }

  template <std::input_or_output_iterator I, std::sentinel_for<I> S>
  constexpr void operator()(I& i, S bound) const {
    if constexpr (std::assignable_from<I&, S>)
      i = static_cast<S&&>(bound);
    else if constexpr (std::sized_sentinel_for<S, I>)
      (*this)(i, bound - i);
    else
      while (i != bound)
        ++i;
  }

  template <std::input_or_output_iterator I, std::sentinel_for<I> S>
  constexpr std::iter_difference_t<I> operator()(I& i, std::iter_difference_t<I> n, S bound) const {
    if constexpr (std::sized_sentinel_for<S, I>) {
      // [range.iter.op.advance]/6.1: if |n| >= |bound - i| go to bound, else advance by n. With
      // opposite signs |n| >= |d| is n + d >= 0 (n >= 0) or n + d <= 0, which cannot overflow.
      const auto d = bound - i;
      const bool reach = (n >= 0) == (d >= 0) ? (n >= 0 ? n >= d : n <= d) : (n >= 0 ? n + d >= 0 : n + d <= 0);
      if (reach) {
        (*this)(i, bound);
        return n - d;
      }
      (*this)(i, n);
      return 0;
    } else {
      if constexpr (std::bidirectional_iterator<I> && std::same_as<I, S>) {
        for (; n < 0 && i != bound; ++n)
          --i;
      }
      for (; n > 0 && i != bound; --n)
        ++i;
      return n;
    }
  }
};

struct distance_fn {
  template <std::input_or_output_iterator I, std::sentinel_for<I> S>
    requires(!std::sized_sentinel_for<S, I>)
  constexpr std::iter_difference_t<I> operator()(I first, S last) const {
    std::iter_difference_t<I> n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
  template <class I, std::sized_sentinel_for<std::decay_t<I>> S>
  constexpr std::iter_difference_t<std::decay_t<I>> operator()(I&& first, S last) const {
    return last - static_cast<const std::decay_t<I>&>(first);
  }
  template <std::ranges::range R>
  constexpr std::ranges::range_difference_t<R> operator()(R&& r) const {
    if constexpr (std::ranges::sized_range<R>)
      return static_cast<std::ranges::range_difference_t<R>>(std::ranges::size(r));
    else
      return (*this)(std::ranges::begin(r), std::ranges::end(r));
  }
};

struct next_fn {
  template <std::input_or_output_iterator I>
  constexpr I operator()(I x) const {
    ++x;
    return x;
  }
  template <std::input_or_output_iterator I>
  constexpr I operator()(I x, std::iter_difference_t<I> n) const {
    advance_fn{}(x, n);
    return x;
  }
  template <std::input_or_output_iterator I, std::sentinel_for<I> S>
  constexpr I operator()(I x, S bound) const {
    advance_fn{}(x, bound);
    return x;
  }
  template <std::input_or_output_iterator I, std::sentinel_for<I> S>
  constexpr I operator()(I x, std::iter_difference_t<I> n, S bound) const {
    advance_fn{}(x, n, bound);
    return x;
  }
};

struct prev_fn {
  template <std::bidirectional_iterator I>
  constexpr I operator()(I x) const {
    --x;
    return x;
  }
  template <std::bidirectional_iterator I>
  constexpr I operator()(I x, std::iter_difference_t<I> n) const {
    advance_fn{}(x, -n);
    return x;
  }
  template <std::bidirectional_iterator I>
  constexpr I operator()(I x, std::iter_difference_t<I> n, I bound) const {
    advance_fn{}(x, -n, bound);
    return x;
  }
};

}} // namespace ycxx::detail::iter_ops

namespace [[gnu::visibility("hidden")]] std { namespace ranges {
inline constexpr ycxx::detail::iter_ops::advance_fn advance{};
inline constexpr ycxx::detail::iter_ops::distance_fn distance{};
inline constexpr ycxx::detail::iter_ops::next_fn next{};
inline constexpr ycxx::detail::iter_ops::prev_fn prev{};
}} // namespace std::ranges
