// [string.view.ops]/11-19: compare overloads. compare(str) compares
// traits::compare(data(), str.data(), rlen) then by size per [tab:string.view.compare];
// compare(pos1, n1, str) is substr(pos1, n1).compare(str); compare(pos1, n1, str, pos2, n2)
// is substr(pos1, n1).compare(str.substr(pos2, n2)); compare(const charT*) etc.
// REQUIRES: exceptions
#include <string_view>
#include <stdexcept>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
static_assert(noexcept(std::declval<const SV&>().compare(SV())));

constexpr int sgn(int x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }

constexpr bool test() {
  SV s("abcde");
  if (s.compare(SV("abcde")) != 0) return false;
  if (sgn(s.compare(SV("abd"))) != -1) return false;
  if (sgn(s.compare(SV("abc"))) != 1) return false;     // longer
  if (sgn(s.compare(SV("abcdef"))) != -1) return false;  // shorter
  if (sgn(SV().compare(SV())) != 0 || sgn(SV().compare("a")) != -1) return false;
  if (s.compare(1, 2, SV("bc")) != 0) return false;
  if (sgn(s.compare(1, 2, SV("bd"))) != -1) return false;
  if (s.compare(1, 100, SV("bcde")) != 0) return false;
  if (s.compare(1, 2, SV("xbcx"), 1, 2) != 0) return false;
  if (s.compare(0, 1, SV("zzza"), 3, 100) != 0) return false;
  if (s.compare("abcde") != 0 || sgn(s.compare("b")) != -1) return false;
  if (s.compare(2, 3, "cde") != 0) return false;
  if (s.compare(2, 2, "cdz", 2) != 0) return false;
  if (sgn(s.compare(2, 2, "cdz", 3)) != -1) return false;
  // unsigned comparison of char_traits<char>::lt
  const char hi[] = {static_cast<char>(0xF0), 0};
  if (sgn(SV(hi).compare(SV("a"))) != 1) return false;
  return true;
}
static_assert(test());

template <class F>
bool throws(F f) {
  try {
    f();
  } catch (const std::out_of_range&) {
    return true;
  }
  return false;
}

int main() {
  CHECK(test());
  SV s("abc");
  CHECK(throws([&] { (void)s.compare(4, 1, SV("a")); }));
  CHECK(throws([&] { (void)s.compare(0, 1, SV("a"), 2, 1); }));
  CHECK(throws([&] { (void)s.compare(4, 1, "a"); }));
  CHECK(throws([&] { (void)s.compare(4, 1, "a", 1); }));
  return 0;
}
