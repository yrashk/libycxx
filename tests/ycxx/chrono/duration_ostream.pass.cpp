// [time.duration.io]/1: operator<<(os, d) inserts d.count() followed by the units suffix,
// formatted in a basic_ostringstream with os's flags, locale and precision, then inserted with
// "os << s.str()" (so width / fill apply to the whole text). Suffixes (1.1-1.22): as fs ps ns,
// "us" or "µs" (implementation-defined), ms cs ds s das hs ks Ms Gs Ts Ps Es min h d, "[num]s"
// when den == 1, otherwise "[num/den]s"; Period::type is the reduced ratio.
// [time.format]/7, Example 4: format("{:=>8}", 42ms) is "====42ms" (no chrono-specs: as if
// streamed).
#include <chrono>
#include <format>
#include <iomanip>
#include <ratio>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;

template <class D>
static std::string str(D d) {
  std::ostringstream os;
  os << d;
  return os.str();
}

int main() {
  CHECK(str(duration<int, std::atto>(1)) == "1as");
  CHECK(str(duration<int, std::femto>(2)) == "2fs");
  CHECK(str(duration<int, std::pico>(3)) == "3ps");
  CHECK(str(nanoseconds(4)) == "4ns");
  std::string us = str(microseconds(5));
  CHECK(us == "5us" || us == "5µs");
  CHECK(str(milliseconds(6)) == "6ms");
  CHECK(str(duration<int, std::centi>(7)) == "7cs");
  CHECK(str(duration<int, std::deci>(8)) == "8ds");
  CHECK(str(seconds(9)) == "9s");
  CHECK(str(duration<int, std::deca>(10)) == "10das");
  CHECK(str(duration<int, std::hecto>(11)) == "11hs");
  CHECK(str(duration<int, std::kilo>(12)) == "12ks");
  CHECK(str(duration<int, std::mega>(13)) == "13Ms");
  CHECK(str(duration<int, std::giga>(14)) == "14Gs");
  CHECK(str(duration<int, std::tera>(15)) == "15Ts");
  CHECK(str(duration<int, std::peta>(16)) == "16Ps");
  CHECK(str(duration<int, std::exa>(17)) == "17Es");
  CHECK(str(minutes(18)) == "18min");
  CHECK(str(hours(19)) == "19h");
  CHECK(str(days(20)) == "20d");
  CHECK(str(weeks(2)) == "2[604800]s");
  CHECK(str(duration<int, std::ratio<3>>(21)) == "21[3]s");
  CHECK(str(duration<int, std::ratio<3, 7>>(22)) == "22[3/7]s");
  CHECK(str(duration<int, std::ratio<6, 4>>(23)) == "23[3/2]s");  // Period::type is reduced
  CHECK(str(duration<int, std::ratio<120, 2>>(24)) == "24min");  // ratio<60>
  CHECK(str(seconds(-5)) == "-5s");
  CHECK(str(duration<double>(1.5)) == "1.5s");

  // flags, precision and width of os
  std::ostringstream os;
  os << std::hex << std::showbase << seconds(255);
  CHECK(os.str() == "0xffs");
  os.str("");
  os << std::dec << std::noshowbase << std::fixed << std::setprecision(2) << duration<double, std::milli>(1.0 / 3);
  CHECK(os.str() == "0.33ms");
  os.str("");
  os << std::setw(6) << std::setfill('*') << seconds(5) << '|';
  CHECK(os.str() == "****5s|");
  os.str("");
  os << std::left << std::setw(6) << minutes(7) << '|';
  CHECK(os.str() == "7min**|");

  std::wostringstream w;
  w << milliseconds(3) << L' ' << hours(1);
  CHECK(w.str() == L"3ms 1h");

  CHECK(std::format("{:=>8}", milliseconds(42)) == "====42ms");
  CHECK(std::format("{}", duration<int, std::ratio<3, 7>>(1)) == "1[3/7]s");
  CHECK(std::format("{:<6}|", minutes(5)) == "5min  |");
  return 0;
}
