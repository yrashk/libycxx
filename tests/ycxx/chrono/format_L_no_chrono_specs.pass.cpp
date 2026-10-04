// "{:L}" without chrono-specs uses the formatting locale for the whole stream insertion.
// [time.format]/2: with the L option the formatting locale is the locale passed to the
// formatting function, otherwise the global locale; without L it is the "C" locale.
// [time.format]/7: "if the chrono-specs is omitted, the chrono object is formatted as if by
// streaming it to basic_ostringstream<charT> os with the formatting locale imbued and copying
// os.str() through the output iterator of the context with additional padding and
// adjustments as specified by the format specifiers."
// [time.duration.io]/1: os << d is s.imbue(os.getloc()); s << d.count() << units-suffix, so
// the count is inserted by num_put with the locale's numpunct (digit grouping with
// thousands_sep, [facet.num.put.virtuals] stage 2; decimal_point for a floating-point count).
// Hence format(loc, "{:L}", d) equals the text of an ostringstream imbued with loc after
// os << d, and format(loc, "{}", d) the text with the classic locale.
#include <chrono>
#include <format>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class C>
struct Punct : std::numpunct<C> {
  C do_thousands_sep() const override { return C('\''); }
  C do_decimal_point() const override { return C(','); }
  std::string do_grouping() const override { return "\3"; }
};

template <class D>
std::string streamed(const std::locale& loc, D d) {
  std::ostringstream os;
  os.imbue(loc);
  os << d;
  return os.str();
}

int main() {
  const std::locale loc(std::locale(std::locale::classic(), new Punct<char>), new Punct<wchar_t>);
  CHECK(streamed(loc, 1234567ms) == "1'234'567ms");
  CHECK(streamed(loc, duration<double>(1234.5)) == "1'234,5s");

  CHECK(std::format(loc, "{:L}", 1234567ms) == "1'234'567ms");
  CHECK(std::format(loc, "{:L}", -1234567ms) == "-1'234'567ms");
  CHECK(std::format(loc, "{:L}", duration<double>(1234.5)) == "1'234,5s");
  CHECK(std::format(loc, "{:L}", minutes(999)) == "999min");
  CHECK(std::format(loc, "{:>14L}", 1234567ms) == "   1'234'567ms");
  CHECK(std::format(loc, "{:*<13L}", 1234567ms) == "1'234'567ms**");
  CHECK(std::format(loc, L"{:L}", 1234567ms) == L"1'234'567ms");
  CHECK(std::format(loc, "{:L}", duration<long long, std::ratio<3>>(4000)) == "4'000[3]s");

  // Without L: the "C" locale.
  CHECK(std::format(loc, "{}", 1234567ms) == "1234567ms");
  CHECK(std::format(loc, "{}", duration<double>(1234.5)) == "1234.5s");
  CHECK(std::format(loc, "{}", sys_days{2021y / January / 1} + 1500ms) == "2021-01-01 00:00:01.500");
  CHECK(std::format(loc, "{:L}", sys_days{2021y / January / 1} + 1500ms) == "2021-01-01 00:00:01,500");

  // The global locale.
  const std::locale old = std::locale::global(loc);
  CHECK(std::format("{:L}", 1234567ms) == "1'234'567ms");
  CHECK(std::format("{}", 1234567ms) == "1234567ms");
  std::ostringstream os;  // constructed with the global locale
  os << 1234567ms;
  CHECK(os.str() == "1'234'567ms");
  std::locale::global(old);
  return 0;
}
