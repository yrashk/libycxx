// [rand.dist.bern.negbin]/2: "Preconditions: 0 < p <= 1 and 0 < k." With p == 1,
// P(i | k, 1) = C(k + i - 1, i) * 1^k * 0^i is 1 for i == 0 and 0 otherwise, so every value is 0.
#include <random>
#include "check.hpp"

int main() {
  std::mt19937 g(4);
  std::negative_binomial_distribution<> one(4, 1.0);
  CHECK(one.k() == 4 && one.p() == 1.0);
  for (int i = 0; i < 1000; ++i) CHECK(one(g) == 0);
  std::negative_binomial_distribution<long> d;
  std::negative_binomial_distribution<long>::param_type p(7, 1.0);
  for (int i = 0; i < 1000; ++i) CHECK(d(g, p) == 0);
}
