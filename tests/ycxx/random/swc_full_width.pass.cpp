// [rand.eng.sub]/3: "Let Y = X_{i-s} - X_{i-r} - c. Set X_i to y = Y mod m. Set c to 1 if Y < 0,
// otherwise set c to 0." with m = 2^w and w == numeric_limits<UIntType>::digits allowed (/5).
// With X_{i-s} == X_{i-r} == m - 1 and c == 1, Y = -1: X_i = m - 1 and c = 1.
// Seeding from an all-zero seed sequence (/9) gives c = 1 and reaches that case at i = 23.
#include <random>
#include <cstdint>
#include "check.hpp"
#include "random_support.hpp"
#include "swc_reference.hpp"

using swc64 = std::subtract_with_carry_engine<std::uint64_t, 64, 7, 19>;
using swc32 = std::subtract_with_carry_engine<std::uint32_t, 32, 7, 19>;

template <class E>
void check() {
  constexpr auto M1 = E::max();
  rs::pattern_seq z1, z2;
  z1.zeros = z2.zeros = true;
  E e(z1);
  ref_swc<E> ref(z2, 0);
  // Hand-computed from /3 for s = 7, r = 19, all X = 0, c = 1:
  //   X_0..X_6 = m-1 (c=1), X_7 = m-2 (c=0), X_8..X_13 = m-1, X_14 = m-2, X_15..X_18 = m-1,
  //   X_19 = X_20 = 0, X_21 = X_22 = X_23 = m-1 (c = 1 throughout).
  typename E::result_type expect[24];
  for (int i = 0; i < 24; ++i) expect[i] = M1;
  expect[7] = expect[14] = M1 - 1;
  expect[19] = expect[20] = 0;
  for (int i = 0; i < 24; ++i) {
    auto v = e();
    CHECK(v == expect[i]);
    CHECK(v == ref());
  }
  for (int i = 0; i < 2000; ++i) CHECK(e() == ref());

  rs::pattern_seq q1, q2;
  q1.salt = q2.salt = 23;
  E f(q1);
  ref_swc<E> rf(q2, 0);
  for (int i = 0; i < 2000; ++i) CHECK(f() == rf());
}

int main() {
  check<swc32>();
  check<swc64>();
}
