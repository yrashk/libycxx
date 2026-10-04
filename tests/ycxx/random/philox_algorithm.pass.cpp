// [rand.eng.philox]: the counter-based philox_engine: K_0 = seed mod 2^w, X = 0, i = n - 1 (/9);
// each step increments i and, when i == n, computes Y = Philox(K, X), increments the counter Z and
// sets i = 0; the result is Y_i. Philox runs r rounds of V_j = X'_{f_n(j)} (f_2 = identity,
// f_4 = {2, 1, 0, 3}), X_{2k} = mulhi(V_{2k}, M_k) xor key_k^q xor V_{2k+1},
// X_{2k+1} = mullo(V_{2k}, M_k), key_k^q = (K_k + q*C_k) mod 2^w; consts are [M0, C0, M1, C1, ...].
#include <random>
#include <array>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

using u128 = unsigned __int128;

template <class E>
struct ref_philox {
  using T = typename E::result_type;
  static constexpr std::size_t w = E::word_size, n = E::word_count, r = E::round_count;
  static constexpr u128 mask = (u128(1) << w) - 1;
  u128 X[n] = {}, K[n / 2] = {}, Y[n] = {};
  std::size_t i = n - 1;

  explicit ref_philox(u128 value) { K[0] = value & mask; }
  template <class Q> ref_philox(Q& q, int) {
    constexpr std::size_t p = (w + 31) / 32;
    std::uint32_t a[n / 2 * p];
    q.generate(a + 0, a + n / 2 * p);
    for (std::size_t k = 0; k < n / 2; ++k) {
      u128 v = 0;
      for (std::size_t j = 0; j < p; ++j) v += u128(a[k * p + j]) << (32 * j);
      K[k] = v & mask;
    }
  }
  void set_counter(const std::array<T, n>& c) {
    for (std::size_t j = 0; j < n; ++j) X[j] = u128(c[n - 1 - j]) & mask;
    i = n - 1;
  }
  void philox() {
    u128 Xp[n];
    for (std::size_t j = 0; j < n; ++j) Xp[j] = X[j];
    for (std::size_t q = 0; q < r; ++q) {
      u128 V[n];
      for (std::size_t j = 0; j < n; ++j) {
        std::size_t f = n == 2 ? j : (j == 0 ? 2 : j == 2 ? 0 : j);
        V[j] = Xp[f];
      }
      for (std::size_t k = 0; k < n / 2; ++k) {
        u128 M = u128(E::multipliers[k]), C = u128(E::round_consts[k]);
        u128 key = (K[k] + u128(q) * C) & mask;
        u128 prod = V[2 * k] * M;  // both < 2^64
        Xp[2 * k] = ((prod >> w) & mask) ^ key ^ V[2 * k + 1];
        Xp[2 * k + 1] = prod & mask;
      }
    }
    for (std::size_t j = 0; j < n; ++j) Y[j] = Xp[j];
  }
  T operator()() {
    if (++i == n) {
      philox();
      for (std::size_t j = 0; j < n; ++j) {  // Z = Z + 1 (mod 2^(n*w))
        X[j] = (X[j] + 1) & mask;
        if (X[j] != 0) break;
      }
      i = 0;
    }
    return T(Y[i]);
  }
};

using p2x32 = std::philox_engine<std::uint32_t, 32, 2, 10, 0xD256D193, 0x9E3779B9>;
using p2x64 = std::philox_engine<std::uint64_t, 64, 2, 6, 0xD2B74407B1CE6E93, 0x9E3779B97F4A7C15>;
using p4x24 = std::philox_engine<std::uint32_t, 24, 4, 3, 0xD2511F, 0x9E3779, 0xCD9E8D, 0xBB67AE>;
using p2x40 = std::philox_engine<std::uint64_t, 40, 2, 1, 0xD256D19317ull, 0x9E3779B9ull>;

template <class E>
void compare(typename E::result_type seed, int count) {
  E e(seed);
  ref_philox<E> ref(seed);
  for (int k = 0; k < count; ++k) {
    auto v = e();
    CHECK(v == ref());
    CHECK(v <= E::max());
  }
}

static_assert(std::philox4x32::word_size == 32 && std::philox4x32::word_count == 4 &&
              std::philox4x32::round_count == 10);
static_assert(std::is_same_v<decltype(std::philox4x32::word_size), const std::size_t>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(std::philox4x32::multipliers)>,
                             std::array<std::philox4x32::result_type, 2>>);
static_assert(std::philox4x32::multipliers[0] == 0xCD9E8D57 && std::philox4x32::multipliers[1] == 0xD2511F53);
static_assert(std::philox4x32::round_consts[0] == 0x9E3779B9 && std::philox4x32::round_consts[1] == 0xBB67AE85);
static_assert(std::philox4x64::multipliers[1] == 0xD2E7470EE14C6C93);
static_assert(std::philox4x64::round_consts[1] == 0xBB67AE8584CAA73B);
static_assert(std::philox4x32::default_seed == 20111115u);
static_assert(std::is_same_v<decltype(std::philox4x32::default_seed), const std::philox4x32::result_type>);
static_assert(std::philox4x32::min() == 0 && std::philox4x32::max() == 0xffffffffu);
static_assert(std::philox4x64::max() == ~0ull);
static_assert(p4x24::max() == 0xffffffu && p2x40::max() == 0xffffffffffull);
static_assert(p2x32::multipliers.size() == 1);

int main() {
  compare<std::philox4x32>(20111115u, 2000);
  compare<std::philox4x32>(0, 2000);
  compare<std::philox4x64>(20111115u, 2000);
  compare<std::philox4x64>(~0ull, 2000);
  compare<p2x32>(7, 2000);
  compare<p2x64>(7, 2000);
  compare<p4x24>(0xffffffffu, 2000);  // the seed is reduced mod 2^24
  compare<p2x40>(~0ull, 2000);
  CHECK(std::philox4x32() == std::philox4x32(20111115u));
  CHECK(p4x24(0x1000005u) == p4x24(5u));

  // set_counter: X_j = c_{n-1-j} mod 2^w, i = n - 1.
  std::philox4x32 a(3);
  ref_philox<std::philox4x32> ra(3);
  std::array<std::uint_fast32_t, 4> c{1, 2, 3, 0xfffffffe};
  a.set_counter(c);
  ra.set_counter(c);
  for (int k = 0; k < 40; ++k) CHECK(a() == ra());
  // The counter wraps around modulo 2^(n*w).
  std::philox4x32 b(3), b0(3);
  b.set_counter({0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu});
  for (int k = 0; k < 4; ++k) b();
  for (int k = 0; k < 4; ++k) CHECK(b() == b0());
  // The counter increments in its lowest word, which is the last element given to set_counter.
  std::philox4x32 s1(3), s2(3);
  s1.discard(4);
  s2.set_counter({0, 0, 0, 1});
  for (int k = 0; k < 8; ++k) CHECK(s1() == s2());

  // Seed sequence: p = ceil(w/32), q.generate(a + 0, a + n/2 * p), K_k from a.
  rs::pattern_seq q1, q2;
  std::philox4x64 e(q1);
  ref_philox<std::philox4x64> re(q2, 0);
  CHECK(q1.calls == 1 && q1.last_length == 4);
  for (int k = 0; k < 100; ++k) CHECK(e() == re());
  rs::pattern_seq q3, q4;
  p4x24 f(q3);
  ref_philox<p4x24> rf(q4, 0);
  CHECK(q3.last_length == 2);
  for (int k = 0; k < 100; ++k) CHECK(f() == rf());
  rs::pattern_seq q5;
  p2x40 g(q5);
  CHECK(q5.last_length == 2);

  // seed(), seed(s), seed(q), discard, equality.
  std::philox4x32 h(9);
  h();
  h.seed();
  CHECK(h == std::philox4x32());
  h.seed(9);
  CHECK(h == std::philox4x32(9));
  rs::pattern_seq q6, q7;
  h.seed(q6);
  CHECK(h == std::philox4x32(q7));
  std::philox4x32 u, v;
  u.discard(10001);
  for (int k = 0; k < 10001; ++k) v();
  CHECK(u == v);
  CHECK(u() == v());
  v();
  CHECK(u != v);
  u();
  CHECK(u == v);
}
