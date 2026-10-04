// [time.cal.wdidx]: weekday_indexed(wd, index); weekday(), index(); ok() is
// wd_.ok() && 1 <= index_ && index_ <= 5; ==. [time.cal.wdlast]: explicit weekday_last(wd);
// weekday(); ok() is wd_.ok(); ==.
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_trivially_default_constructible_v<weekday_indexed>);
static_assert(!std::is_convertible_v<weekday, weekday_last>);
static_assert(std::is_nothrow_constructible_v<weekday_indexed, weekday, unsigned>);

constexpr bool test() {
  weekday_indexed a(Friday, 2);
  if (a.weekday() != Friday || a.index() != 2 || !a.ok()) return false;
  if (weekday_indexed(Friday, 0).ok() || !weekday_indexed(Friday, 1).ok() || !weekday_indexed(Friday, 5).ok())
    return false;
  if (weekday_indexed(Friday, 6).ok() || weekday_indexed(weekday(9), 1).ok()) return false;
  if (!(a == Friday[2]) || a == Friday[3] || a == Thursday[2]) return false;
  weekday_last l(Monday);
  if (l.weekday() != Monday || !l.ok() || weekday_last(weekday(8)).ok()) return false;
  if (!(l == Monday[last]) || l == Tuesday[last]) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
