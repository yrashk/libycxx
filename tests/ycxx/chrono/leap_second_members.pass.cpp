// [time.zone.leap.overview]: leap_second is copyable (defaulted copy constructor and copy
// assignment; its other constructors are unspecified, so objects come from the database);
// date() and value() are constexpr and noexcept. [time.zone.leap.members]/1-2: date() is the
// insertion's date and time, value() +1s or -1s. [time.zone.leap.nonmembers]/1-12: == and <=>
// between two leap_seconds compare date() (strong_ordering); ==, <, >, <=, >= and <=> with a
// sys_time<Duration> of any duration compare date() with it, all noexcept.
// COUNTERPART: libcxx:time/time.zone/time.zone.leap/.*
#include <chrono>
#include <compare>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::chrono;
using LS = leap_second;

static_assert(std::is_copy_constructible_v<LS> && std::is_copy_assignable_v<LS>);
static_assert(std::is_same_v<decltype(std::declval<const LS&>().date()), sys_seconds>);
static_assert(std::is_same_v<decltype(std::declval<const LS&>().value()), seconds>);
static_assert(noexcept(std::declval<const LS&>().date()) && noexcept(std::declval<const LS&>().value()));
static_assert(std::is_same_v<decltype(std::declval<const LS&>() <=> std::declval<const LS&>()), std::strong_ordering>);
static_assert(noexcept(std::declval<const LS&>() == std::declval<const LS&>()));
static_assert(noexcept(std::declval<const LS&>() < std::declval<const sys_days&>()));
static_assert(noexcept(std::declval<const sys_time<milliseconds>&>() >= std::declval<const LS&>()));
static_assert(std::is_same_v<decltype(std::declval<const LS&>() <=> std::declval<const sys_days&>()),
                             std::strong_ordering>);

template <class D>
void compare_with(const LS& l, sys_time<D> t) {
  const auto d = l.date();
  CHECK((l == t) == (d == t) && (t == l) == (d == t) && (l != t) == (d != t));
  CHECK((l < t) == (d < t) && (t < l) == (t < d));
  CHECK((l > t) == (d > t) && (t > l) == (t > d));
  CHECK((l <= t) == (d <= t) && (t <= l) == (t <= d));
  CHECK((l >= t) == (d >= t) && (t >= l) == (t >= d));
  CHECK((l <=> t) == (d <=> t));
}

int main() {
  const auto& ls = get_tzdb().leap_seconds;
  CHECK(ls.size() >= 27);
  const LS first = ls[0];  // copy construction
  CHECK(first.date() == sys_days{1972y / July / 1} && first.value() == 1s);
  LS copy = ls[1];
  CHECK(copy.date() == sys_days{1973y / January / 1});
  copy = first;  // copy assignment
  CHECK(copy.date() == first.date() && copy.value() == first.value());

  // Between leap_seconds.
  CHECK(copy == first && !(copy != first));
  CHECK(ls[0] < ls[1] && ls[1] > ls[0] && ls[0] <= ls[0] && ls[1] >= ls[0]);
  CHECK((ls[0] <=> ls[1]) == std::strong_ordering::less);
  CHECK((ls[1] <=> ls[0]) == std::strong_ordering::greater);
  CHECK((ls[2] <=> ls[2]) == std::strong_ordering::equal);
  for (std::size_t i = 1; i < ls.size(); ++i) CHECK(ls[i - 1] < ls[i] && ls[i].date() > ls[i - 1].date());

  // With sys_time of several durations: before, at and after the date.
  for (const LS& l : {ls[0], ls[5], ls[ls.size() - 1]}) {
    compare_with(l, l.date());
    compare_with(l, l.date() - 1s);
    compare_with(l, l.date() + 1s);
    compare_with(l, sys_time<milliseconds>{l.date()} + 1ms);
    compare_with(l, sys_time<milliseconds>{l.date()} - 1ms);
    compare_with(l, sys_time<nanoseconds>{l.date()});
    compare_with(l, floor<days>(l.date()));
    compare_with(l, floor<days>(l.date()) - days{1});
  }
  CHECK(ls[0] == sys_days{1972y / July / 1});
  CHECK(sys_days{1972y / July / 1} == ls[0]);
  CHECK(ls[0] > sys_seconds{sys_days{1972y / July / 1}} - 1s);
}
