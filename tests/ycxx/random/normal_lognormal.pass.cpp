// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.norm.normal]: p(x | mu, sigma) = exp(-(x - mu)^2 / (2 sigma^2)) / (sigma sqrt(2 pi));
// mean and stddev are the parameters. [rand.dist.norm.lognormal]: x > 0 with ln x normal with
// parameters m and s (mean exp(m + s^2/2)). [rand.req.dist]: after reset(), "Subsequent uses of d
// do not depend on values produced by any engine prior to invoking reset."
#include <random>
#include "check.hpp"
#include "moments.hpp"

int main() {
  std::mt19937_64 g(7);
  const int N = 200000;
  {
    std::normal_distribution<> d;
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 0.0, 0.015));
    CHECK(rs::near(s.var, 1.0, 0.02));
    CHECK(s.lo < -3.5 && s.hi > 3.5);  // tails are produced
  }
  {
    std::normal_distribution<> d(10.0, 3.0);
    CHECK(d.mean() == 10.0 && d.stddev() == 3.0);
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, 10.0, 0.05));
    CHECK(rs::near(s.var, 9.0, 0.2));
    // Within one standard deviation: 68.27%.
    int in = 0;
    for (int i = 0; i < N; ++i) {
      double v = d(g);
      in += v > 7.0 && v < 13.0;
    }
    CHECK(rs::near(double(in) / N, 0.6826894921, 0.006));
  }
  {
    std::normal_distribution<float> d(-2.0f, 0.5f);
    sample_moments s = moments_of(d, g, N);
    CHECK(rs::near(s.mean, -2.0, 0.01));
    CHECK(rs::near(s.var, 0.25, 0.01));
  }
  {
    std::lognormal_distribution<> d(0.5, 0.25);
    CHECK(d.m() == 0.5 && d.s() == 0.25);
    sample_moments s = moments_of(d, g, N);
    CHECK(s.lo > 0);
    CHECK(rs::near(s.mean, 1.6990442448, 0.01));  // exp(0.5 + 0.03125)
    // The median is exp(m).
    int below = 0;
    for (int i = 0; i < N; ++i) below += d(g) < 1.6487212707;
    CHECK(rs::near(double(below) / N, 0.5, 0.006));
  }
  {  // reset(): a cached value from before the reset is not used.
    std::normal_distribution<> a(0.0, 1.0), b(0.0, 1.0);
    std::mt19937 noise(1);
    (void)b(noise);
    b.reset();
    std::mt19937 g1(9), g2(9);
    for (int i = 0; i < 100; ++i) CHECK(a(g1) == b(g2));
  }
}
