// [time.format] Table 133 for durations: %H %M %S %T %R (a duration is "interpreted as the time
// of day elapsed since midnight", /6), %Q the count, %q the units suffix ([time.duration.io]),
// %j "the decimal number of days without padding" for a duration, %n %t %%; %S with a
// precision finer than seconds: "a decimal floating-point number with a fixed format and a
// precision matching that of the precision of the input" (milliseconds: 3 digits). /4: a
// negative duration is formatted as the positive value with '-' before the replacement of the
// first conversion specifier (Example 1). /1: a precision is valid only for durations with a
// floating-point rep; "For all other types, an exception of type format_error is thrown".
// /6 and /3: a specifier the type has no information for (%Y, %a for a duration) throws
// format_error. Literal characters are copied; fill, align and width apply to the whole result.
#include <chrono>
#include <format>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class... A>
static bool throws(std::string_view fmt, A... a) {
  try {
    (void)std::vformat(fmt, std::make_format_args(a...));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

int main() {
  CHECK(std::format("{:%H:%M:%S}", 3723s) == "01:02:03");
  CHECK(std::format("{:%T}", 3723s) == "01:02:03");
  CHECK(std::format("{:%R}", 3723s) == "01:02");
  CHECK(std::format("{:%T}", 25h + 1min) == "25:01:00");  // hours are not reduced modulo 24
  CHECK(std::format("{:%Q %q}", 42ms) == "42 ms");
  CHECK(std::format("{:%Q}", minutes(3)) == "3");
  CHECK(std::format("{:%q}", hours(1)) == "h");
  CHECK(std::format("{:%j}", days(400)) == "400");
  CHECK(std::format("{:%j}", hours(49)) == "2");
  CHECK(std::format("{:%S}", 1250ms) == "01.250");
  CHECK(std::format("{:%T}", 3723004ms) == "01:02:03.004");
  CHECK(std::format("{:%S}", duration<long long, std::micro>(5000001)) == "05.000001");
  CHECK(std::format("{:%S}", duration<int, std::deci>(15)) == "01.5");
  CHECK(std::format("{:%M}", 61s) == "01");
  CHECK(std::format("{:%n%t%%}", 1s) == "\n\t%");
  CHECK(std::format("[{:%H h}]", 7h) == "[07 h]");
  // Example 1: negative durations
  CHECK(std::format("{:%T}", -10'000s) == "-02:46:40");
  CHECK(std::format("{:%H:%M:%S}", -10'000s) == "-02:46:40");
  CHECK(std::format("minutes {:%M, hours %H, seconds %S}", -10'000s) == "minutes -46, hours 02, seconds 40");
  CHECK(std::format("{:%Q%q}", -5ms) == "-5ms");
  // width / fill / align
  CHECK(std::format("{:*^12%T}", 3723s) == "**01:02:03**");
  CHECK(std::format("{:>10%Q}", 42s) == "        42");
  // a floating-point rep accepts a precision (its effect on the output is not checked here)
  (void)std::format("{:.2%Q}", duration<double>(1.23456));
  (void)std::format("{:.1}", duration<double, std::milli>(2.25));
  // errors
  CHECK(throws("{:.2%Q}", 5s));
  CHECK(throws("{:.1}", 5s));
  CHECK(throws("{:%Y}", 5s));
  CHECK(throws("{:%a}", 5s));
  CHECK(throws("{:%Z}", 5s));
  CHECK(throws("{:%K}", 5s));  // not a conversion specifier
  CHECK(throws("{:%}", 5s));
  CHECK(!throws("{:%H}", 5s));
  // wide
  CHECK(std::format(L"{:%T}", 3723s) == L"01:02:03");
  CHECK(std::format(L"{}", 3ms) == L"3ms");
  return 0;
}
