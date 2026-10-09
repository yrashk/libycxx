// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.uni.real]: produces x with a <= x < b, p(x | a, b) = 1 / (b - a); min() == a,
// max() == b; a() and b() return the constructor arguments; d(g, p) uses p.
// COUNTERPART: libstdcxx:26_numerics/random/uniform_real_distribution/operators/(64351|gencanon).cc
#include <random>
#include <cfloat>
#include <limits>
#include "check.hpp"
#include "random_support.hpp"

template <class R, class G>
void range(G& g, R a, R b, int n = 20000) {
  std::uniform_real_distribution<R> d(a, b);
  CHECK(d.a() == a && d.b() == b && d.min() == a && d.max() == b);
  double sum = 0;
  for (int i = 0; i < n; ++i) {
    R v = d(g);
    CHECK(a <= v && v < b);
    sum += double(v) / n;  // scaled: no overflow near DBL_MAX
  }
  double mid = double(a) / 2 + double(b) / 2;
  CHECK(rs::near(sum, mid, (double(b) - double(a)) * 0.02));
}

int main() {
  std::mt19937 g(1);
  std::minstd_rand m(2);
  std::mt19937_64 g64(3);
  range<double>(g, 0.0, 1.0);
  range<double>(g, -10.0, 10.0);
  range<double>(m, 1e6, 1e6 + 1.0);
  range<double>(g64, -1e-300, 1e-300);
  range<float>(g, 0.0f, 1.0f);
  range<float>(g, 1.0f, 2.0f);
  range<float>(m, -3.0f, -2.5f);
  range<long double>(g64, 0.0L, 1.0L);
  range<double>(g, -DBL_MAX / 2, DBL_MAX / 2);  // b - a <= numeric_limits<double>::max()

  // Distribution: ten equal bins.
  std::uniform_real_distribution<> d(2.0, 7.0);
  int bins[10] = {};
  const int N = 200000;
  for (int i = 0; i < N; ++i) {
    double value = d(g);
    CHECK(2.0 <= value && value < 7.0);
    int bin = int((value - 2.0) * 2.0);
    CHECK(0 <= bin && bin < 10);
    ++bins[bin];
  }
  for (int c : bins) CHECK(rs::near(c, N / 10.0, 0.03 * N / 10.0));

  std::uniform_real_distribution<>::param_type p(-1.0, -0.5);
  for (int i = 0; i < 1000; ++i) {
    double v = d(g, p);
    CHECK(-1.0 <= v && v < -0.5);
  }
  CHECK(d.a() == 2.0 && d.b() == 7.0);
}
