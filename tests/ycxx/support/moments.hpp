// Sample mean and variance of a distribution with a fixed engine seed (for [rand.dist] tests).
#pragma once
#include "check.hpp"
#include "random_support.hpp"

struct sample_moments {
  double mean, var, lo, hi;
};

template <class D, class G>
sample_moments moments_of(D& d, G& g, int n) {
  double s = 0, s2 = 0, lo = 1e300, hi = -1e300;
  for (int i = 0; i < n; ++i) {
    double v = double(d(g));
    CHECK(v == v);  // not NaN
    s += v;
    s2 += v * v;
    lo = v < lo ? v : lo;
    hi = v > hi ? v : hi;
  }
  double m = s / n;
  return {m, s2 / n - m * m, lo, hi};
}
