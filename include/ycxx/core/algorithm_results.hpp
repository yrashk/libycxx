// libycxx core: algorithm result types ([algorithms.results]) shared by <algorithm> and the
// specialized <memory> algorithms.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace std::ranges {

template <class I, class O>
struct in_out_result {
  [[no_unique_address]] I in;
  [[no_unique_address]] O out;

  template <class I2, class O2>
    requires convertible_to<const I&, I2> && convertible_to<const O&, O2>
  constexpr operator in_out_result<I2, O2>() const& {
    return {in, out};
  }
  template <class I2, class O2>
    requires convertible_to<I, I2> && convertible_to<O, O2>
  constexpr operator in_out_result<I2, O2>() && {
    return {static_cast<I&&>(in), static_cast<O&&>(out)};
  }
};

} // namespace std::ranges
