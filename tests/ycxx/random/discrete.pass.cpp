// [rand.dist.samp.discrete]: integers 0 <= i < n with P(i) = p_i = w_i / S. The default
// constructor and an empty weight range give n = 1, p_0 = 1 (/3, /5); the (nw, xmin, xmax, fw)
// constructor uses w_k = fw(xmin + k*delta + delta/2), delta = (xmax - xmin)/n, with n = 1 and
// w_0 = 1 if nw == 0 (/9-10), and calls fw at most n times (/11); probabilities() returns the p_k.
#include <random>
#include <cstddef>
#include <vector>
#include "check.hpp"
#include "random_support.hpp"
#include "test_iterators.hpp"

template <class D>
void expect_probs(const D& d, std::initializer_list<double> want) {
  std::vector<double> p = d.probabilities();
  CHECK(p.size() == want.size());
  std::size_t i = 0;
  for (double w : want) CHECK(rs::near(p[i++], w, 1e-12));
  CHECK(d.min() == 0 && std::size_t(d.max()) == want.size() - 1);
}

int main() {
  std::discrete_distribution<> d{1.0, 2.0, 3.0, 4.0};
  expect_probs(d, {0.1, 0.2, 0.3, 0.4});
  std::mt19937 g(3);
  const int N = 200000;
  int c[4] = {};
  for (int i = 0; i < N; ++i) {
    int v = d(g);
    CHECK(0 <= v && v < 4);
    ++c[v];
  }
  for (int k = 0; k < 4; ++k) CHECK(rs::near(double(c[k]) / N, 0.1 * (k + 1), 0.006));

  // Zero weights are never produced.
  std::discrete_distribution<> z{0.0, 5.0, 0.0, 5.0};
  expect_probs(z, {0.0, 0.5, 0.0, 0.5});
  for (int i = 0; i < 10000; ++i) {
    int v = z(g);
    CHECK(v == 1 || v == 3);
  }

  // Defaults.
  std::discrete_distribution<> def;
  expect_probs(def, {1.0});
  for (int i = 0; i < 100; ++i) CHECK(def(g) == 0);
  const double none[1] = {0};
  std::discrete_distribution<> empty(none, none);
  expect_probs(empty, {1.0});
  std::discrete_distribution<> empty_il{};
  expect_probs(empty_il, {1.0});

  // Iterator constructor, with input iterators and a value type convertible to double.
  const int wi[3] = {2, 1, 1};
  std::discrete_distribution<long> it(InputIter<const int>(wi), InputIter<const int>(wi + 3));
  expect_probs(it, {0.5, 0.25, 0.25});

  // (nw, xmin, xmax, fw): delta = 2, w_k = fw(2k + 1) = 2k + 1, S = 16.
  int calls = 0;
  auto fw = [&calls](double x) { ++calls; return x; };
  std::discrete_distribution<> f(4, 0.0, 8.0, fw);
  expect_probs(f, {1.0 / 16, 3.0 / 16, 5.0 / 16, 7.0 / 16});
  CHECK(calls <= 4);
  calls = 0;
  std::discrete_distribution<> f0(0, 0.0, 1.0, fw);
  expect_probs(f0, {1.0});
  CHECK(calls <= 1);

  // param_type has the same constructors.
  using P = std::discrete_distribution<>::param_type;
  P p1{3.0, 1.0};
  std::vector<double> pp = p1.probabilities();
  CHECK(pp.size() == 2 && rs::near(pp[0], 0.75, 1e-12));
  P p2(wi, wi + 3);
  CHECK(std::discrete_distribution<>(p2).probabilities().size() == 3);
  P p3(2, 0.0, 2.0, [](double x) { return x; });  // w = {0.5, 1.5}
  CHECK(rs::near(p3.probabilities()[1], 0.75, 1e-12));
  for (int i = 0; i < 1000; ++i) {
    int v = def(g, p1);
    CHECK(v == 0 || v == 1);
  }
  def.param(p1);
  CHECK(def.max() == 1);
}
