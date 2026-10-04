// [simd.math]: the <cmath> functions apply element-wise; return types deduced-vec-t<V>,
// rebind_t<int, ...> for ilogb/fpclassify, mask_type for the classification and comparison
// functions; abs for signed integers; the names are also declared in namespace std ([simd.syn]).
// The functions whose scalar counterparts are exact (floor, ceil, trunc, round, fabs, copysign,
// fmax, fmin, fmod, frexp, ldexp, ilogb, isnan, ...) are compared exactly; sqrt is checked on
// perfect squares.
#include <simd>
#include <cmath>
#include <limits>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using VD = simd::vec<double, 4>;
using VF = simd::vec<float, 3>;

int main() {
  VD x([](int i) { return -1.5 + i; });  // -1.5 -0.5 0.5 1.5
  auto fl = simd::floor(x);
  static_assert(std::is_same_v<decltype(fl), VD>);
  CHECK(fl[0] == -2 && fl[1] == -1 && fl[2] == 0 && fl[3] == 1);
  auto ce = std::ceil(x);  // found in namespace std as well
  CHECK(ce[0] == -1 && ce[3] == 2);
  auto tr = simd::trunc(x);
  CHECK(tr[0] == -1 && tr[3] == 1);
  auto ro = simd::round(x);  // halfway cases away from zero
  CHECK(ro[0] == -2 && ro[1] == -1 && ro[2] == 1 && ro[3] == 2);
  auto ab = simd::fabs(x);
  CHECK(ab[0] == 1.5 && ab[1] == 0.5);
  CHECK(simd::abs(x)[0] == 1.5);
  auto cs = simd::copysign(VD(2.0), x);
  CHECK(cs[0] == -2 && cs[3] == 2);
  auto mx = simd::fmax(x, VD(0.0));
  CHECK(mx[0] == 0 && mx[3] == 1.5);
  auto mn = simd::fmin(x, 0.0);  // mixed: deduced-vec-t parameter accepts a scalar
  CHECK(mn[0] == -1.5 && mn[3] == 0);
  auto md = simd::fmod(VD(7.5), VD(2.0));
  CHECK(md[0] == 1.5);
  CHECK(simd::sqrt(VD([](int i) { return double(i * i); }))[3] == 3.0);
  VF fsq([](int i) { return float(i * i * 4); });
  CHECK(simd::sqrt(fsq)[2] == 4.0f);

  simd::rebind_t<int, VD> ex;
  auto fr = simd::frexp(VD(8.0), &ex);
  CHECK(fr[0] == 0.5 && ex[0] == 4);
  auto ld = simd::ldexp(VD(0.75), simd::rebind_t<int, VD>(3));
  CHECK(ld[2] == 6.0);
  auto ib = simd::ilogb(VD(1024.0));
  static_assert(std::is_same_v<decltype(ib), simd::rebind_t<int, VD>>);
  CHECK(ib[1] == 10);
  VD ip;
  auto frac = simd::modf(x, &ip);
  CHECK(frac[0] == -0.5 && ip[0] == -1 && frac[3] == 0.5 && ip[3] == 1);

  const double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
  VD special([&](int i) { return i == 0 ? inf : i == 1 ? nan : i == 2 ? -0.0 : 1e-310; });
  auto isn = simd::isnan(special);
  static_assert(std::is_same_v<decltype(isn), VD::mask_type>);
  CHECK(!isn[0] && isn[1] && !isn[2]);
  CHECK(simd::isinf(special)[0] && !simd::isinf(special)[1]);
  CHECK(!simd::isfinite(special)[0] && simd::isfinite(special)[2]);
  CHECK(simd::signbit(special)[2] && !simd::signbit(special)[0]);
  CHECK(!simd::isnormal(special)[3] && !simd::isnormal(special)[2]);
  auto fc = simd::fpclassify(special);
  static_assert(std::is_same_v<decltype(fc), simd::rebind_t<int, VD>>);
  CHECK(fc[0] == FP_INFINITE && fc[1] == FP_NAN && fc[2] == FP_ZERO && fc[3] == FP_SUBNORMAL);
  CHECK(simd::isunordered(special, VD(0.0))[1] && !simd::isunordered(special, VD(0.0))[0]);
  CHECK(simd::isgreater(x, 0.0)[3] && !simd::isgreater(x, 0.0)[1] && simd::isless(x, 0.0)[1]);

  // approximate functions: check closeness
  auto e = simd::exp(VD(1.0));
  CHECK(std::fabs(e[0] - 2.718281828459045) < 1e-12);
  auto p = simd::pow(VD(2.0), VD(10.0));
  CHECK(std::fabs(p[1] - 1024.0) < 1e-9);
  auto s = simd::sin(VF(0.0f));
  CHECK(s[0] == 0.0f);
  auto h = simd::hypot(VD(3.0), VD(4.0));
  CHECK(std::fabs(h[0] - 5.0) < 1e-12);

  // integer abs
  simd::vec<int, 4> iv([](int i) { return 2 - 2 * i; });  // 2 0 -2 -4
  auto ia = simd::abs(iv);
  static_assert(std::is_same_v<decltype(ia), simd::vec<int, 4>>);
  CHECK(ia[0] == 2 && ia[2] == 2 && ia[3] == 4);

  return 0;
}
