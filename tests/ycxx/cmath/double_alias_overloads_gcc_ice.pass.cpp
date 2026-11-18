// Calls of abs ([c.math.abs]) and of the <cmath> functions ([cmath.syn]/2: one overload per
// floating-point type) with an argument whose type is an alias of double, through the global
// names of <stdlib.h> and <math.h> ([support.c.headers.other]/1), through using-declarations and
// using-directives, and qualified. On targets where long double has double's representation
// (macOS arm64), GCC 16.2 crashed (internal compiler error in compare_ics, cp/call.cc:12857)
// comparing the long double and _Float64 overloads for such calls (issue #1, found building
// HiGHS: `abs(a[0])` with `a` a `const std::vector<double>&`). libycxx declares the double
// overloads between the two (COMPILER-BUG(gcc) in ycxx/core/math_abs.hpp).
#include <cmath>
#include <cstdlib>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "check.hpp"

using D = double;
struct S {
  using T = double;
  T v;
};

double global_abs(const std::vector<double>& a) { return abs(a[0]) + abs(a.front()); }
double global_abs_member(const S& s) { return abs(s.v); }
double global_math(D x) { return sqrt(x) + fabs(x) + fmax(x, x); }
double using_decl(D x) {
  using std::abs;
  using std::sqrt;
  using std::fmax;
  using std::pow;
  using std::hypot;
  using std::fma;
  return abs(x) + sqrt(x) + fmax(x, x) + pow(x, x) + hypot(x, x) + hypot(x, x, x) + fma(x, x, x);
}
bool using_decl_classify(D x) {
  using std::isnan;
  using std::signbit;
  using std::isless;
  return isnan(x) || signbit(x) || isless(x, D(0));
}
double using_decl_out_params(D x, int& e) {
  using std::frexp;
  using std::modf;
  D ip;
  return frexp(x, &e) + modf(x, &ip);
}
double using_directive(D x) {
  using namespace std;
  return abs(x) + sqrt(x);
}
double qualified(D x) { return std::abs(x) + std::sqrt(x) + std::fmax(x, x); }
template <class T>
T dependent(T x) {
  using std::abs;
  using std::sqrt;
  return abs(x) + sqrt(x);
}

int main() {
  const std::vector<double> a{-4.0};
  CHECK(global_abs(a) == 8.0);
  CHECK(global_abs_member(S{-2.5}) == 2.5);
  CHECK(global_math(4.0) == 10.0);
  CHECK(using_decl(1.0) == 1.0 + 1.0 + 1.0 + 1.0 + std::hypot(1.0, 1.0) + std::hypot(1.0, 1.0, 1.0) + 2.0);
  CHECK(!using_decl_classify(1.0));
  int e = 0;
  CHECK(using_decl_out_params(8.0, e) == 0.5);
  CHECK(e == 4);
  CHECK(using_directive(4.0) == 6.0);
  CHECK(qualified(4.0) == 10.0);
  CHECK(dependent<D>(4.0) == 6.0);
}
