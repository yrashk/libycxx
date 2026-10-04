// [rand.dist.pois.weibull]: x >= 0 with p(x | a, b) = (a/b) (x/b)^(a-1) exp(-(x/b)^a); the CDF is
// 1 - exp(-(x/b)^a), so the median is b (ln 2)^(1/a); mean b Gamma(1 + 1/a).
// [rand.dist.pois.extreme]: p(x | a, b) = (1/b) exp((a - x)/b - exp((a - x)/b)); the CDF is
// exp(-exp((a - x)/b)), mean a + b * 0.5772156649 (Euler's constant), variance (pi b)^2 / 6.
#include <random>
#include "check.hpp"
#include "moments.hpp"

template <class D>
double fraction_below(D d, double x, int n = 200000) {
  std::mt19937_64 g(43);
  int c = 0;
  for (int i = 0; i < n; ++i) c += double(d(g)) < x;
  return double(c) / n;
}

int main() {
  std::mt19937_64 g(5);
  const int N = 200000;
  {
    std::weibull_distribution<> d(1.0, 2.0);  // exponential with mean 2
    CHECK(d.a() == 1.0 && d.b() == 2.0 && d.min() == 0.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(s.lo >= 0);
    CHECK(rs::near(s.mean, 2.0, 0.03));
    CHECK(rs::near(s.var, 4.0, 0.2));
  }
  {
    std::weibull_distribution<> d(2.0, 1.0);  // mean sqrt(pi)/2, variance 1 - pi/4
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 0.8862269255, 0.01));
    CHECK(rs::near(s.var, 1 - 0.7853981634, 0.01));
  }
  // CDF at b: 1 - e^-1 for every a.
  CHECK(rs::near(fraction_below(std::weibull_distribution<>(3.0, 5.0), 5.0), 0.6321205588, 0.006));
  CHECK(rs::near(fraction_below(std::weibull_distribution<float>(0.5f, 1.0f), 1.0), 0.6321205588, 0.006));
  {
    std::extreme_value_distribution<> d(1.0, 2.0);
    CHECK(d.a() == 1.0 && d.b() == 2.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 1.0 + 2.0 * 0.5772156649, 0.04));
    CHECK(rs::near(s.var, 4.0 * 1.6449340668, 0.3));
    CHECK(d.min() <= s.lo && s.hi <= d.max());
  }
  // CDF at a: e^-1.
  CHECK(rs::near(fraction_below(std::extreme_value_distribution<>(-3.0, 0.5), -3.0), 0.3678794412, 0.006));
}
