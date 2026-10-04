// libycxx core: ranges::generate_random ([alg.rand.generate], P1068).
//
// A generator's (or distribution's) own bulk member `generate_random(r)` (`generate_random(r, g)`)
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

namespace ycxx::detail::ranges_algo {

struct generate_random_fn {
private:
  static constexpr size_t chunk = 64;

  // The end iterator of r, once its elements have been written by a bulk member.
  template <class R>
  static constexpr std::ranges::iterator_t<R> end_of(R& r) {
    if constexpr (std::ranges::common_range<R>)
      return std::ranges::end(r);
    else
      return std::ranges::next(std::ranges::begin(r), std::ranges::end(r));
  }

  // Fills r with values of type T. `one()` produces a value; `bulk(s)` fills a span<T>.
  template <class T, class R, class One, class Bulk>
  static constexpr std::ranges::iterator_t<R> fill(R& r, One one, Bulk bulk) {
    constexpr bool has_bulk = !std::is_same_v<Bulk, std::nullptr_t>;
    auto it = std::ranges::begin(r);
    if constexpr (has_bulk && std::ranges::sized_range<R>) {
      if constexpr (std::ranges::contiguous_range<R> && std::is_same_v<std::ranges::range_value_t<R>, T> &&
                    std::is_same_v<std::ranges::range_reference_t<R>, T&>) {
        const size_t n = static_cast<size_t>(std::ranges::size(r));
        bulk(std::span<T>(std::ranges::data(r), n));
        return it + static_cast<std::ranges::range_difference_t<R>>(n);
      } else {
        T buf[chunk]{};
        for (auto n = static_cast<size_t>(std::ranges::size(r)); n != 0;) {
          const size_t k = n < chunk ? n : chunk;
          bulk(std::span<T>(buf, k));
          for (size_t i = 0; i < k; ++i, ++it)
            *it = buf[i];
          n -= k;
        }
        return it;
      }
    } else {
      for (auto last = std::ranges::end(r); it != last; ++it)
        *it = one();
      return it;
    }
  }

public:
  template <class R, class G>
    requires std::ranges::output_range<R, std::invoke_result_t<G&>> &&
             std::uniform_random_bit_generator<std::remove_cvref_t<G>>
  constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, G&& g) const {
    using T = std::invoke_result_t<G&>;
    if constexpr (requires { g.generate_random(static_cast<R&&>(r)); }) {
      g.generate_random(static_cast<R&&>(r));
      return end_of(r);
    } else if constexpr (requires(std::span<T> s) { g.generate_random(s); }) {
      return fill<T>(r, [&g] { return g(); }, [&g](std::span<T> s) { g.generate_random(s); });
    } else {
      return fill<T>(r, [&g] { return g(); }, nullptr);
    }
  }
  template <class G, std::output_iterator<std::invoke_result_t<G&>> O, std::sentinel_for<O> S>
    requires std::uniform_random_bit_generator<std::remove_cvref_t<G>>
  constexpr O operator()(O first, S last, G&& g) const {
    return (*this)(std::ranges::subrange<O, S>(static_cast<O&&>(first), last), g);
  }

  template <class R, class G, class D>
    requires std::ranges::output_range<R, std::invoke_result_t<D&, G&>> && std::invocable<D&, G&> &&
             std::uniform_random_bit_generator<std::remove_cvref_t<G>> &&
             std::is_arithmetic_v<std::invoke_result_t<D&, G&>>
  constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, G&& g, D&& d) const {
    using T = std::invoke_result_t<D&, G&>;
    if constexpr (requires { d.generate_random(static_cast<R&&>(r), g); }) {
      d.generate_random(static_cast<R&&>(r), g);
      return end_of(r);
    } else if constexpr (requires(std::span<T> s) { d.generate_random(s, g); }) {
      return fill<T>(r, [&d, &g] { return std::invoke(d, g); }, [&d, &g](std::span<T> s) { d.generate_random(s, g); });
    } else {
      return fill<T>(r, [&d, &g] { return std::invoke(d, g); }, nullptr);
    }
  }
  template <class G, class D, std::output_iterator<std::invoke_result_t<D&, G&>> O, std::sentinel_for<O> S>
    requires std::invocable<D&, G&> && std::uniform_random_bit_generator<std::remove_cvref_t<G>> &&
             std::is_arithmetic_v<std::invoke_result_t<D&, G&>>
  constexpr O operator()(O first, S last, G&& g, D&& d) const {
    return (*this)(std::ranges::subrange<O, S>(static_cast<O&&>(first), last), g, d);
  }
};

} // namespace ycxx::detail::ranges_algo

namespace std::ranges {
inline constexpr ycxx::detail::ranges_algo::generate_random_fn generate_random{};
} // namespace std::ranges
