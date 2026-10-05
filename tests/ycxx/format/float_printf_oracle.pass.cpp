// Floating-point formatting of random bit patterns (normal, subnormal, huge, tiny, zeros and
// infinities; NaN excluded) of float, double and long double against snprintf as the oracle.
// [format.string.std] Table 110: e/E, f/F and g/G are to_chars(first, last, value,
// chars_format::scientific/fixed/general, precision), precision 6 when absent; none with a
// precision is general with that precision. [charconv.to.chars]/7 (the precision overload):
// "value is converted to a string in the style of printf in the "C" locale with the given
// precision"; "The conversion specifier is f if fmt is chars_format::fixed, e if fmt is
// chars_format::scientific, ... and g if fmt is chars_format::general". So "{:.Ne}" is
// printf's "%.Ne" etc. /7 (alternate form #) keeps the decimal point and, for g, trailing zeros:
// printf's "#" flag; Table 105 (+, space) and /8 (0: zeros after the sign) match printf's
// flags for finite values, Table 104's < and > its "-" flag and default.
// Type none without precision ([charconv.to.chars]/2): the shortest representation that round
// trips, the closest to the value among those; /7: style f when |value| is in [l, u) (l the
// smallest T >= 1e-4, u = 1e16 for double and 1e7 for float: radix^(digits+1) rounded down to
// a power of 10), e otherwise. Checked with strtod (round trip), printf "%.*e" (that D-1
// significant digits do not round trip, and that the D digits are the correctly rounded ones),
// and a spelling oracle.
// COUNTERPART: libcxx:utilities/format/format.functions/(format|format.locale|vformat|vformat.locale).pass.cpp
#include <format>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
#include <cmath>
#include "check.hpp"

static std::string pf(const char* fmt, int prec, long double v) {
  char buf[5200];
  int n = std::snprintf(buf, sizeof buf, fmt, prec, v);
  CHECK(n > 0 && n < static_cast<int>(sizeof buf));
  return std::string(buf, static_cast<std::size_t>(n));
}

static std::string pf(const char* fmt, int prec, double v) {
  char buf[1200];
  int n = std::snprintf(buf, sizeof buf, fmt, prec, v);
  CHECK(n > 0 && n < static_cast<int>(sizeof buf));
  return std::string(buf, static_cast<std::size_t>(n));
}

static unsigned long long state = 0x9E3779B97F4A7C15ull;
static unsigned long long next() {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

static long checks = 0;

static void same(const std::string& got, const std::string& exp, const char* what, int prec) {
  if (got != exp) dprintf(2, "%s (precision %d): got \"%s\", printf gives \"%s\"\n", what, prec, got.c_str(), exp.c_str());
  CHECK(got == exp);
  ++checks;
}

template <class T>
static void precision_forms(T v) {
  using P = std::conditional_t<std::is_same_v<T, long double>, long double, double>;
  const P pv = static_cast<P>(v);
  constexpr bool ld = std::is_same_v<T, long double>;
  const int precs[] = {0, 1, 2, 5, 6, 9, 16, 17, 25, 40};
  for (int p : precs) {
    same(std::vformat("{:.{}e}", std::make_format_args(v, p)), pf(ld ? "%.*Le" : "%.*e", p, pv), "e", p);
    same(std::vformat("{:.{}E}", std::make_format_args(v, p)), pf(ld ? "%.*LE" : "%.*E", p, pv), "E", p);
    same(std::vformat("{:.{}g}", std::make_format_args(v, p)), pf(ld ? "%.*Lg" : "%.*g", p, pv), "g", p);
    same(std::vformat("{:.{}G}", std::make_format_args(v, p)), pf(ld ? "%.*LG" : "%.*G", p, pv), "G", p);
    same(std::vformat("{:.{}}", std::make_format_args(v, p)), pf(ld ? "%.*Lg" : "%.*g", p, pv), "none", p);
    same(std::vformat("{:#.{}g}", std::make_format_args(v, p)), pf(ld ? "%#.*Lg" : "%#.*g", p, pv), "#g", p);
    same(std::vformat("{:#.{}e}", std::make_format_args(v, p)), pf(ld ? "%#.*Le" : "%#.*e", p, pv), "#e", p);
    if (std::fabs(pv) < P(1e30)) {
      same(std::vformat("{:.{}f}", std::make_format_args(v, p)), pf(ld ? "%.*Lf" : "%.*f", p, pv), "f", p);
      same(std::vformat("{:#.{}F}", std::make_format_args(v, p)), pf(ld ? "%#.*LF" : "%#.*F", p, pv), "#F", p);
    }
  }
  // flags with a width
  same(std::vformat("{:+30.8e}", std::make_format_args(v)), pf(ld ? "%+30.*Le" : "%+30.*e", 8, pv), "+30.8e", 8);
  same(std::vformat("{: 030.4g}", std::make_format_args(v)), pf(ld ? "% 030.*Lg" : "% 030.*g", 4, pv), " 030.4g", 4);
  same(std::vformat("{:<40.3e}", std::make_format_args(v)), pf(ld ? "%-40.*Le" : "%-40.*e", 3, pv), "<40.3e", 3);
  same(std::vformat("{:+045.10E}", std::make_format_args(v)), pf(ld ? "%+045.*LE" : "%+045.*E", 10, pv), "+045.10E", 10);
  // defaults: precision 6
  same(std::vformat("{:e}", std::make_format_args(v)), pf(ld ? "%.*Le" : "%.*e", 6, pv), "e default", 6);
  same(std::vformat("{:g}", std::make_format_args(v)), pf(ld ? "%.*Lg" : "%.*g", 6, pv), "g default", 6);
  if (std::fabs(pv) < P(1e30)) same(std::vformat("{:f}", std::make_format_args(v)), pf(ld ? "%.*Lf" : "%.*f", 6, pv), "f default", 6);
}

// shortest round trip, checked without relying on the library's own to_chars
template <class T>
static void shortest(T v) {
  const std::string s = std::format("{}", v);
  if (std::isinf(v)) {
    CHECK(s == (v < 0 ? "-inf" : "inf"));
    return;
  }
  // round trip
  T back;
  if constexpr (std::is_same_v<T, float>) back = std::strtof(s.c_str(), nullptr);
  else back = std::strtod(s.c_str(), nullptr);
  CHECK(back == v && std::signbit(back) == std::signbit(v));
  if (v == 0) {
    CHECK(s == (std::signbit(v) ? "-0" : "0"));
    return;
  }
  // significant digits of s
  std::string digs;
  bool seen_exp = false;
  for (char c : s) {
    if (c == 'e') seen_exp = true;
    if (!seen_exp && c >= '0' && c <= '9') digs += c;
  }
  while (!digs.empty() && digs.front() == '0') digs.erase(0, 1);
  while (!digs.empty() && digs.back() == '0') digs.pop_back();  // integers like 1e+22 written in full
  const int D = static_cast<int>(digs.size());
  CHECK(D >= 1);
  // fewer digits never round trip
  if (D > 1) {
    const std::string fewer = pf("%.*e", D - 2, static_cast<double>(v));
    T fb;
    if constexpr (std::is_same_v<T, float>) fb = std::strtof(fewer.c_str(), nullptr);
    else fb = std::strtod(fewer.c_str(), nullptr);
    CHECK(fb != v);
  }
  // the correctly rounded D digits round trip, so they are the closest: they must be ours
  const std::string sci = pf("%.*e", D - 1, static_cast<double>(v));  // [-]d.ddde[+-]XX
  std::string pd;
  std::size_t epos = sci.find('e');
  for (std::size_t i = 0; i < epos; ++i)
    if (sci[i] >= '0' && sci[i] <= '9') pd += sci[i];
  while (pd.size() > 1 && pd.back() == '0') pd.pop_back();
  T pb;
  if constexpr (std::is_same_v<T, float>) pb = std::strtof(sci.c_str(), nullptr);
  else pb = std::strtod(sci.c_str(), nullptr);
  if (pb == v) CHECK(pd == digs);
  // style: shorter of the fixed and the scientific spelling of these digits, ties to fixed
  const int e10 = std::atoi(sci.c_str() + epos + 1);
  const std::string sign = v < 0 ? "-" : "";
  std::string fixed;
  if (e10 >= D - 1) fixed = digs + std::string(static_cast<std::size_t>(e10 - (D - 1)), '0');
  else if (e10 >= 0) fixed = digs.substr(0, static_cast<std::size_t>(e10 + 1)) + "." + digs.substr(static_cast<std::size_t>(e10 + 1));
  else fixed = "0." + std::string(static_cast<std::size_t>(-e10 - 1), '0') + digs;
  std::string scis = digs.substr(0, 1) + (D > 1 ? "." + digs.substr(1) : "") + "e" + (e10 < 0 ? "-" : "+");
  const int ae = e10 < 0 ? -e10 : e10;
  scis += (ae < 10 ? "0" : "") + std::to_string(ae);
  T lo = static_cast<T>(1e-4);
  if (static_cast<long double>(lo) < 1e-4L) lo = std::nextafter(lo, T(1));
  const T hi = std::is_same_v<T, float> ? T(1e7) : T(1e16);
  const T mag = v < 0 ? -v : v;
  const std::string exp = sign + (mag >= lo && mag < hi ? fixed : scis);
  if (pb == v) {
    if (s != exp) dprintf(2, "shortest: got \"%s\", expected \"%s\"\n", s.c_str(), exp.c_str());
    CHECK(s == exp);
  }
  ++checks;
}

int main() {
  double specials[] = {0.0, -0.0, 1.0, -1.0, 0.1, 0.5, 1e22, 1e23, 5e-324, 2.2250738585072014e-308,
                       std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                       std::numeric_limits<double>::infinity(), 9.5, 0.25, 2.5, 1234567.5, 1e15, 1e16, 1e-5};
  for (double d : specials) {
    precision_forms(d);
    shortest(d);
    precision_forms(static_cast<float>(d));
    shortest(static_cast<float>(d));
  }
  for (int i = 0; i < 400; ++i) {
    std::uint64_t bits = next();
    if (i % 4 == 1) bits &= 0x800FFFFFFFFFFFFFull;  // subnormal
    if (i % 4 == 2) bits = (bits & 0x800FFFFFFFFFFFFFull) | (static_cast<std::uint64_t>(1023 + static_cast<int>(next() % 60) - 30) << 52);
    double d;
    std::memcpy(&d, &bits, sizeof d);
    if (std::isnan(d)) continue;
    precision_forms(d);
    shortest(d);
    std::uint32_t fbits = static_cast<std::uint32_t>(next());
    if (i % 3 == 1) fbits &= 0x807FFFFFu;
    float f;
    std::memcpy(&f, &fbits, sizeof f);
    if (std::isnan(f)) continue;
    precision_forms(f);
    shortest(f);
  }
  // long double with precision (x87 80-bit or wider)
  for (int i = 0; i < 60; ++i) {
    long double ld = static_cast<long double>(static_cast<double>(next() >> 11)) * 1.0000000001L /
                     static_cast<long double>(1ull << (next() % 60));
    if (i % 2) ld = -ld;
    precision_forms(ld);
  }
  CHECK(checks > 10000);
  return 0;
}
