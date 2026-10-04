// [rand.eng.mers]/4: the relations "r <= w" (so r == 0 and r == w are allowed),
// "u <= w", "s <= w", "t <= w", "l <= w" and "2u < w" are the only constraints; the
// algorithm of /2-3 must work at those limits, including w equal to the width of UIntType.
#include <random>
#include <cstdint>
#include "check.hpp"
#include "mt_reference.hpp"

// r == w: Y is the lower w bits of X_{i+1-n}.
using r_eq_w = std::mersenne_twister_engine<std::uint32_t, 32, 13, 7, 32, 0x9908b0df, 11, 0xffffffff, 7,
                                            0x9d2c5680, 15, 0xefc60000, 18, 1812433253>;
// r == 0: Y is the upper w bits of X_{i-n}.
using r_0 = std::mersenne_twister_engine<std::uint32_t, 32, 13, 7, 0, 0x9908b0df, 11, 0xffffffff, 7,
                                         0x9d2c5680, 15, 0xefc60000, 18, 1812433253>;
// s == t == l == w (the shifted-out values vanish), u at its largest (2u < w).
using big_shifts = std::mersenne_twister_engine<std::uint64_t, 64, 7, 3, 31, 0xb5026f5aa96619e9ull, 31,
                                                0x5555555555555555ull, 64, 0x71d67fffeda60000ull, 64,
                                                0xfff7eee000000000ull, 64, 6364136223846793005ull>;
// A word narrower than the type: w = 3 is the smallest w with 2u < w for u = 1.
using tiny = std::mersenne_twister_engine<std::uint16_t, 3, 5, 2, 1, 5, 1, 7, 1, 3, 2, 6, 1, 5>;

template <class E>
void compare(typename E::result_type seed, int count) {
  E e(seed);
  ref_mt<E> ref(seed);
  for (int i = 0; i < count; ++i) CHECK(e() == ref());
}

int main() {
  compare<r_eq_w>(5489, 500);
  compare<r_0>(5489, 500);
  compare<big_shifts>(123, 500);
  compare<tiny>(6, 200);
  static_assert(tiny::max() == 7);
  tiny t;
  for (int i = 0; i < 100; ++i) CHECK(t() <= 7);
}
