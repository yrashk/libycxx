// [time.cal.md]: month_day ok() when m_.ok(), 1d <= d_ and d_ is at most the days in m_ (29 for
// February); == and <=> (month, then day). [time.cal.mdlast]: month_day_last, ok() is m_.ok().
// [time.cal.mwd]: month_weekday, ok() is m_.ok() && wdi_.ok(). [time.cal.mwdlast]:
// month_weekday_last, ok() is m_.ok() && wdl_.ok().
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(February / 29d <=> March / 1d), std::strong_ordering>);
static_assert(std::is_same_v<decltype(February / last <=> March / last), std::strong_ordering>);
static_assert(!std::is_convertible_v<month, month_day_last>);

constexpr bool test() {
  if (!(February / 29).ok() || (February / 30).ok() || !(January / 31).ok() || (April / 31).ok()) return false;
  if (!(April / 30).ok() || (May / 0).ok() || (month(13) / 1).ok() || (December / 32).ok()) return false;
  month_day md(March, day(14));
  if (md.month() != March || md.day() != 14d) return false;
  if (!(January / 31 < February / 1) || !(March / 1 > February / 29)) return false;
  if ((June / 6 <=> June / 6) != std::strong_ordering::equal) return false;

  month_day_last mdl = February / last;
  if (mdl.month() != February || !mdl.ok() || month_day_last(month(0)).ok()) return false;
  if (!(January / last < February / last) || January / last != last / January) return false;

  month_weekday mwd = May / Sunday[2];
  if (mwd.month() != May || mwd.weekday_indexed() != Sunday[2] || !mwd.ok()) return false;
  if ((May / Sunday[6]).ok() || (month(0) / Sunday[1]).ok()) return false;
  if (!(mwd == Sunday[2] / May) || mwd == May / Sunday[3]) return false;

  month_weekday_last mwdl = May / Monday[last];
  if (mwdl.month() != May || mwdl.weekday_last() != Monday[last] || !mwdl.ok()) return false;
  if ((month(13) / Monday[last]).ok() || (May / weekday(9)[last]).ok()) return false;
  if (!(mwdl == Monday[last] / May)) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
