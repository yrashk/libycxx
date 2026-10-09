// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.bern.bin]: integers 0 <= i (<= t) with P(i | t, p) = C(t, i) p^i (1 - p)^(t - i);
// preconditions 0 <= p <= 1 and 0 <= t. Mean t*p, variance t*p*(1-p).
#include <random>
#include "check.hpp"
#include "random_support.hpp"

template <class I>
void moments(I t, double p) {
  std::mt19937_64 g(17);
  std::binomial_distribution<I> d(t, p);
  CHECK(d.t() == t && d.p() == p);
  CHECK(d.min() == 0 && d.max() == t);
  const int N = 100000;
  double s = 0, s2 = 0;
  for (int i = 0; i < N; ++i) {
    I v = d(g);
    CHECK(0 <= v && v <= t);
    s += double(v);
    s2 += double(v) * double(v);
  }
  double mean = s / N, var = s2 / N - mean * mean;
  double em = double(t) * p, ev = double(t) * p * (1 - p);
  CHECK(rs::near(mean, em, 6 * rs::sqrtd(ev / N) + 1e-9));
  CHECK(rs::near(var, ev, 0.05 * ev + 1e-9));
}

int main() {
  std::mt19937 g(1);
  std::binomial_distribution<> zero_t(0, 0.5), p0(10, 0.0), p1(10, 1.0);
  for (int i = 0; i < 1000; ++i) {
    CHECK(zero_t(g) == 0);
    CHECK(p0(g) == 0);
    CHECK(p1(g) == 10);
  }
  moments<int>(1, 0.5);
  moments<int>(20, 0.3);
  moments<int>(20, 0.95);
  moments<long>(1000, 0.6);
  moments<long long>(100000, 0.001);
  moments<unsigned>(500, 0.5);

  // Exact small case: t = 2, p = 0.5: P(0) = P(2) = 1/4, P(1) = 1/2.
  std::binomial_distribution<> two(2, 0.5);
  int c[3] = {};
  const int N = 200000;
  for (int i = 0; i < N; ++i) {
    int value = two(g);
    CHECK(0 <= value && value <= 2);
    ++c[value];
  }
  CHECK(rs::near(c[0], N / 4.0, 0.03 * N / 4.0));
  CHECK(rs::near(c[1], N / 2.0, 0.03 * N / 2.0));
  CHECK(rs::near(c[2], N / 4.0, 0.03 * N / 4.0));
}
