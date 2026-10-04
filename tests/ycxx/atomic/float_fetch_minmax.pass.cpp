// [atomics.types.float]/9: fetch_fmaximum / fetch_fminimum act as fmaximum / fminimum (NaN
// propagates, -0 < +0); fetch_fmaximum_num / fetch_fminimum_num act as fmaximum_num /
// fminimum_num (a NaN operand is ignored when the other is a number); fetch_max / fetch_min act
// as fmaximum_num / fminimum_num for ordinary numbers. All return the previous value (/7).
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cmath>
#include <limits>
#include "check.hpp"

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::atomic<double> a(1.0);
  CHECK(a.fetch_max(3.0) == 1.0);
  CHECK(a.load() == 3.0);
  CHECK(a.fetch_max(2.0) == 3.0);
  CHECK(a.load() == 3.0);
  CHECK(a.fetch_min(-1.5) == 3.0);
  CHECK(a.load() == -1.5);
  CHECK(a.fetch_min(0.0) == -1.5);
  CHECK(a.load() == -1.5);

  // fmaximum / fminimum: NaN propagates; -0.0 is less than +0.0
  a = 0.0;
  CHECK(a.fetch_fminimum(-0.0) == 0.0);
  CHECK(a.load() == 0.0 && std::signbit(a.load()));
  CHECK(std::signbit(a.fetch_fmaximum(0.0)));
  CHECK(a.load() == 0.0 && !std::signbit(a.load()));
  a = 5.0;
  CHECK(a.fetch_fmaximum(nan) == 5.0);
  CHECK(std::isnan(a.load()));
  a = 5.0;
  a.fetch_fminimum(nan);
  CHECK(std::isnan(a.load()));

  // fmaximum_num / fminimum_num: a NaN operand is ignored
  a = 5.0;
  CHECK(a.fetch_fmaximum_num(nan) == 5.0);
  CHECK(a.load() == 5.0);
  CHECK(a.fetch_fminimum_num(nan) == 5.0);
  CHECK(a.load() == 5.0);
  a = nan;
  CHECK(std::isnan(a.fetch_fmaximum_num(2.0)));
  CHECK(a.load() == 2.0);
  CHECK(a.fetch_fminimum_num(7.0) == 2.0);
  CHECK(a.load() == 2.0);
  CHECK(a.fetch_fmaximum_num(7.0) == 2.0);
  CHECK(a.load() == 7.0);

  std::atomic<float> f(1.0f);
  CHECK(f.fetch_max(4.0f) == 1.0f);
  CHECK(f.fetch_min(-4.0f) == 4.0f);
  CHECK(f.load() == -4.0f);
  return 0;
}
