// [charconv.from.chars]/1: from_chars sets value to "the parsed value, after rounding
// according to round_to_nearest ([round.style])" -- whatever the floating-point environment's
// current rounding direction ([cfenv.syn], fesetround). [charconv.to.chars]/2: to_chars
// without a precision produces the shortest representation that round-trips, "If there are
// several such representations, the representation with the smallest difference to the
// floating-point argument value is chosen, resolving any remaining ties using rounding
// according to round_to_nearest" -- fully determined, so also independent of the rounding
// direction. Decimal and hexadecimal inputs with more digits than the type holds (exact ties,
// just above, just below), subnormals and values near the overflow threshold are parsed, and
// shortest forms written, under FE_UPWARD, FE_DOWNWARD and FE_TOWARDZERO, and compared bit for
// bit with the results under FE_TONEAREST (the oracle there: strtod/strtof).
#include <charconv>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>
#include <vector>
#include "check.hpp"

static std::uint64_t st = 0x123456789abcdefull;
static std::uint64_t rnd() {
  st ^= st << 13;
  st ^= st >> 7;
  st ^= st << 17;
  return st;
}

struct Case {
  std::string s;
  std::chars_format fmt;
  double d;
  float f;
};

static std::vector<Case> cases;

static void add(const std::string& s, std::chars_format fmt = std::chars_format::general) {
  Case c{s, fmt, 0, 0};
  std::string forc = fmt == std::chars_format::hex ? "0x" + s : s;
  c.d = std::strtod(forc.c_str(), nullptr);
  c.f = std::strtof(forc.c_str(), nullptr);
  cases.push_back(c);
}

template <class T>
static bool same_bits(T a, T b) {
  return std::memcmp(&a, &b, sizeof a) == 0;
}

static void parse_all(const char* mode) {
  for (const Case& c : cases) {
    double d = 0;
    auto r = std::from_chars(c.s.data(), c.s.data() + c.s.size(), d, c.fmt);
    if (std::isinf(c.d)) {
      CHECK(r.ec == std::errc::result_out_of_range);
    } else if (c.d != 0 || c.s.find_first_of("123456789") == std::string::npos) {
      if (!(r.ec == std::errc{} && same_bits(d, c.d)))
        dprintf(2, "%s: from_chars<double>(\"%.60s\") = %a, want %a\n", mode, c.s.c_str(), d, c.d);
      CHECK(r.ec == std::errc{} && same_bits(d, c.d));
    }
    float f = 0;
    auto rf = std::from_chars(c.s.data(), c.s.data() + c.s.size(), f, c.fmt);
    if (!std::isinf(c.f) && (c.f != 0 || c.s.find_first_of("123456789") == std::string::npos)) {
      if (!(rf.ec == std::errc{} && same_bits(f, c.f)))
        dprintf(2, "%s: from_chars<float>(\"%.60s\") = %a, want %a\n", mode, c.s.c_str(), static_cast<double>(f),
                static_cast<double>(c.f));
      CHECK(rf.ec == std::errc{} && same_bits(f, c.f));
    }
  }
}

static std::vector<double> shortest_values;
static std::vector<std::string> shortest_nearest;

static std::string shortest(double v) {
  char b[64];
  auto r = std::to_chars(b, b + sizeof b, v);
  CHECK(r.ec == std::errc{});
  return std::string(b, r.ptr);
}
static std::string shortest_f(float v) {
  char b[64];
  auto r = std::to_chars(b, b + sizeof b, v);
  CHECK(r.ec == std::errc{});
  return std::string(b, r.ptr);
}

int main() {
  // exact ties between adjacent doubles, padded, just above and below
  for (int i = 0; i < 300; ++i) {
    double lo = std::ldexp(static_cast<double>((rnd() >> 11) | (1ull << 52)), static_cast<int>(rnd() % 1900) - 1000);
    double hi = std::nextafter(lo, INFINITY);
    if (!std::isfinite(hi)) continue;
    long double mid = (static_cast<long double>(lo) + static_cast<long double>(hi)) / 2;
    char buf[2000];
    int n = std::snprintf(buf, sizeof buf, "%.800Le", mid);
    std::string s(buf, static_cast<std::size_t>(n));
    auto e = s.find('e');
    std::string m = s.substr(0, e), ex = s.substr(e);
    while (m.back() == '0') m.pop_back();
    add(m + ex);
    add(m + "0000000000000000000000001" + ex);
    if (m.back() > '1') {
      std::string lower = m;
      --lower.back();
      add(lower + "99999999999999999999" + ex);
    }
  }
  // random decimal strings, subnormal and near-overflow magnitudes
  for (int i = 0; i < 300; ++i) {
    std::string d = std::to_string(rnd() % 9 + 1) + "." + std::to_string(rnd()) + std::to_string(rnd());
    add(d + "e" + std::to_string(static_cast<int>(rnd() % 640) - 330));
    add(d + "e-" + std::to_string(310 + rnd() % 14));
    add(d + "e" + std::to_string(300 + rnd() % 8));
    add(d + "e-" + std::to_string(38 + rnd() % 8));
  }
  // hexadecimal with more bits than the significand
  for (int i = 0; i < 200; ++i) {
    char b[80];
    std::snprintf(b, sizeof b, "1.%016llx%04xp%d", static_cast<unsigned long long>(rnd()),
                  static_cast<unsigned>(rnd() % 65536), static_cast<int>(rnd() % 2000) - 1000);
    add(b, std::chars_format::hex);
    std::snprintf(b, sizeof b, "1.%013llx8p%d", static_cast<unsigned long long>(rnd() >> 12),
                  static_cast<int>(rnd() % 200) - 100);  // an exact tie for double
    add(b, std::chars_format::hex);
  }
  add("1.7976931348623158e308");  // rounds to DBL_MAX under round-to-nearest
  add("1.7976931348623159e308");  // overflows under round-to-nearest
  add("2.4703282292062328e-324");  // just above half the smallest subnormal
  add("0.000");
  // shortest forms of random values
  for (int i = 0; i < 2000; ++i) {
    double v;
    std::uint64_t bits = rnd();
    std::memcpy(&v, &bits, sizeof v);
    if (!std::isfinite(v)) continue;
    shortest_values.push_back(v);
  }

  CHECK(std::fegetround() == FE_TONEAREST);
  parse_all("nearest");
  std::vector<std::string> sf;
  for (double v : shortest_values) {
    shortest_nearest.push_back(shortest(v));
    sf.push_back(shortest_f(static_cast<float>(v)));
  }
  for (int mode : {FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
    CHECK(std::fesetround(mode) == 0);
    parse_all(mode == FE_UPWARD ? "upward" : mode == FE_DOWNWARD ? "downward" : "towardzero");
    for (std::size_t i = 0; i < shortest_values.size(); ++i) {
      CHECK(shortest(shortest_values[i]) == shortest_nearest[i]);
    }
    std::fesetround(FE_TONEAREST);
    // the float conversions above must happen in nearest mode; compare float shortest forms
    std::vector<float> fv;
    for (double v : shortest_values) fv.push_back(static_cast<float>(v));
    std::fesetround(mode);
    for (std::size_t i = 0; i < fv.size(); ++i) CHECK(shortest_f(fv[i]) == sf[i]);
    std::fesetround(FE_TONEAREST);
  }
}
