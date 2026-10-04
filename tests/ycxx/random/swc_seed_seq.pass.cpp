// [rand.eng.sub]/9: E(q): "With k = ceil(w/32) and a an array of length r*k, invokes
// q.generate(a+0, a+r*k) and then, iteratively for i = -r, ..., -1, sets X_i to
// (sum_{j<k} a_{k(i+r)+j} * 2^(32j)) mod m. If X_{-1} is then 0, sets c to 1; otherwise sets c to 0."
#include <random>
#include <cstdint>
#include "check.hpp"
#include "random_support.hpp"
#include "swc_reference.hpp"

using swc33 = std::subtract_with_carry_engine<std::uint64_t, 33, 4, 9>;

template <class E>
void check(std::size_t k, std::uint32_t salt) {
  rs::pattern_seq q1, q2;
  q1.salt = q2.salt = salt;
  E e(q1);
  ref_swc<E> ref(q2, 0);
  CHECK(q1.calls == 1);
  CHECK(q1.last_length == E::long_lag * k);
  for (int i = 0; i < 1000; ++i) CHECK(e() == ref());

  // All zeros: X_{-1} == 0, so the carry starts at 1 and the first value is m - 1.
  rs::pattern_seq z1, z2;
  z1.zeros = z2.zeros = true;
  E ez(z1);
  ref_swc<E> rz(z2, 0);
  CHECK(ez() == E::max());
  CHECK(rz() == E::max());
  for (int i = 0; i < 1000; ++i) CHECK(ez() == rz());

  E f;
  rs::pattern_seq q3;
  q3.salt = salt;
  f.seed(q3);
  rs::pattern_seq q4;
  q4.salt = salt;
  CHECK(f == E(q4));
}

int main() {
  check<std::ranlux24_base>(1, 21);
  check<std::ranlux48_base>(2, 22);
  check<swc33>(2, 24);
}
