// [rand.dist.samp.plinear]/12: piecewise_linear_distribution(nw, xmin, xmax, fw): "Let
// b_k = xmin + k*delta for k = 0, ..., n, and w_k = fw(b_k) for k = 0, ..., n." /13: "The number of
// invocations of fw does not exceed n+1." With fw(x) = x + 1 on [0, 2], nw = 2: w = {1, 2, 3},
// S = 1/2 ((1 + 2) + (2 + 3)) = 4, rho = {0.25, 0.5, 0.75}.
#include <random>
#include <vector>
#include "check.hpp"
#include "random_support.hpp"

int main() {
  int calls = 0;
  auto fw = [&calls](double x) { ++calls; return x + 1; };
  std::piecewise_linear_distribution<> d(2, 0.0, 2.0, fw);
  std::vector<double> iv = d.intervals(), dv = d.densities();
  CHECK(iv.size() == 3 && iv[0] == 0.0 && iv[1] == 1.0 && iv[2] == 2.0);
  CHECK(dv.size() >= 3);
  CHECK(rs::near(dv[0], 0.25, 1e-12) && rs::near(dv[1], 0.5, 1e-12) && rs::near(dv[2], 0.75, 1e-12));
  CHECK(calls <= 3);
  // nw = 0: n = 1, b = {xmin, xmax}, w = {fw(xmin), fw(xmax)}.
  calls = 0;
  std::piecewise_linear_distribution<> e(0, 1.0, 3.0, fw);  // w = {2, 4}, S = 6
  iv = e.intervals();
  dv = e.densities();
  CHECK(iv.size() == 2 && iv[0] == 1.0 && iv[1] == 3.0);
  CHECK(rs::near(dv[0], 1.0 / 3, 1e-12) && rs::near(dv[1], 2.0 / 3, 1e-12));
  CHECK(calls <= 2);
  using P = std::piecewise_linear_distribution<>::param_type;
  P p(2, 0.0, 2.0, [](double x) { return x + 1; });
  std::vector<double> pd = p.densities();
  CHECK(rs::near(pd[0], 0.25, 1e-12) && rs::near(pd[2], 0.75, 1e-12));
}
