// [rand.dist.samp.pconst]/11-12: piecewise_constant_distribution(nw, xmin, xmax, fw):
// "If nw = 0, let n = 1, otherwise let n = nw. The relation 0 < delta = (xmax - xmin)/n holds."
// "Let b_k = xmin + k*delta for k = 0, ..., n, and w_k = fw(b_k + delta/2) for k = 0, ..., n-1."
// So nw == 0 gives the single interval [xmin, xmax), not the default [0, 1).
#include <random>
#include <vector>
#include "check.hpp"

int main() {
  int calls = 0;
  auto fw = [&calls](double x) { ++calls; return x; };
  std::piecewise_constant_distribution<> d(0, 1.0, 3.0, fw);
  std::vector<double> iv = d.intervals(), dv = d.densities();
  CHECK(iv.size() == 2 && iv[0] == 1.0 && iv[1] == 3.0);
  CHECK(dv.size() == 1 && dv[0] == 0.5);  // rho_0 = w_0 / (S * 2) with S = w_0
  CHECK(calls <= 1);
  CHECK(d.min() == 1.0 && d.max() == 3.0);
  std::mt19937 g(1);
  for (int i = 0; i < 1000; ++i) {
    double x = d(g);
    CHECK(1.0 <= x && x < 3.0);
  }
}
