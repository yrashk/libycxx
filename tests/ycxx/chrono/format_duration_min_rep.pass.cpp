// Formatting a duration whose count is the most negative value of a narrow signed rep.
// [time.format]/4: "The result of formatting a std::chrono::duration instance holding a
// negative value ... is equivalent to the output of the corresponding positive value, with a
// STATICALLY-WIDEN<charT>("-") character sequence placed before the replacement of the
// initial conversion specifier." The corresponding positive value (here 3276.8 s, 1.28 s,
// 128 s, 2^31 s, 32768 min) need not be a value of the rep, but nothing makes formatting such
// a duration undefined: the count itself is a valid value and %Q is "as if extracted via
// .count()" of that positive value. Table 133: %T is %H:%M:%S, %S with the input's precision.
// (duration<long long> with LLONG_MIN is not tested: its positive value has no wider type.)
#include <chrono>
#include <format>
#include <ratio>
#include "check.hpp"

using namespace std::chrono;

int main() {
  // 2147483648 s is 596523 h 14 min 8 s; 32768 min is 546 h 8 min.
  CHECK(std::format("{:%T}", duration<short, std::deci>{-32767}) == "-00:54:36.7");
  CHECK(std::format("{:%T|%Q%q}", duration<short, std::deci>{-32768}) == "-00:54:36.8|32768ds");
  CHECK(std::format("{:%S}", duration<signed char, std::centi>{-128}) == "-01.28");
  CHECK(std::format("{:%T}", duration<signed char>{-128}) == "-00:02:08");
  CHECK(std::format("{:%T}", duration<int>{-2147483647 - 1}) == "-596523:14:08");
  CHECK(std::format("{:%M}", duration<short, std::ratio<60>>{-32768}) == "-08"); // 546 h 8 min
  CHECK(std::format("{}", duration<short, std::deci>{-32768}) == "-32768ds");
  CHECK(std::format("{:%T}", duration<unsigned, std::milli>{4'000'000'001u}) == "1111:06:40.001");
  CHECK(std::format("{:%T}", duration<unsigned char, std::centi>{255}) == "00:00:02.55");

  CHECK(std::format(L"{:%T}", duration<short, std::deci>{-32768}) == L"-00:54:36.8");
  CHECK(std::format("{:%H|%M|%S}", duration<int, std::milli>{-2147483647 - 1}) == "-596|31|23.648");
  return 0;
}
