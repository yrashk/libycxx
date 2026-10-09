// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; sampling strategies can differ across implementations.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.bern.geo]: integers i >= 0 with P(i | p) = p (1 - p)^i, 0 < p < 1 (mean (1-p)/p,
// variance (1-p)/p^2). [rand.dist.bern.negbin]: integers i >= 0 with
// P(i | k, p) = C(k + i - 1, i) p^k (1 - p)^i, 0 < p <= 1 and 0 < k (mean k(1-p)/p,
// variance k(1-p)/p^2). min() is 0 for both.
#include <random>
#include "check.hpp"
#include "random_support.hpp"

template <class D>
void moments(D d, double em, double ev) {
  std::mt19937_64 g(23);
  CHECK(d.min() == 0);
  const int N = 200000;
  double s = 0, s2 = 0;
  for (int i = 0; i < N; ++i) {
    auto v = d(g);
    CHECK(v >= 0 && v <= d.max());
    s += double(v);
    s2 += double(v) * double(v);
  }
  double mean = s / N, var = s2 / N - mean * mean;
  CHECK(rs::near(mean, em, 6 * rs::sqrtd(ev / N)));
  CHECK(rs::near(var, ev, 0.06 * ev));
}

int main() {
  moments(std::geometric_distribution<>(0.5), 1.0, 2.0);
  moments(std::geometric_distribution<>(0.1), 9.0, 90.0);
  moments(std::geometric_distribution<long long>(0.9), 1.0 / 9, 0.1 / 0.81);
  moments(std::negative_binomial_distribution<>(1, 0.5), 1.0, 2.0);  // k = 1 is geometric
  moments(std::negative_binomial_distribution<>(5, 0.3), 5 * 0.7 / 0.3, 5 * 0.7 / 0.09);
  moments(std::negative_binomial_distribution<long>(20, 0.8), 20 * 0.2 / 0.8, 20 * 0.2 / 0.64);

  // geometric: P(0) = p.
  std::mt19937 g(4);
  std::geometric_distribution<> geo(0.3);
  CHECK(geo.p() == 0.3);
  int zeros = 0;
  const int N = 200000;
  for (int i = 0; i < N; ++i) zeros += geo(g) == 0;
  CHECK(rs::near(double(zeros) / N, 0.3, 0.006));
}
