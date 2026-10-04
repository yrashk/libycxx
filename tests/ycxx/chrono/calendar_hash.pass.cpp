// [time.hash]/3: hash<day>, hash<month>, hash<year>, hash<weekday>, hash<weekday_indexed>,
// hash<weekday_last>, hash<month_day>, hash<month_day_last>, hash<month_weekday>,
// hash<month_weekday_last>, hash<year_month>, hash<year_month_day>, hash<year_month_day_last>,
// hash<year_month_weekday>, hash<year_month_weekday_last> are enabled; they meet Cpp17Hash "even
// when called on objects k of type Key such that k.ok() is false".
#include <chrono>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class T>
void check(const T& a, const T& b) {
  static_assert(std::is_default_constructible_v<std::hash<T>>);
  static_assert(std::is_same_v<decltype(std::hash<T>{}(a)), std::size_t>);
  std::hash<T> h;
  CHECK(h(a) == h(b));
  CHECK(std::hash<T>{}(a) == h(a));
}

int main() {
  check(day(5), day(5));
  check(day(200), day(200));
  check(month(3), month(3));
  check(month(0), month(0));
  check(year(2000), year(2000));
  check(weekday(2), weekday(2));
  check(weekday(7), weekday(0));  // equal values
  check(Monday[2], Monday[2]);
  check(Monday[last], Monday[last]);
  check(March / 4, March / 4);
  check(month(15) / 40, month(15) / 40);
  check(March / last, March / last);
  check(March / Monday[1], March / Monday[1]);
  check(March / Monday[last], March / Monday[last]);
  check(2000y / March, 2000y / March);
  check(2000y / March / 4, 2000y / March / 4);
  check(2001y / February / 29, 2001y / February / 29);
  check(2000y / March / last, 2000y / March / last);
  check(2000y / March / Monday[1], 2000y / March / Monday[1]);
  check(2000y / March / Monday[last], 2000y / March / Monday[last]);
}
