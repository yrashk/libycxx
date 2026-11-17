// Statistical smoke test with chosen tolerances (fixed seed), as binomial.pass.cpp.
// [rand.dist.bern.bin]: P(i | t, p) = C(t, i) p^i (1 - p)^(t - i) also when p is so small that
// 1 - p rounds to 1 in double: t = 10^17, p = 10^-17 is close to Poisson(1), with
// P(0) = e^-1 = 0.368 and mean 1, not a distribution that is always 0.
#include <cstdint>
#include <random>
#include "check.hpp"
#include "random_support.hpp"

int main() {
  std::mt19937_64 g(5);
  std::binomial_distribution<long long> d(100000000000000000LL, 1e-17);
  const int N = 200000;
  int zeros = 0;
  double s = 0;
  for (int i = 0; i < N; ++i) {
    const long long v = d(g);
    CHECK(v >= 0);
    zeros += v == 0;
    s += double(v);
  }
  CHECK_SAY(rs::near(zeros / double(N), 0.36788, 0.01), "P(0) = %g", zeros / double(N));
  CHECK_SAY(rs::near(s / N, 1.0, 0.02), "mean = %g", s / N);
}
