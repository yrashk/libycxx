// Formatting durations whose period is finer than a nanosecond or not a power of ten.
// [time.format] Table 133, %S: "If the precision of the input cannot be exactly represented
// with seconds, then the format is a decimal floating-point number with a fixed format and a
// precision matching that of the precision of the input (or to a microseconds precision if
// the conversion to floating-point decimal seconds cannot be made within 18 fractional
// digits)"; %Q / %q: the count and the unit suffix of [time.duration.io]/1 ("as" for atto,
// "[num/den]s" otherwise); [time.format]/7: without chrono-specs the value is formatted as
// os << d writes it; /4: a negative duration gets a '-' before the first replacement;
// [time.hms.overview]/1: fractional_width is the smallest n <= 18 with 10^n a multiple of
// the period's denominator, else 6. [time.duration.io]/1-3 (atto -> "as").
// The formatter of every duration specialisation exists ([time.format]: formatter<
// chrono::duration<Rep, Period>, charT>); none of these needs hours that do not fit.
#include <chrono>
#include <format>
#include <ratio>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class D>
std::string streamed(D d) {
  std::ostringstream os;
  os << d;
  return os.str();
}

int main() {
  // atto: 18 fractional digits, exactly representable.
  using as = duration<long long, std::atto>;
  CHECK(streamed(as{5}) == "5as");
  CHECK(std::format("{}", as{5}) == "5as");
  CHECK(std::format("{:>6}", as{5}) == "   5as");
  CHECK(std::format("{:%Q%q}", as{-5}) == "-5as");
  CHECK(std::format("{:%S}", as{5}) == "00.000000000000000005");
  CHECK(std::format("{:%S}", as{1'234'567'890'123'456'789}) == "01.234567890123456789");
  CHECK(std::format("{:%T}", as{1'234'567'890'123'456'789}) == "00:00:01.234567890123456789");
  CHECK(std::format("{:%S}", as{-5}) == "-00.000000000000000005");
  CHECK(std::format(L"{}", as{7}) == L"7as");

  // femto, pico: 15 and 12 digits.
  CHECK(std::format("{:%T}", duration<long long, std::femto>{1'000'000'000'000'001}) == "00:00:01.000000000000001");
  // /4: only the initial conversion specifier gets the '-'; %Q is then the positive count.
  CHECK(std::format("{:%S|%Q%q}", duration<long long, std::pico>{-1}) == "-00.000000000001|1ps");

  // 1/(3*10^18): no decimal fraction within 18 digits -> microseconds.
  using third_as = duration<long long, std::ratio<1, 3'000'000'000'000'000'000>>;
  CHECK(std::format("{}", third_as{1}) == "1[1/3000000000000000000]s");
  CHECK(std::format("{:%S}", third_as{3'000'000'000'000'000'000 / 2}) == "00.500000");

  // Periods that are not powers of ten.
  using third = duration<long long, std::ratio<1, 3>>;
  CHECK(std::format("{:%S}", third{4}) == "01.333333");
  CHECK(std::format("{:%T}", third{-4}) == "-00:00:01.333333");
  CHECK(std::format("{:%S}", duration<long long, std::ratio<1, 1024>>{1025}) == "01.0009765625"); // 10 digits
  CHECK(std::format("{:%S}", duration<long long, std::ratio<1, 8>>{5}) == "00.625");
  CHECK(std::format("{:%S}", duration<long long, std::ratio<1, 40>>{1}) == "00.025");
  CHECK(std::format("{:%S}", duration<long long, std::ratio<3, 2>>{3}) == "04.5");     // 4.5 s
  CHECK(std::format("{:%S}", duration<long long, std::ratio<5, 4>>{1}) == "01.25");
  CHECK(std::format("{:%S}", duration<long long, std::ratio<7, 3>>{1}) == "02.333333");
  CHECK(std::format("{:%Q%q}", duration<long long, std::ratio<7, 3>>{1}) == "1[7/3]s");
  CHECK(std::format("{:%Q%q}", duration<long long, std::ratio<120>>{2}) == "2[120]s");
  // Coarser than a second: no fraction.
  CHECK(std::format("{:%S|%T}", duration<long long, std::ratio<60>>{61}) == "00|01:01:00");
  CHECK(std::format("{:%T|%j}", duration<long long, std::ratio<172800>>{1}) == "48:00:00|2");
  // The same subsecond digits through hh_mm_ss and a time_point.
  CHECK(std::format("{:%T}", hh_mm_ss{third{4}}) == "00:00:01.333333");
  CHECK(std::format("{:%S}", sys_time<third>{third{4}}) == "01.333333");
  CHECK(std::format("{:%S}", sys_time<duration<long long, std::femto>>{duration<long long, std::femto>{5}}) ==
        "00.000000000000005");
  return 0;
}
