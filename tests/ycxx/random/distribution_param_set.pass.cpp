// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.req.dist] Table 128: d.param(p): "Postconditions: d.param() == p." and d(g): "With
// p = d.param(), the sequence of numbers returned by successive invocations with the same object g
// is randomly distributed according to the associated p(z | {p}) or P(z_i | {p}) function."
// So after d.param(p) a default-constructed distribution produces values distributed by p; here
// checked through the sample mean (fixed seed, tolerances of several standard errors).
// COUNTERPART: libstdcxx:26_numerics/random/[a-z_]+_distribution/cons/parms.cc
#include <random>
#include "check.hpp"
#include "random_support.hpp"

template <class D>
void check(const D& target, double mean, double tol) {
  D d;  // warm any cached state with default parameters, then switch
  std::mt19937_64 warm(17);
  (void)d(warm);
  d.param(target.param());
  CHECK(d.param() == target.param());
  std::mt19937_64 g(2024);
  const int N = 200000;
  double sum = 0;
  for (int i = 0; i < N; ++i) sum += double(d(g));
  CHECK(rs::near(sum / N, mean, tol));
  CHECK(d.param() == target.param());
  // The same through d(g, p) on a default-constructed distribution.
  D e;
  sum = 0;
  for (int i = 0; i < N; ++i) sum += double(e(g, target.param()));
  CHECK(rs::near(sum / N, mean, tol));
}

int main() {
  check(std::uniform_int_distribution<>(10, 20), 15.0, 0.05);
  check(std::uniform_real_distribution<>(10.0, 20.0), 15.0, 0.05);
  check(std::bernoulli_distribution(0.2), 0.2, 0.01);
  check(std::binomial_distribution<>(30, 0.3), 9.0, 0.05);
  check(std::geometric_distribution<>(0.25), 3.0, 0.05);                   // (1-p)/p
  check(std::negative_binomial_distribution<>(3, 0.4), 4.5, 0.06);         // k(1-p)/p
  check(std::poisson_distribution<>(7.5), 7.5, 0.05);
  check(std::exponential_distribution<>(4.0), 0.25, 0.005);
  check(std::gamma_distribution<>(3.0, 2.0), 6.0, 0.05);                   // alpha * beta
  check(std::weibull_distribution<>(1.0, 3.0), 3.0, 0.05);                 // b * Gamma(1 + 1/a)
  check(std::extreme_value_distribution<>(2.0, 1.0), 2.5772156649, 0.02);  // a + b * gamma_Euler
  check(std::normal_distribution<>(5.0, 2.0), 5.0, 0.03);
  check(std::lognormal_distribution<>(0.0, 0.5), 1.1331484531, 0.01);      // exp(m + s^2/2)
  check(std::chi_squared_distribution<>(5.0), 5.0, 0.05);
  check(std::fisher_f_distribution<>(6.0, 10.0), 1.25, 0.03);              // n / (n - 2)
  check(std::student_t_distribution<>(5.0), 0.0, 0.03);
  check(std::discrete_distribution<>{0.0, 1.0, 3.0}, 1.75, 0.01);
  const double b[] = {0.0, 1.0, 3.0}, wc[] = {0.0, 1.0};
  check(std::piecewise_constant_distribution<>(b, b + 3, wc), 2.0, 0.01);  // uniform on [1, 3)
}
