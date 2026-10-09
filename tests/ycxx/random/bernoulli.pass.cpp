// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; sampling strategies can differ across implementations.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.bern.bernoulli]: produces bool values with P(true) = p, P(false) = 1 - p
// (0 <= p <= 1); min() is false and max() is true.
#include <random>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

int main() {
  std::mt19937 g(5);
  std::bernoulli_distribution never(0.0), always(1.0);
  for (int i = 0; i < 10000; ++i) {
    CHECK(!never(g));
    CHECK(always(g));
  }
  std::bernoulli_distribution d(0.25);
  CHECK(d.p() == 0.25);
  CHECK(d.min() == false && d.max() == true);
  static_assert(std::is_same_v<decltype(d(g)), bool>);
  const int N = 200000;
  int t = 0;
  for (int i = 0; i < N; ++i) t += d(g);
  CHECK(rs::near(double(t) / N, 0.25, 0.006));
  std::bernoulli_distribution::param_type p(0.9);
  t = 0;
  for (int i = 0; i < N; ++i) t += d(g, p);
  CHECK(rs::near(double(t) / N, 0.9, 0.005));
  // Default p = 0.5.
  std::bernoulli_distribution half;
  t = 0;
  std::minstd_rand m(1);
  for (int i = 0; i < N; ++i) t += half(m);
  CHECK(rs::near(double(t) / N, 0.5, 0.006));
}
