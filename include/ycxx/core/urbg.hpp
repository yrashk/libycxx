// libycxx core: the uniform_random_bit_generator concept ([rand.req.urng]), which <random> and
// the shuffling and sampling algorithms share.
#pragma once

#include <ycxx/core/concepts.hpp>

namespace std {

template <class G>
concept uniform_random_bit_generator = invocable<G&> && unsigned_integral<invoke_result_t<G&>> && requires {
  { G::min() } -> same_as<invoke_result_t<G&>>;
  { G::max() } -> same_as<invoke_result_t<G&>>;
  requires bool_constant<(G::min() < G::max())>::value;
};

} // namespace std
