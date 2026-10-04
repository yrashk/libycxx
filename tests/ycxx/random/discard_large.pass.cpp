// [rand.req.eng] Table 127, e.discard(z): "Advances e's state e_i to e_{i+z} by any means
// equivalent to z consecutive calls e()" (complexity: no worse than z calls), checked
//  - for every predefined engine and the adaptors with z crossing block/state boundaries and
//    z = 1000003, against z calls (equal state, ==, and equal subsequent values);
//  - for linear_congruential_engine with z up to 2^24 + 5, against the transition algorithm
//    [rand.eng.lcong]/1 (x_{i+1} = (a x_i + c) mod m; m == 0 means 2^w, /2), composed by repeated
//    squaring of the affine map (the complexity bound allows z calls, so z stays moderate);
//  - for philox_engine with z = 4 * 2^20 (+1, +3) against set_counter ([rand.eng.philox]/3: each
//    n calls advance the counter Z by one; /11: set_counter sets X from the counter and i = n - 1).
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <random>
#include "check.hpp"

template <class E>
void by_calls(unsigned long long z) {
  E a(4242u), b(4242u);
  a.discard(z);
  for (unsigned long long i = 0; i < z; ++i) b();
  CHECK(a == b);
  for (int i = 0; i < 10; ++i) CHECK(a() == b());
}

template <class E>
void boundaries(std::initializer_list<unsigned long long> zs) {
  for (unsigned long long z : zs) by_calls<E>(z);
  by_calls<E>(1000003);
}

using u128 = unsigned __int128;

template <class UInt, UInt a, UInt c, UInt m>
void lcg_jump() {
  using E = std::linear_congruential_engine<UInt, a, c, m>;
  const u128 M = m == 0 ? u128(1) << std::numeric_limits<UInt>::digits : u128(m);
  auto mulmod = [&](u128 x, u128 y) {  // x, y < M <= 2^64: compute (x * y) mod M without overflow
    u128 r = 0;
    x %= M;
    while (y) {
      if (y & 1) r = (r + x) % M;
      x = (x + x) % M;
      y >>= 1;
    }
    return r;
  };
  for (unsigned long long z : {0ull, 1ull, 5ull, 4096ull, (1ull << 24) + 5}) {
    // f(x) = A x + C; f^z by squaring
    u128 A = 1, C = 0, sa = a % M, sc = c % M;
    for (unsigned long long k = z; k; k >>= 1) {
      if (k & 1) {
        A = mulmod(sa, A);
        C = (mulmod(sa, C) + sc) % M;
      }
      sc = (mulmod(sa, sc) + sc) % M;
      sa = mulmod(sa, sa);
    }
    E e(12345u);
    e.discard(z);
    // the state after z steps from the seed state s: f^z(s); the next value is f(f^z(s))
    // s itself: [rand.eng.lcong]/5, seeding with s: x = s mod m, or 1 if that is 0 and c mod m is 0
    u128 s = u128(12345u) % M;
    if (s == 0 && u128(c) % M == 0) s = 1;
    u128 state = (mulmod(A, s) + C) % M;
    u128 next = (mulmod(u128(a) % M, state) + u128(c) % M) % M;
    CHECK(u128(e()) == next);
  }
}

int main() {
  boundaries<std::minstd_rand0>({0, 1, 2, 3});
  boundaries<std::minstd_rand>({0, 1, 2});
  boundaries<std::mt19937>({0, 1, 623, 624, 625, 1247, 1248, 1249});
  boundaries<std::mt19937_64>({311, 312, 313, 624});
  boundaries<std::ranlux24_base>({9, 10, 11, 23, 24, 25});
  boundaries<std::ranlux48_base>({4, 5, 6, 11, 12, 13});
  boundaries<std::ranlux24>({22, 23, 24, 222, 223, 224});
  boundaries<std::ranlux48>({10, 11, 12, 388, 389, 390});
  boundaries<std::knuth_b>({255, 256, 257});
  boundaries<std::philox4x32>({1, 2, 3, 4, 5, 7, 8, 9});
  boundaries<std::philox4x64>({3, 4, 5});
  boundaries<std::independent_bits_engine<std::mt19937, 64, std::uint64_t>>({311, 312, 313});
  boundaries<std::independent_bits_engine<std::minstd_rand, 13, std::uint16_t>>({1, 2, 3});
  boundaries<std::shuffle_order_engine<std::ranlux24_base, 5>>({4, 5, 6});
  boundaries<std::discard_block_engine<std::philox4x32, 5, 2>>({1, 2, 3, 4, 5, 6});

  lcg_jump<std::uint_fast32_t, 16807, 0, 2147483647>();
  lcg_jump<std::uint_fast32_t, 48271, 0, 2147483647>();
  lcg_jump<std::uint64_t, 6364136223846793005ull, 1442695040888963407ull, 0>();
  lcg_jump<std::uint32_t, 1664525u, 1013904223u, 0>();
  lcg_jump<std::uint64_t, 25214903917ull, 11ull, 1ull << 48>();
  lcg_jump<std::uint64_t, 3ull, 7ull, 1000000007ull>();
  lcg_jump<std::uint32_t, 5u, 3u, 16u>();

  for (unsigned long long extra : {0ull, 1ull, 3ull}) {
    std::philox4x32 a, b;
    a.discard(4 * (1ull << 20) + extra);
    b.set_counter({0, 0, 0, 1u << 20});  // Z = 2^20 = X_0 (c[n - 1])
    b.discard(extra);
    for (int i = 0; i < 9; ++i) CHECK(a() == b());
    std::philox4x64 c, d;
    c.discard(4 * (1ull << 20) + extra);
    d.set_counter({0, 0, 0, 1ull << 20});
    d.discard(extra);
    for (int i = 0; i < 9; ++i) CHECK(c() == d());
  }
  return 0;
}
