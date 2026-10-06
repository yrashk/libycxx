// libycxx core: ranges::generate_random ([alg.rand.generate], P1068).
//
// A generator's (or distribution's) own bulk member `generate_random(r)` (`generate_random(r, __g)`)
// is used when the range can be passed to it. Otherwise, for a sized range, a bulk member taking a
// span is used: directly on a contiguous range of the result type, else through a local buffer;
// and the values are produced one at a time when there is no bulk member at all.
#pragma once

#include <ycxx/core/invoke.hpp>
#include <ycxx/core/iterator_ops.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/urbg.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

struct __generate_random_fn {
private:
  static constexpr size_t chunk = 64;

  // The end iterator of r, once its elements have been written by a bulk member.
  template <class _Rp>
  static constexpr std::ranges::iterator_t<_Rp> __end_of(_Rp& r) {
    if constexpr (std::ranges::common_range<_Rp>)
      return std::ranges::end(r);
    else
      return std::ranges::next(std::ranges::begin(r), std::ranges::end(r));
  }

  // Fills r with values of type T. `__one()` produces a value; `bulk(s)` fills a span<T>.
  template <class _Tp, class _Rp, class _One, class _Bulk>
  static constexpr std::ranges::iterator_t<_Rp> fill(_Rp& r, _One __one, _Bulk bulk) {
    constexpr bool __has_bulk = !std::is_same_v<_Bulk, std::nullptr_t>;
    auto __it = std::ranges::begin(r);
    if constexpr (__has_bulk && std::ranges::sized_range<_Rp>) {
      if constexpr (std::ranges::contiguous_range<_Rp> && std::is_same_v<std::ranges::range_value_t<_Rp>, _Tp> &&
                    std::is_same_v<std::ranges::range_reference_t<_Rp>, _Tp&>) {
        const size_t n = static_cast<size_t>(std::ranges::size(r));
        bulk(std::span<_Tp>(std::ranges::data(r), n));
        return __it + static_cast<std::ranges::range_difference_t<_Rp>>(n);
      } else {
        _Tp __buf[chunk]{};
        for (auto n = static_cast<size_t>(std::ranges::size(r)); n != 0;) {
          const size_t k = n < chunk ? n : chunk;
          bulk(std::span<_Tp>(__buf, k));
          for (size_t i = 0; i < k; ++i, ++__it)
            *__it = __buf[i];
          n -= k;
        }
        return __it;
      }
    } else {
      for (auto last = std::ranges::end(r); __it != last; ++__it)
        *__it = __one();
      return __it;
    }
  }

public:
  template <class _Rp, class _Gp>
    requires std::ranges::output_range<_Rp, std::invoke_result_t<_Gp&>> &&
             std::uniform_random_bit_generator<std::remove_cvref_t<_Gp>>
  constexpr std::ranges::borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Gp&& __g) const {
    using _Tp = std::invoke_result_t<_Gp&>;
    if constexpr (requires { __g.generate_random(static_cast<_Rp&&>(r)); }) {
      __g.generate_random(static_cast<_Rp&&>(r));
      return __end_of(r);
    } else if constexpr (requires(std::span<_Tp> s) { __g.generate_random(s); }) {
      return fill<_Tp>(r, [&__g] { return __g(); }, [&__g](std::span<_Tp> s) { __g.generate_random(s); });
    } else {
      return fill<_Tp>(r, [&__g] { return __g(); }, nullptr);
    }
  }
  template <class _Gp, std::output_iterator<std::invoke_result_t<_Gp&>> _Op, std::sentinel_for<_Op> _Sp>
    requires std::uniform_random_bit_generator<std::remove_cvref_t<_Gp>>
  constexpr _Op operator()(_Op first, _Sp last, _Gp&& __g) const {
    return (*this)(std::ranges::subrange<_Op, _Sp>(static_cast<_Op&&>(first), last), __g);
  }

  template <class _Rp, class _Gp, class _Dp>
    requires std::ranges::output_range<_Rp, std::invoke_result_t<_Dp&, _Gp&>> && std::invocable<_Dp&, _Gp&> &&
             std::uniform_random_bit_generator<std::remove_cvref_t<_Gp>> &&
             std::is_arithmetic_v<std::invoke_result_t<_Dp&, _Gp&>>
  constexpr std::ranges::borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Gp&& __g, _Dp&& d) const {
    using _Tp = std::invoke_result_t<_Dp&, _Gp&>;
    if constexpr (requires { d.generate_random(static_cast<_Rp&&>(r), __g); }) {
      d.generate_random(static_cast<_Rp&&>(r), __g);
      return __end_of(r);
    } else if constexpr (requires(std::span<_Tp> s) { d.generate_random(s, __g); }) {
      return fill<_Tp>(r, [&d, &__g] { return std::invoke(d, __g); }, [&d, &__g](std::span<_Tp> s) { d.generate_random(s, __g); });
    } else {
      return fill<_Tp>(r, [&d, &__g] { return std::invoke(d, __g); }, nullptr);
    }
  }
  template <class _Gp, class _Dp, std::output_iterator<std::invoke_result_t<_Dp&, _Gp&>> _Op, std::sentinel_for<_Op> _Sp>
    requires std::invocable<_Dp&, _Gp&> && std::uniform_random_bit_generator<std::remove_cvref_t<_Gp>> &&
             std::is_arithmetic_v<std::invoke_result_t<_Dp&, _Gp&>>
  constexpr _Op operator()(_Op first, _Sp last, _Gp&& __g, _Dp&& d) const {
    return (*this)(std::ranges::subrange<_Op, _Sp>(static_cast<_Op&&>(first), last), __g, d);
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__detail::__ranges_algo::__generate_random_fn generate_random{};
}} // namespace std::ranges
