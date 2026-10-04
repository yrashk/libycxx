// [rand.eng.mers]/8: E(q): "With k = ceil(w/32) and a an array of length n*k, invokes
// q.generate(a+0, a+n*k) and then, iteratively for i = -n, ..., -1, sets X_i to
// (sum_{j<k} a_{k(i+n)+j} * 2^(32j)) mod 2^w. Finally, if the most significant w-r bits of X_{-n}
// are zero, and if each of the other resulting X_i is 0, changes X_{-n} to 2^(w-1)."
#include <random>
#include <cstdint>
#include "check.hpp"
#include "random_support.hpp"
#include "mt_reference.hpp"

// Writes `first` to the first element and zeros elsewhere.
struct one_seq {
  using result_type = std::uint_least32_t;
  std::uint32_t first = 0;
  template <class RA> void generate(RA b, RA e) {
    for (bool f = true; b != e; ++b, f = false) *b = f ? first : 0u;
  }
};

using mt40 = std::mersenne_twister_engine<std::uint64_t, 40, 23, 11, 13, 0xab12cd34efull, 13, 0xfffffffffull, 7,
                                          0x5555555555ull, 15, 0x7777700000ull, 17, 1812433253ull>;

template <class E>
void check(std::size_t k, std::uint32_t salt) {
  rs::pattern_seq q1, q2;
  q1.salt = q2.salt = salt;
  E e(q1);
  ref_mt<E> ref(q2, 0);
  CHECK(q1.calls == 1);
  CHECK(q1.last_length == E::state_size * k);
  for (int i = 0; i < 1000; ++i) CHECK(e() == ref());

  rs::pattern_seq q3;
  q3.salt = salt;
  E f(0);
  f.seed(q3);
  CHECK(f == E(q2));  // q2 generates the same pattern again (pattern_seq is stateless)

  rs::pattern_seq z1, z2;
  z1.zeros = z2.zeros = true;
  E ez(z1);
  ref_mt<E> rz(z2, 0);
  for (int i = 0; i < 1000; ++i) CHECK(ez() == rz());
}

int main() {
  check<std::mt19937>(1, 11);
  check<std::mt19937_64>(2, 12);
  check<mt40>(2, 13);

  // Only the lower r bits of X_{-n} nonzero: still replaced by 2^(w-1).
  one_seq q;
  q.first = 1;  // bit 0 lies in the lower r = 31 bits
  std::mt19937 e(q);
  one_seq q0;
  q0.first = 0x80000000u;  // what the replacement produces
  std::mt19937 f(q0);
  CHECK(e == f);
  // A bit in the upper w - r bits: no replacement, so the state is X_{-n} = 0x80000000 as given...
  one_seq qh;
  qh.first = 0x80000000u;
  std::mt19937 g(qh);
  CHECK(g == f);
  // An all-zero sequence and a sequence with only low bits give the same engine.
  one_seq qz;
  std::mt19937 i(qz);
  CHECK(i == e);
}
