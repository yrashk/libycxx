// [facet.num.get.virtuals] (C locale): Stage 1: %d / %u by default, %o for oct, %X for hex,
// %i when basefield is 0 (prefix detection: 0x hex, 0 octal); Stage 2 accumulates the
// characters allowed by the conversion and leaves the rest (Example 1: "0x1a.bp+07p" with %d
// accumulates "0", with %i "0x1a"); Stage 3 converts with strtoll / strtoull: zero and failbit
// if the field is not converted; eofbit when the end of input is reached.
#include <sstream>
#include <ios>
#include <string>
#include "check.hpp"

template<class T>
static T get(const char* s, std::ios_base::fmtflags base, std::ios_base::iostate* st = nullptr,
             std::string* rest = nullptr) {
  std::istringstream is(s);
  is.setf(base, std::ios_base::basefield);
  T v{};
  is >> v;
  if (st) *st = is.rdstate();
  if (rest) {
    is.clear();
    std::getline(is, *rest, '\0');
  }
  return v;
}

int main() {
  using B = std::ios_base;
  B::iostate st;
  std::string rest;
  CHECK(get<int>("123", B::dec, &st) == 123 && st == B::eofbit);
  CHECK(get<int>("-123 ", B::dec, &st) == -123 && st == B::goodbit);
  CHECK(get<int>("+7", B::dec) == 7);
  CHECK(get<int>("ff", B::hex) == 255);
  CHECK(get<int>("0xFF", B::hex) == 255);  // %X accepts the 0x prefix
  CHECK(get<int>("17", B::oct) == 15);
  CHECK(get<int>("0x1f", B::fmtflags{}) == 31);
  CHECK(get<int>("017", B::fmtflags{}) == 15);
  CHECK(get<int>("17", B::fmtflags{}) == 17);
  CHECK(get<unsigned>("4000000000", B::dec) == 4000000000u);

  // Example 1
  CHECK(get<int>("0x1a.bp+07p", B::dec, &st, &rest) == 0 && rest == "x1a.bp+07p" && st == B::goodbit);
  CHECK(get<int>("0x1a.bp+07p", B::fmtflags{}, &st, &rest) == 26 && rest == ".bp+07p");

  CHECK(get<int>("12abc", B::dec, &st, &rest) == 12 && rest == "abc");
  CHECK(get<int>("9", B::oct, &st) == 0 && (st & B::failbit));  // nothing accumulated
  CHECK(get<int>("", B::dec, &st) == 0 && st == (B::failbit | B::eofbit));
  CHECK(get<int>("-", B::dec, &st) == 0 && (st & B::failbit));
  CHECK(get<int>("   42", B::dec) == 42);  // skipws

  std::istringstream multi("1 0x10 010");
  multi.unsetf(B::basefield);
  int a, b, c;
  multi >> a >> b >> c;
  CHECK(a == 1 && b == 16 && c == 8);

  std::istringstream ns(" 5");
  ns >> std::noskipws;
  int n = 3;
  ns >> n;
  CHECK(ns.fail() && n == 0);
  return 0;
}
