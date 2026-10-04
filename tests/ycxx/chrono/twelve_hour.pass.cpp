// [time.12]: is_am(h) is 0h <= h <= 11h; is_pm(h) is 12h <= h <= 23h; make12(h) is the 12-hour
// equivalent in [1h, 12h]; make24(h, is_pm) is the 24-hour equivalent in [0h, 11h] (am) or
// [12h, 23h] (pm). All constexpr and noexcept.
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

static_assert(noexcept(is_am(hours())) && noexcept(is_pm(hours())) && noexcept(make12(hours())) &&
              noexcept(make24(hours(), true)));

constexpr bool test() {
  for (int h = 0; h < 24; ++h) {
    if (is_am(hours(h)) != (h < 12) || is_pm(hours(h)) != (h >= 12)) return false;
    hours h12 = make12(hours(h));
    if (h12 < hours(1) || h12 > hours(12)) return false;
    if (h12 != hours(h % 12 == 0 ? 12 : h % 12)) return false;
    if (make24(h12, h >= 12) != hours(h)) return false;
  }
  if (is_am(hours(-1)) || is_pm(hours(24)) || is_am(hours(24))) return false;
  if (make24(hours(12), false) != hours(0) || make24(hours(12), true) != hours(12)) return false;
  if (make24(hours(1), true) != hours(13) || make24(hours(11), false) != hours(11)) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
