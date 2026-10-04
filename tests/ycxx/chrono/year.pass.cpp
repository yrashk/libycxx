// [time.cal.year]: is_leap() is y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); ok() is
// min() <= y <= max() with min() year{-32767} and max() year{32767}; unary + and -; year + years
// is year{int{x} + static_cast<int>(y.count())}; year - year gives years; the literal y.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(2000y), year>);
static_assert(std::is_same_v<decltype(2000y - 1999y), years>);
static_assert(!std::is_convertible_v<int, year> && !std::is_convertible_v<year, int>);
static_assert(noexcept(year::min()) && noexcept(year(1).is_leap()) && noexcept(-year(1)));

constexpr bool test() {
  if (!year(2000).is_leap() || year(1900).is_leap() || !year(2024).is_leap() || year(2023).is_leap()) return false;
  if (!year(0).is_leap() || !year(-4).is_leap() || year(-100).is_leap() || !year(-400).is_leap()) return false;
  if (year::min() != year(-32767) || year::max() != year(32767)) return false;
  if (!year(-32767).ok() || !year(32767).ok() || year(-32768).ok()) return false;
  if (int(-year(5)) != -5 || +year(5) != year(5)) return false;
  if (year(2000) + years(24) != 2024y || years(-1) + year(2000) != 1999y || 2000y - years(1000) != 1000y) return false;
  if (2024y - 2000y != years(24) || 2000y - 2024y != years(-24)) return false;
  year y = 1999y;
  if (++y != 2000y || y++ != 2000y || y != 2001y || --y != 2000y || y-- != 2000y || y != 1999y) return false;
  y += years(3);
  if (y != 2002y) return false;
  y -= years(5);
  if (y != 1997y) return false;
  if (!(1999y < 2000y) || (2000y <=> 2000y) != std::strong_ordering::equal) return false;
  if (int(0y) != 0 || int(32767y) != 32767) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
