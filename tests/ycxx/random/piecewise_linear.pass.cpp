// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; distribution/shuffle algorithms are implementation-defined.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [rand.dist.samp.plinear]: x in [b_0, b_n) with density linear between rho_i at b_i and
// rho_{i+1} at b_{i+1}; rho_k = w_k / S with S = 1/2 sum (w_k + w_{k+1})(b_{k+1} - b_k).
// Defaults n = 1, rho_0 = rho_1 = 1, b = {0, 1}; the initializer_list form uses w_k = fw(b_k);
// the (nw, xmin, xmax, fw) form b_k = xmin + k*delta, w_k = fw(b_k), at most n + 1 calls of fw.
#include <random>
#include <vector>
#include "check.hpp"
#include "random_support.hpp"

template <class D>
void expect(const D& d, std::initializer_list<double> b, std::initializer_list<double> rho) {
  std::vector<double> iv = d.intervals(), dv = d.densities();
  CHECK(iv.size() == b.size());
  CHECK(dv.size() == rho.size());  // [rand.dist.samp.plinear]: exactly rho_0 ... rho_n
  std::size_t i = 0;
  for (double x : b) CHECK(rs::near(iv[i++], x, 1e-12));
  i = 0;
  for (double x : rho) CHECK(rs::near(dv[i++], x, 1e-12));
  CHECK(d.min() == *b.begin() && d.max() == *(b.end() - 1));
}

int main() {
  // S = 1/2 ((1 + 2) * 1 + (2 + 0) * 2) = 3.5.
  const double b[] = {0.0, 1.0, 3.0}, w[] = {1.0, 2.0, 0.0};
  std::piecewise_linear_distribution<> d(b, b + 3, w);
  expect(d, {0.0, 1.0, 3.0}, {2.0 / 7, 4.0 / 7, 0.0});
  std::mt19937 g(13);
  const int N = 200000;
  int below1 = 0;
  for (int i = 0; i < N; ++i) {
    double x = d(g);
    CHECK(0.0 <= x && x < 3.0);
    below1 += x < 1.0;
  }
  CHECK(rs::near(double(below1) / N, 1.5 / 3.5, 0.006));  // mass of [0, 1): (1 + 2)/2 / S

  // Triangular density 2x on [0, 1): mean 2/3, P(x < 1/2) = 1/4.
  const double tb[] = {0.0, 1.0}, tw[] = {0.0, 1.0};
  std::piecewise_linear_distribution<> t(tb, tb + 2, tw);
  expect(t, {0.0, 1.0}, {0.0, 2.0});
  double sum = 0;
  int below_half = 0;
  for (int i = 0; i < N; ++i) {
    double x = t(g);
    sum += x;
    below_half += x < 0.5;
  }
  CHECK(rs::near(sum / N, 2.0 / 3, 0.005));
  CHECK(rs::near(double(below_half) / N, 0.25, 0.006));

  // Defaults.
  expect(std::piecewise_linear_distribution<>(), {0.0, 1.0}, {1.0, 1.0});
  expect(std::piecewise_linear_distribution<>(b, b, w), {0.0, 1.0}, {1.0, 1.0});
  expect(std::piecewise_linear_distribution<>(b, b + 1, w), {0.0, 1.0}, {1.0, 1.0});
  expect(std::piecewise_linear_distribution<>({2.0}, [](double) { return 3.0; }), {0.0, 1.0}, {1.0, 1.0});

  // initializer_list: w = {1, 2, 3} at b = {0, 1, 2}; S = 4.
  int calls = 0;
  auto fw = [&calls](double x) { ++calls; return x + 1; };
  expect(std::piecewise_linear_distribution<>({0.0, 1.0, 2.0}, fw), {0.0, 1.0, 2.0}, {0.25, 0.5, 0.75});
  CHECK(calls <= 3);

  using P = std::piecewise_linear_distribution<>::param_type;
  P p(tb, tb + 2, tw);
  for (int i = 0; i < 1000; ++i) {
    double x = d(g, p);
    CHECK(0.0 <= x && x < 1.0);
  }
}
