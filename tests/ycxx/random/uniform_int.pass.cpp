// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; sampling strategies can differ across implementations.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.uni.int]: produces integers i, a <= i <= b, with P(i | a, b) = 1 / (b - a + 1);
// min() == a and max() == b ([rand.req.dist]: glb and lub); a() and b() return the constructor
// arguments; d(g, p) uses p. Works for every IntType and any uniform random bit generator range.
#include <random>
#include <cstdint>
#include <limits>
#include "check.hpp"
#include "random_support.hpp"

template <class I, class G>
void range(G& g, I a, I b, int n = 20000) {
  std::uniform_int_distribution<I> d(a, b);
  CHECK(d.a() == a && d.b() == b && d.min() == a && d.max() == b);
  bool lo = false, hi = false;
  for (int i = 0; i < n; ++i) {
    I v = d(g);
    CHECK(a <= v && v <= b);
    lo = lo || v == a;
    hi = hi || v == b;
  }
  if (double(b) - double(a) < 100) CHECK(lo && hi);  // small ranges: both ends are hit
}

template <class G>
void frequencies(G& g) {
  std::uniform_int_distribution<> d(-3, 3);
  int count[7] = {};
  const int N = 140000;
  for (int i = 0; i < N; ++i) {
    int value = d(g);
    CHECK(-3 <= value && value <= 3);
    ++count[value + 3];
  }
  for (int c : count) CHECK(rs::near(c, N / 7.0, 0.04 * N / 7.0));
}

using lcg10 = std::linear_congruential_engine<std::uint32_t, 3u, 1u, 10u>;  // very small range

int main() {
  std::mt19937 g;
  std::minstd_rand m;  // range [1, 2^31 - 2], not a power of two
  std::mt19937_64 g64;
  std::ranlux24_base r24;  // 24-bit range
  std::independent_bits_engine<std::mt19937, 3, std::uint32_t> g3;  // values 0..7

  range<int>(g, 0, 9);
  range<int>(g, -5, 5);
  range<int>(g, 7, 7);
  range<int>(g, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  range<long long>(g, std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max());
  range<unsigned long long>(g, 0, ~0ull);
  range<unsigned long long>(g, ~0ull - 3, ~0ull);
  range<short>(g, -32768, 32767);
  range<signed char>(g, -128, 127);
  range<unsigned char>(g, 0, 255);
  range<unsigned short>(m, 1, 60000);
  range<long long>(m, -(1ll << 40), 1ll << 40);  // needs more than one engine call per value
  range<unsigned long long>(m, 0, ~0ull);
  range<std::int64_t>(r24, -1000000000000ll, 1000000000000ll);
  range<int>(g3, 0, 1000);
  range<int>(g3, 0, 7);
  range<int>(g3, 2, 4);
  range<std::uint64_t>(g64, 0, 1);

  frequencies(g);
  frequencies(m);
  frequencies(r24);
  frequencies(g3);

  // Full 64-bit range: both halves appear and values spread over all bits.
  std::uniform_int_distribution<unsigned long long> full;
  CHECK(full.a() == 0 && full.b() == ~0ull);
  unsigned long long all_or = 0, all_and = ~0ull;
  for (int i = 0; i < 1000; ++i) {
    unsigned long long v = full(m);
    all_or |= v;
    all_and &= v;
  }
  CHECK(all_or == ~0ull && all_and == 0);

  // d(g, p) uses p and leaves d's parameters alone.
  std::uniform_int_distribution<> d(0, 1);
  std::uniform_int_distribution<>::param_type p(100, 103);
  for (int i = 0; i < 1000; ++i) {
    int v = d(g, p);
    CHECK(100 <= v && v <= 103);
  }
  CHECK(d.a() == 0 && d.b() == 1);
  d.param(p);
  CHECK(d.a() == 100 && d.b() == 103 && d.min() == 100 && d.max() == 103);
}
