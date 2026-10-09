// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.norm.chisq]: x > 0, chi-squared with n degrees of freedom (mean n, variance 2n).
// [rand.dist.norm.cauchy]: p(x | a, b) = (pi b (1 + ((x - a)/b)^2))^-1: median a, quartiles a +- b.
// [rand.dist.norm.f]: x >= 0, Fisher F with m and n degrees of freedom (mean n/(n-2) for n > 2).
// [rand.dist.norm.t]: Student's t with n degrees of freedom (symmetric, variance n/(n-2), n > 2).
#include <random>
#include "check.hpp"
#include "moments.hpp"

template <class D>
double fraction_below(D d, double x, int n = 200000) {
  std::mt19937_64 g(47);
  int c = 0;
  for (int i = 0; i < n; ++i) c += double(d(g)) < x;
  return double(c) / n;
}

int main() {
  std::mt19937_64 g(11);
  const int N = 200000;
  {
    std::chi_squared_distribution<> d(4.0);
    CHECK(d.n() == 4.0 && d.min() == 0.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(s.lo > 0);
    CHECK(rs::near(s.mean, 4.0, 0.04));
    CHECK(rs::near(s.var, 8.0, 0.4));
  }
  {
    std::chi_squared_distribution<> d(0.5);  // n need not be an integer
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 0.5, 0.01));
  }
  {
    std::cauchy_distribution<> d(3.0, 2.0);
    CHECK(d.a() == 3.0 && d.b() == 2.0);
    CHECK(rs::near(fraction_below(d, 3.0), 0.5, 0.006));
    CHECK(rs::near(fraction_below(d, 1.0), 0.25, 0.006));
    CHECK(rs::near(fraction_below(d, 5.0), 0.75, 0.006));
  }
  {
    std::fisher_f_distribution<> d(5.0, 10.0);
    CHECK(d.m() == 5.0 && d.n() == 10.0 && d.min() == 0.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(s.lo >= 0);
    CHECK(rs::near(s.mean, 1.25, 0.02));
    // F(m, n) with m == n has median 1.
    CHECK(rs::near(fraction_below(std::fisher_f_distribution<>(7.0, 7.0), 1.0), 0.5, 0.006));
  }
  {
    std::student_t_distribution<> d(6.0);
    CHECK(d.n() == 6.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 0.0, 0.02));
    CHECK(rs::near(s.var, 1.5, 0.1));
    CHECK(rs::near(fraction_below(d, 0.0), 0.5, 0.006));
    // n = 1 is the standard Cauchy distribution: P(x < 1) = 3/4.
    CHECK(rs::near(fraction_below(std::student_t_distribution<>(1.0), 1.0), 0.75, 0.006));
  }
}
