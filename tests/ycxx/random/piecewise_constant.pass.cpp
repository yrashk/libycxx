// [rand.dist.samp.pconst]: x in [b_0, b_n), uniform within each [b_i, b_{i+1}) with density
// rho_k = w_k / (S (b_{k+1} - b_k)), S = sum w_k. Defaults n = 1, rho_0 = 1, b = {0, 1} (/3, and
// when firstB == lastB, ++firstB == lastB, or bl.size() < 2); the initializer_list form uses
// w_k = fw((b_{k+1} + b_k)/2); the (nw, xmin, xmax, fw) form uses b_k = xmin + k*delta,
// w_k = fw(b_k + delta/2). intervals() returns the b_k, densities() the rho_k.
#include <random>
#include <vector>
#include "check.hpp"
#include "random_support.hpp"

template <class D>
void expect(const D& d, std::initializer_list<double> b, std::initializer_list<double> rho) {
  std::vector<double> iv = d.intervals(), dv = d.densities();
  CHECK(iv.size() == b.size() && dv.size() == rho.size());
  std::size_t i = 0;
  for (double x : b) CHECK(rs::near(iv[i++], x, 1e-12));
  i = 0;
  for (double x : rho) CHECK(rs::near(dv[i++], x, 1e-12));
  CHECK(d.min() == *b.begin() && d.max() == *(b.end() - 1));
}

int main() {
  const double b[] = {0.0, 1.0, 3.0}, w[] = {1.0, 1.0};
  std::piecewise_constant_distribution<> d(b, b + 3, w);
  expect(d, {0.0, 1.0, 3.0}, {0.5, 0.25});
  std::mt19937 g(9);
  const int N = 200000;
  int below1 = 0, below2 = 0;
  for (int i = 0; i < N; ++i) {
    double x = d(g);
    CHECK(0.0 <= x && x < 3.0);
    below1 += x < 1.0;
    below2 += x < 2.0;
  }
  CHECK(rs::near(double(below1) / N, 0.5, 0.006));
  CHECK(rs::near(double(below2) / N, 0.75, 0.006));

  // Zero weight: that interval is never produced.
  const double wz[] = {0.0, 3.0};
  std::piecewise_constant_distribution<> z(b, b + 3, wz);
  expect(z, {0.0, 1.0, 3.0}, {0.0, 0.5});
  for (int i = 0; i < 10000; ++i) {
    double x = z(g);
    CHECK(1.0 <= x && x < 3.0);
  }

  // Defaults.
  expect(std::piecewise_constant_distribution<>(), {0.0, 1.0}, {1.0});
  expect(std::piecewise_constant_distribution<>(b, b, w), {0.0, 1.0}, {1.0});
  expect(std::piecewise_constant_distribution<>(b, b + 1, w), {0.0, 1.0}, {1.0});
  auto id = [](double x) { return x; };
  expect(std::piecewise_constant_distribution<>({5.0}, id), {0.0, 1.0}, {1.0});

  // initializer_list: w = {fw(1), fw(4)} = {1, 4}, S = 5, rho = {1/(5*2), 4/(5*4)}.
  int calls = 0;
  auto fw = [&calls](double x) { ++calls; return x; };
  expect(std::piecewise_constant_distribution<>({0.0, 2.0, 6.0}, fw), {0.0, 2.0, 6.0}, {0.1, 0.2});
  CHECK(calls <= 2);
  // (nw, xmin, xmax, fw): b = {0, 2, 4}, w = {fw(1), fw(3)} = {1, 3}, S = 4.
  calls = 0;
  expect(std::piecewise_constant_distribution<>(2, 0.0, 4.0, fw), {0.0, 2.0, 4.0}, {0.125, 0.375});
  CHECK(calls <= 2);

  // param_type.
  using P = std::piecewise_constant_distribution<>::param_type;
  P p(b, b + 3, wz);
  for (int i = 0; i < 1000; ++i) {
    double x = d(g, p);
    CHECK(1.0 <= x && x < 3.0);
  }
  std::piecewise_constant_distribution<float> f({0.0f, 0.5f, 1.0f}, [](double) { return 1.0; });
  for (int i = 0; i < 1000; ++i) {
    float x = f(g);
    CHECK(0.0f <= x && x < 1.0f);
  }
}
