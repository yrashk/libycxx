// [rand.dist.pois.exp]: x > 0 with p(x | lambda) = lambda e^(-lambda x) (mean 1/lambda,
// variance 1/lambda^2). [rand.dist.pois.gamma]: x > 0 with
// p(x | alpha, beta) = e^(-x/beta) / (beta^alpha Gamma(alpha)) x^(alpha-1) (mean alpha*beta,
// variance alpha*beta^2), for 0 < alpha and 0 < beta, including alpha < 1.
#include <random>
#include "check.hpp"
#include "moments.hpp"

template <class D>
void check(D d, double em, double ev, double rel = 0.06) {
  std::mt19937_64 g(41);
  const int N = 200000;
  sample_moments s = moments_of(d, g, N);
  CHECK(s.lo > 0);  // x > 0
  CHECK(rs::near(s.mean, em, 6 * rs::sqrtd(ev / N)));
  CHECK(rs::near(s.var, ev, rel * ev));
  CHECK(d.min() <= s.lo && s.hi <= d.max());
}

int main() {
  check(std::exponential_distribution<>(1.0), 1.0, 1.0);
  check(std::exponential_distribution<>(0.25), 4.0, 16.0);
  check(std::exponential_distribution<float>(10.0f), 0.1, 0.01);
  check(std::gamma_distribution<>(1.0, 1.0), 1.0, 1.0);
  check(std::gamma_distribution<>(0.3, 2.0), 0.6, 1.2, 0.1);
  check(std::gamma_distribution<>(2.5, 0.5), 1.25, 0.625);
  check(std::gamma_distribution<>(50.0, 3.0), 150.0, 450.0);
  check(std::gamma_distribution<long double>(4.0L, 1.0L), 4.0, 4.0);

  // Exponential: P(x > 1/lambda) = e^-1.
  std::mt19937 g(2);
  std::exponential_distribution<> e(3.0);
  CHECK(e.lambda() == 3.0);
  CHECK(e.min() == 0.0);
  const int N = 200000;
  int above = 0;
  for (int i = 0; i < N; ++i) above += e(g) > 1.0 / 3.0;
  CHECK(rs::near(double(above) / N, 0.36787944, 0.006));
  std::gamma_distribution<> ga(2.0, 3.0);
  CHECK(ga.alpha() == 2.0 && ga.beta() == 3.0 && ga.min() == 0.0);
}
