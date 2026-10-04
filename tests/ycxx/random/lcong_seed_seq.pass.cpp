// [rand.eng.lcong]/6: E(q): "With k = ceil(log2(m) / 32) and a an array of length k + 3, invokes
// q.generate(a + 0, a + k + 3) and then computes S = (sum_{j<k} a_{j+3} * 2^(32j)) mod m. If
// c mod m is 0 and S is 0, sets the engine's state to 1, else sets the engine's state to S."
#include <random>
#include <cstdint>
#include <limits>
#include "check.hpp"
#include "random_support.hpp"

using u128 = unsigned __int128;

template <class E, int K>
void check_engine(std::uint32_t salt) {
  using T = typename E::result_type;
  const u128 M = E::modulus == 0 ? u128(std::numeric_limits<T>::max()) + 1 : u128(E::modulus);
  rs::pattern_seq q;
  q.salt = salt;
  E e(q);
  CHECK(q.calls == 1);
  CHECK(q.last_length == std::size_t(K + 3));
  u128 S = 0;
  for (int j = K - 1; j >= 0; --j) S = (S << 32) + rs::pattern_seq::mix(salt, std::uint32_t(j + 3));
  S %= M;
  E expect{T(S)};  // S < m, so E(S) has state S unless S == 0
  if (S != 0) CHECK(e == expect);

  rs::pattern_seq q2;
  q2.salt = salt;
  E f;
  f.seed(q2);
  CHECK(q2.calls == 1 && q2.last_length == std::size_t(K + 3));
  CHECK(f == e);

  // All-zero sequence: state 1 if c mod m == 0, else 0.
  rs::pattern_seq z;
  z.zeros = true;
  E ez(z);
  if (E::increment % M == 0) {
    E one(1);
    CHECK(ez == one);
  } else {
    E zero(0);
    CHECK(ez == zero);
  }

  // A seed_seq works too, and the result depends only on the sequence it generates.
  std::seed_seq s1{1, 2, 3}, s2{1, 2, 3};
  E g1(s1), g2(s2);
  CHECK(g1 == g2);
}

int main() {
  check_engine<std::minstd_rand0, 1>(1);                                          // m = 2^31 - 1
  check_engine<std::minstd_rand, 1>(2);
  check_engine<std::linear_congruential_engine<std::uint32_t, 1664525u, 1013904223u, 0u>, 1>(3);  // m = 2^32
  check_engine<std::linear_congruential_engine<std::uint64_t, 25214903917ull, 11u, 1ull << 48>, 2>(4);
  check_engine<std::linear_congruential_engine<std::uint64_t, 6364136223846793005ull, 1u, 0u>, 2>(5);  // 2^64
  check_engine<std::linear_congruential_engine<std::uint64_t, 3512401965023503517ull, 0u, (1ull << 63) - 25>, 2>(6);
  check_engine<std::linear_congruential_engine<unsigned short, 75u, 74u, 0u>, 1>(7);  // m = 2^16
  check_engine<std::linear_congruential_engine<std::uint64_t, 5u, 0u, (1ull << 32) + 15>, 2>(8);  // log2 m > 32
}
