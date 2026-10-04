// [time.cal.day]: day holds an unsigned value; ok() is 1 <= d_ <= 31; ++/--, +=/-= days;
// day + days is day(unsigned{x} + y.count()); day - day is days{int(x) - int(y)}; == and <=>
// (strong_ordering) on the value; the literal d gives day{static_cast<unsigned>(d)}.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(1d), day>);
static_assert(std::is_same_v<decltype(day(1) <=> day(2)), std::strong_ordering>);
static_assert(std::is_same_v<decltype(day(5) - day(2)), days>);
static_assert(!std::is_convertible_v<unsigned, day> && !std::is_convertible_v<day, unsigned>);
static_assert(noexcept(day(1)) && noexcept(++std::declval<day&>()) && noexcept(day(1).ok()));
static_assert(noexcept(std::declval<day>() + std::declval<days>()) && noexcept(unsigned(day(1))) && noexcept(5d));

constexpr bool test() {
  if (day(0).ok() || !day(1).ok() || !day(31).ok() || day(32).ok() || day(255).ok()) return false;
  if (unsigned(day(17)) != 17) return false;
  day d(5);
  if (unsigned(++d) != 6 || unsigned(d++) != 6 || unsigned(d) != 7) return false;
  if (unsigned(--d) != 6 || unsigned(d--) != 6 || unsigned(d) != 5) return false;
  d += days(10);
  if (d != 15d) return false;
  d -= days(3);
  if (d != 12d) return false;
  if (day(3) + days(4) != day(7) || days(4) + day(3) != day(7) || day(10) - days(3) != day(7)) return false;
  if (day(3) - day(10) != days(-7) || day(31) - day(1) != days(30)) return false;
  if (!(day(1) < day(2)) || (day(3) <=> day(3)) != std::strong_ordering::equal) return false;
  if (unsigned(31d) != 31) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
