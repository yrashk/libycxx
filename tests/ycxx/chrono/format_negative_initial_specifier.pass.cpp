// The '-' of a negative duration or hh_mm_ss goes before the replacement of the *initial*
// conversion specifier, whichever it is. [time.format]/4: "The result of formatting a
// std::chrono::duration instance holding a negative value, or an hh_mm_ss object h for which
// h.is_negative() is true, is equivalent to the output of the corresponding positive value,
// with a STATICALLY-WIDEN<charT>("-") character sequence placed before the replacement of the
// initial conversion specifier." (Example 1: "minutes {:%M, hours %H, seconds %S}" of
// -10'000s prints "minutes -46, hours 02, seconds 40".) [time.format]/1: conversion-spec is
// % modifier_opt type, and type includes n, t, % and p, q, Q (Table 133: %n a new-line
// character, %t a horizontal tab, %% a % character), so these are conversion specifiers too.
// /6: %p, %I treat a duration as the time of day elapsed since midnight. Literal characters
// after the first specifier get no '-' (the chrono-specs must start with a conversion-spec).
// hh_mm_ss is not a duration: %j ("If the type being formatted is a specialization of
// duration, the decimal number of days ... Otherwise, the day of the year") needs a date it
// does not hold, so /3 requires format_error.
// REQUIRES: exceptions
#include <chrono>
#include <format>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

int main() {
  CHECK(std::format("{:%n%t%%}", -1s) == "-\n\t%");
  CHECK(std::format("{:%t%S}", -1s) == "-\t01");
  CHECK(std::format("{:%%%Q}", -5s) == "-%5");
  CHECK(std::format("{:%Q|%Q}", -5s) == "-5|5");
  CHECK(std::format("{:%q%Q}", -5s) == "-s5");
  CHECK(std::format("{:%p %I}", -5s) == "-AM 12");
  CHECK(std::format("{:%I%p}", -13h) == "-01PM");
  CHECK(std::format("{:%p}", -13h) == "-PM");
  CHECK(std::format("{:%H:%M:%S %p}", -5s) == "-00:00:05 AM");
  CHECK(std::format("{:%M, hours %H, seconds %S}", -10'000s) == "-46, hours 02, seconds 40");
  CHECK(std::format("{:%j|%M}", -(49h + 5min)) == "-2|05");
  CHECK(std::format("{:>8%n%Q}", -3ms) == "     -\n3"); // padding around the whole
  CHECK(std::format(L"{:%%%Q}", -5s) == L"-%5");
  CHECK(std::format(L"{:%n}", -1min) == L"-\n");

  // hh_mm_ss.
  const hh_mm_ss neg{-(25h + 1min + 2s)};
  CHECK(neg.is_negative());
  CHECK(std::format("{:%n%T}", neg) == "-\n25:01:02");
  CHECK(std::format("{:%p %T}", neg) == "-AM 25:01:02");
  CHECK(std::format("{:%%%H}", neg) == "-%25");
  bool threw = false;
  try {
    const hh_mm_ss<seconds> h{-49h};
    (void)std::vformat("{:%j}", std::make_format_args(h));
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);

  // Non-negative values: no '-'.
  CHECK(std::format("{:%n%t%%}", 0s) == "\n\t%");
  CHECK(std::format("{:%p}", hh_mm_ss{0s}) == "AM");
  return 0;
}
