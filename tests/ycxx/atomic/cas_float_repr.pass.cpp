// [atomics.types.operations]/23: compare_exchange "compares the value representation";
// Note 6: "floating-point -0.0 and +0.0 will not compare equal with memcmp but will compare
// equal with operator==, and NaNs with the same payload will compare equal with memcmp but will
// not compare equal with operator==."
#include <atomic>
#include <cmath>
#include <limits>
#include "check.hpp"

int main() {
  static_assert(std::numeric_limits<double>::is_iec559);
  std::atomic<double> a(0.0);
  double e = -0.0;
  CHECK(!a.compare_exchange_strong(e, 1.0));
  CHECK(e == 0.0 && !std::signbit(e));
  CHECK(a.load() == 0.0);

  const double nan = std::numeric_limits<double>::quiet_NaN();
  a = nan;
  e = nan;
  CHECK(a.compare_exchange_strong(e, 2.0));
  CHECK(a.load() == 2.0);

  std::atomic<float> f(-0.0f);
  float fe = 0.0f;
  CHECK(!f.compare_exchange_strong(fe, 3.0f));
  CHECK(std::signbit(fe));
  CHECK(f.compare_exchange_strong(fe, 3.0f));
  CHECK(f.load() == 3.0f);
  return 0;
}
