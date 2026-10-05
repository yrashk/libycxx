// operator>> for float, double and long double: [facet.num.get.virtuals] Stage 2 accumulates
// the characters a %g field may continue with (from "0123456789abcdefpxABCDEFPX+-" and the
// decimal point), Stage 3 converts "by the rules of" strtof/strtod/strtold; "zero, if the
// conversion function does not convert the entire field"; failbit "if the conversion function
// does not convert the entire field, or if the field represents a value outside the range of
// representable values"; /5 eofbit at the end of input. Checked: the extreme finite values,
// values beyond them (failbit), fields that are only partly convertible ("1e", "1e+", "."),
// characters outside the atoms ("inf", "nan": nothing accumulated), very long mantissas and
// leading zeros, and random decimal strings compared bit for bit with strtod/strtof/strtold.
#include <sstream>
#include <string>
#include <limits>
#include <cstdlib>
#include <cstring>
#include "check.hpp"

template <class T>
struct Res {
  T v;
  bool fail, eof;
  std::string rest;
};

template <class T>
static Res<T> get(const std::string& in) {
  std::istringstream is(in);
  T v = T(77);
  is >> v;
  Res<T> r{v, is.fail(), is.eof(), {}};
  is.clear();
  std::getline(is, r.rest);
  return r;
}

template <class T>
static bool same_bits(T a, T b) {
  if constexpr (std::is_same_v<T, long double>) return a == b;  // padding bytes
  else return std::memcmp(&a, &b, sizeof a) == 0;
}

template <class T>
static T c_conv(const char* s) {
  if constexpr (std::is_same_v<T, float>) return std::strtof(s, nullptr);
  else if constexpr (std::is_same_v<T, double>) return std::strtod(s, nullptr);
  else return std::strtold(s, nullptr);
}

template <class T>
static void ok(const std::string& in, T v, const char* rest = "") {
  Res<T> r = get<T>(in);
  if (!same_bits(r.v, v) || r.fail || r.rest != rest)
    dprintf(2, "\"%s\": got %Lg fail=%d rest=\"%s\"\n", in.c_str(), static_cast<long double>(r.v), r.fail, r.rest.c_str());
  CHECK(same_bits(r.v, v) && !r.fail && r.rest == rest);
  CHECK(r.eof == (*rest == '\0'));
}

template <class T>
static void zero_fail(const std::string& in, const char* rest = "") {
  Res<T> r = get<T>(in);
  if (r.v != T(0) || !r.fail || r.rest != rest)
    dprintf(2, "\"%s\": got %Lg fail=%d rest=\"%s\"\n", in.c_str(), static_cast<long double>(r.v), r.fail, r.rest.c_str());
  CHECK(r.v == T(0) && r.fail && r.rest == rest);
}

template <class T>
static void out_of_range(const std::string& in) {
  Res<T> r = get<T>(in);
  CHECK(r.fail && r.rest.empty());
}

template <class T>
static void run(const char* max_text, const char* beyond, const char* min_text) {
  using L = std::numeric_limits<T>;
  ok<T>(max_text, L::max());
  ok<T>(std::string("-") + max_text, -L::max());
  ok<T>(min_text, L::min());
  out_of_range<T>(beyond);
  out_of_range<T>(std::string("-") + beyond);
  out_of_range<T>("1e999999");
  ok<T>("0", T(0));
  ok<T>("-0", -T(0));
  ok<T>("1", T(1));
  ok<T>("0." + std::string(400, '0') + "1e401", T(1));
  ok<T>("1" + std::string(500, '0') + "e-500", T(1));
  ok<T>(std::string(300, '0') + "2.5", T(2.5));
  ok<T>("  \t\n3.25", T(3.25));
  ok<T>("1.5e+2x", T(150), "x");
  ok<T>("1.5E-1 next", c_conv<T>("1.5E-1"), " next");
  ok<T>("12.5.6", T(12.5), ".6");  // a second '.' cannot continue the field
  ok<T>("7-", T(7), "-");
  zero_fail<T>("1e");
  zero_fail<T>("1e+");
  zero_fail<T>("-1.5e-");
  zero_fail<T>(".");
  zero_fail<T>("-");
  zero_fail<T>("+.e1", "e1");  // "+.e" can begin no field: e is not accumulated
  zero_fail<T>("inf", "inf");
  zero_fail<T>("nan", "nan");
  zero_fail<T>("-inf", "inf");
  zero_fail<T>("$1", "$1");

  // random decimal strings against the C conversion function
  unsigned long long x = 0x2545F4914F6CDD1Dull;
  auto rnd = [&](unsigned m) {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return static_cast<unsigned>(x % m);
  };
  const int emax = L::max_exponent10 - 2;
  for (int i = 0; i < 3000; ++i) {
    std::string s;
    if (rnd(2)) s += '-';
    const unsigned nd = 1 + rnd(i % 10 == 0 ? 60 : 22);
    const unsigned dot = rnd(nd + 1);
    for (unsigned k = 0; k < nd; ++k) {
      if (k == dot) s += '.';
      s += static_cast<char>('0' + rnd(10));
    }
    if (rnd(3)) {
      s += rnd(2) ? 'e' : 'E';
      const int e = static_cast<int>(rnd(static_cast<unsigned>(2 * emax))) - emax;
      s += std::to_string(e);
    }
    const T expect = c_conv<T>(s.c_str());
    if (expect == T(0) || expect != expect || expect - expect != T(0)) continue;  // skip 0, under/overflow
    if (expect < L::min() && expect > -L::min()) continue;  // subnormal: strtod may report ERANGE
    Res<T> r = get<T>(s);
    if (!same_bits(r.v, expect) || r.fail) dprintf(2, "\"%s\": got %.21Lg, expected %.21Lg\n", s.c_str(), static_cast<long double>(r.v), static_cast<long double>(expect));
    CHECK(same_bits(r.v, expect) && !r.fail && r.eof);
  }
}

int main() {
  run<float>("3.40282346638528859811704183484516925440e+38", "3.5e38", "1.17549435082228750796873653722224568e-38");
  run<double>("1.7976931348623157e308", "1.8e308", "2.2250738585072014e-308");
  run<long double>("1.18973149535723176502e+4932", "1.2e4932", "3.36210314311209350626e-4932");
  return 0;
}
