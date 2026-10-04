// [time.cal.wd]: weekday(unsigned wd) stores wd == 7 ? 0 : wd; weekday(sys_days) computes the
// day of the week (1970-01-01 is Thursday); c_encoding() is wd_, iso_encoding() is 7 for Sunday;
// ok() is wd_ <= 6; weekday + days is weekday{modulo(wd_ + y.count(), 7)} (in [0, 6] even if
// !x.ok()); weekday - weekday is the days in [0, 6] with y + d == x; operator[] gives
// weekday_indexed / weekday_last; == only (no ordering).
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class T> concept ordered = requires(T a) { a < a; };
static_assert(!ordered<weekday>);
static_assert(std::is_convertible_v<sys_days, weekday>);    // implicit from sys_days
static_assert(!std::is_convertible_v<local_days, weekday>);  // explicit from local_days
static_assert(std::is_constructible_v<weekday, local_days>);
static_assert(std::is_same_v<decltype(Monday[1]), weekday_indexed>);
static_assert(std::is_same_v<decltype(Monday[last]), weekday_last>);
static_assert(noexcept(Monday + std::declval<days>()) && noexcept(Monday - Sunday) && noexcept(Monday.iso_encoding()));

constexpr bool test() {
  if (Sunday.c_encoding() != 0 || Saturday.c_encoding() != 6 || Monday.c_encoding() != 1) return false;
  if (Sunday.iso_encoding() != 7 || Monday.iso_encoding() != 1 || Saturday.iso_encoding() != 6) return false;
  if (weekday(7) != Sunday || weekday(7).c_encoding() != 0) return false;
  if (!weekday(6).ok() || weekday(8).ok()) return false;
  if (weekday{sys_days{days(0)}} != Thursday) return false;
  if (weekday{sys_days{days(-1)}} != Wednesday || weekday{sys_days{days(-4)}} != Sunday) return false;
  if (weekday{sys_days{days(3)}} != Sunday) return false;
  if (weekday{local_days{days(2)}} != Saturday) return false;
  if (Monday + days{6} != Sunday) return false;  // the draft's example
  if (Sunday - Monday != days{6}) return false;  // the draft's example
  if (Saturday + days(1) != Sunday || Sunday - days(1) != Saturday || days(15) + Friday != Saturday) return false;
  if (Wednesday + days(-10) != Sunday) return false;
  if (Monday - Monday != days(0) || Monday - Tuesday != days(6)) return false;
  if (weekday(10) + days(0) != Wednesday) return false;  // not ok, result still in [0, 6]
  weekday w = Saturday;
  if (++w != Sunday || w++ != Sunday || w != Monday || --w != Sunday || w-- != Sunday || w != Saturday) return false;
  w += days(8);
  if (w != Sunday) return false;
  w -= days(2);
  if (w != Friday) return false;
  weekday_indexed wi = Tuesday[3];
  if (wi.weekday() != Tuesday || wi.index() != 3) return false;
  if (Tuesday[last].weekday() != Tuesday) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // Every day over 2000 years cycles through the week.
  weekday expect = weekday{sys_days{days(-400000)}};
  for (int n = -400000; n < 400000; ++n) {
    weekday w{sys_days{days(n)}};
    CHECK(w == expect);
    CHECK(w.ok());
    ++expect;
  }
}
