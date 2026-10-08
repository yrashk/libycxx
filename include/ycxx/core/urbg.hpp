// libycxx core: the uniform_random_bit_generator concept ([rand.req.urng]), which <random> and
// the shuffling and sampling algorithms share.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Gp>
concept uniform_random_bit_generator = invocable<_Gp&> && unsigned_integral<invoke_result_t<_Gp&>> && requires {
  { _Gp::min() } -> same_as<invoke_result_t<_Gp&>>;
  { _Gp::max() } -> same_as<invoke_result_t<_Gp&>>;
  requires bool_constant<(_Gp::min() < _Gp::max())>::value;
};

}} // namespace std
