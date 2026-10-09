// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; sampling strategies can differ across implementations.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.pois.poisson]: integers i >= 0 with P(i | mu) = e^-mu mu^i / i!, 0 < mean; the mean
// and the variance are both mu; P(0) = e^-mu. min() is 0.
#include <random>
#include "check.hpp"
#include "random_support.hpp"

template <class I>
void moments(double mu) {
  std::mt19937_64 g(31);
  std::poisson_distribution<I> d(mu);
  CHECK(d.mean() == mu && d.min() == 0);
  const int N = 200000;
  double s = 0, s2 = 0;
  for (int i = 0; i < N; ++i) {
    I v = d(g);
    CHECK(v >= 0 && v <= d.max());
    s += double(v);
    s2 += double(v) * double(v);
  }
  double mean = s / N, var = s2 / N - mean * mean;
  CHECK(rs::near(mean, mu, 6 * rs::sqrtd(mu / N)));
  CHECK(rs::near(var, mu, 0.05 * mu));
}

int main() {
  moments<int>(0.05);
  moments<int>(1.0);
  moments<int>(4.5);
  moments<long>(12.0);  // above the threshold where implementations switch methods
  moments<unsigned>(250.0);
  moments<long long>(1e5);

  // P(0) = e^-2 = 0.1353352832.
  std::mt19937 g(8);
  std::poisson_distribution<> d(2.0);
  const int N = 200000;
  int zeros = 0;
  for (int i = 0; i < N; ++i) zeros += d(g) == 0;
  CHECK(rs::near(double(zeros) / N, 0.1353352832, 0.005));
}
