// [time.cal.month]: ok() is 1 <= m_ <= 12; month + months is
// month{modulo(unsigned{x} + (y.count() - 1), 12) + 1} (Euclidean, always in [1, 12] "even if
// !x.ok()"); month - month gives months in [0, 11] with y + m == x; ++/-- wrap; the constants
// January..December are month{1}..month{12}.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(January), const month>);
static_assert(std::is_same_v<decltype(month(1) - month(2)), months>);
static_assert(!std::is_convertible_v<unsigned, month>);
static_assert(noexcept(std::declval<month>() + std::declval<months>()) && noexcept(month(1) - month(2)) && noexcept(month(1).ok()));

constexpr bool test() {
  if (unsigned(January) != 1 || unsigned(February) != 2 || unsigned(December) != 12) return false;
  if (month(0).ok() || !month(1).ok() || !month(12).ok() || month(13).ok()) return false;
  if (February + months{11} != January) return false;  // the draft's example
  if (January - February != months{11}) return false;  // the draft's example
  if (December + months(1) != January || January - months(1) != December) return false;
  if (March + months(-14) != January) return false;
  if (March + months(120) != March || March - months(121) != February) return false;
  if (months(3) + October != January) return false;
  if (December - January != months(11) || January - January != months(0)) return false;
  // Not-ok months still give a result in [1, 12].
  if (month(0) + months(0) != December) return false;
  if (month(13) + months(0) != January) return false;
  if (month(200) + months(0) != month(8)) return false;  // (200 - 1) mod 12 + 1
  month m = December;
  if (++m != January || m-- != January || m != December || --m != November) return false;
  m += months(2);
  if (m != January) return false;
  m -= months(13);
  if (m != December) return false;
  if (!(January < February) || (March <=> March) != std::strong_ordering::equal) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
