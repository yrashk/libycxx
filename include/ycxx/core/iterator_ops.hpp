// libycxx core: iterator operations ([iterator.operations], [range.iter.ops]) and sentinels.
#pragma once

#include <ycxx/core/range_access.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [iterator.operations]
template <class _InputIt, class _Distance>
constexpr void advance(_InputIt& __it, _Distance n) {
  using cat = typename iterator_traits<_InputIt>::iterator_category;
  auto d = static_cast<typename iterator_traits<_InputIt>::difference_type>(n);
  if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
    __it += d;
  } else if constexpr (is_base_of_v<bidirectional_iterator_tag, cat>) {
    for (; d > 0; --d)
      ++__it;
    for (; d < 0; ++d)
      --__it;
  } else {
    __ycxx::__detail::__precondition(d >= 0, "std::advance: negative distance for a non-bidirectional iterator");
    for (; d > 0; --d)
      ++__it;
  }
}

template <class _InputIt>
constexpr typename iterator_traits<_InputIt>::difference_type distance(_InputIt first, _InputIt last) {
  using cat = typename iterator_traits<_InputIt>::iterator_category;
  if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
    return last - first;
  } else {
    typename iterator_traits<_InputIt>::difference_type n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

template <class _InputIt>
constexpr _InputIt next(_InputIt __it, typename iterator_traits<_InputIt>::difference_type n = 1) {
  std::advance(__it, n);
  return __it;
}
template <class _BidirIt>
constexpr _BidirIt prev(_BidirIt __it, typename iterator_traits<_BidirIt>::difference_type n = 1) {
  std::advance(__it, -n);
  return __it;
}

// [default.sentinel], [unreachable.sentinel]
struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel{};

struct unreachable_sentinel_t {
  template <weakly_incrementable _Ip>
  friend constexpr bool operator==(unreachable_sentinel_t, const _Ip&) noexcept {
    return false;
  }
};
inline constexpr unreachable_sentinel_t unreachable_sentinel{};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__iter_ops {

struct __advance_fn {
  template <std::input_or_output_iterator _Ip>
  constexpr void operator()(_Ip& i, std::iter_difference_t<_Ip> n) const {
    if constexpr (std::random_access_iterator<_Ip>) {
      i += n;
    } else {
      if constexpr (std::bidirectional_iterator<_Ip>) {
        for (; n < 0; ++n)
          --i;
      } else {
        __precondition(n >= 0, "ranges::advance: negative n for a non-bidirectional iterator");
      }
      for (; n > 0; --n)
        ++i;
    }
  }

  template <std::input_or_output_iterator _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr void operator()(_Ip& i, _Sp __y_bound) const {
    if constexpr (std::assignable_from<_Ip&, _Sp>)
      i = static_cast<_Sp&&>(__y_bound);
    else if constexpr (std::sized_sentinel_for<_Sp, _Ip>)
      (*this)(i, __y_bound - i);
    else
      while (i != __y_bound)
        ++i;
  }

  template <std::input_or_output_iterator _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr std::iter_difference_t<_Ip> operator()(_Ip& i, std::iter_difference_t<_Ip> n, _Sp __y_bound) const {
    if constexpr (std::sized_sentinel_for<_Sp, _Ip>) {
      // [range.iter.op.advance]/6.1: if |n| >= |bound - i| go to bound, else advance by n. With
      // opposite signs |n| >= |d| is n + d >= 0 (n >= 0) or n + d <= 0, which cannot overflow.
      const auto d = __y_bound - i;
      const bool __reach = (n >= 0) == (d >= 0) ? (n >= 0 ? n >= d : n <= d) : (n >= 0 ? n + d >= 0 : n + d <= 0);
      if (__reach) {
        (*this)(i, __y_bound);
        return n - d;
      }
      (*this)(i, n);
      return 0;
    } else {
      if constexpr (std::bidirectional_iterator<_Ip> && std::same_as<_Ip, _Sp>) {
        for (; n < 0 && i != __y_bound; ++n)
          --i;
      }
      for (; n > 0 && i != __y_bound; --n)
        ++i;
      return n;
    }
  }
};

struct __distance_fn {
  template <std::input_or_output_iterator _Ip, std::sentinel_for<_Ip> _Sp>
    requires(!std::sized_sentinel_for<_Sp, _Ip>)
  constexpr std::iter_difference_t<_Ip> operator()(_Ip first, _Sp last) const {
    std::iter_difference_t<_Ip> n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
  template <class _Ip, std::sized_sentinel_for<std::decay_t<_Ip>> _Sp>
  constexpr std::iter_difference_t<std::decay_t<_Ip>> operator()(_Ip&& first, _Sp last) const {
    // [range.iter.op.distance]/3 (LWG 4242): first itself, so a volatile iterator works; an
    // array decays.
    if constexpr (!std::is_array_v<std::remove_reference_t<_Ip>>)
      return last - first;
    else
      return last - static_cast<std::decay_t<_Ip>>(first);
  }
  template <std::ranges::range _Rp>
  constexpr std::ranges::range_difference_t<_Rp> operator()(_Rp&& r) const {
    if constexpr (std::ranges::sized_range<_Rp>)
      return static_cast<std::ranges::range_difference_t<_Rp>>(std::ranges::size(r));
    else
      return (*this)(std::ranges::begin(r), std::ranges::end(r));
  }
};

struct __next_fn {
  template <std::input_or_output_iterator _Ip>
  constexpr _Ip operator()(_Ip __x) const {
    ++__x;
    return __x;
  }
  template <std::input_or_output_iterator _Ip>
  constexpr _Ip operator()(_Ip __x, std::iter_difference_t<_Ip> n) const {
    __advance_fn{}(__x, n);
    return __x;
  }
  template <std::input_or_output_iterator _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr _Ip operator()(_Ip __x, _Sp __y_bound) const {
    __advance_fn{}(__x, __y_bound);
    return __x;
  }
  template <std::input_or_output_iterator _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr _Ip operator()(_Ip __x, std::iter_difference_t<_Ip> n, _Sp __y_bound) const {
    __advance_fn{}(__x, n, __y_bound);
    return __x;
  }
};

struct __prev_fn {
  template <std::bidirectional_iterator _Ip>
  constexpr _Ip operator()(_Ip __x) const {
    --__x;
    return __x;
  }
  template <std::bidirectional_iterator _Ip>
  constexpr _Ip operator()(_Ip __x, std::iter_difference_t<_Ip> n) const {
    __advance_fn{}(__x, -n);
    return __x;
  }
  template <std::bidirectional_iterator _Ip>
  constexpr _Ip operator()(_Ip __x, std::iter_difference_t<_Ip> n, _Ip __y_bound) const {
    __advance_fn{}(__x, -n, __y_bound);
    return __x;
  }
};

}} // namespace __ycxx::__detail::__iter_ops

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__detail::__iter_ops::__advance_fn advance{};
inline constexpr __ycxx::__detail::__iter_ops::__distance_fn distance{};
inline constexpr __ycxx::__detail::__iter_ops::__next_fn next{};
inline constexpr __ycxx::__detail::__iter_ops::__prev_fn prev{};
}} // namespace std::ranges
