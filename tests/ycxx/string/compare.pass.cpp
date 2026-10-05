// [string.compare]: compare(t), compare(pos1, n1, t), compare(pos1, n1, t, pos2, n2),
// compare(str), compare(pos1, n1, str), compare(pos1, n1, str, pos2, n2), compare(s),
// compare(pos, n1, s), compare(pos, n1, s, n2); each is the basic_string_view comparison of
// the selected substrings; the substr calls throw out_of_range when a position exceeds the
// size ([string.view.ops]).
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <stdexcept>
#include "check.hpp"

constexpr int sgn(int v) { return (v > 0) - (v < 0); }

constexpr bool test() {
  const std::string s = "abcde";
  if (s.compare(std::string("abcde")) != 0) return false;
  if (sgn(s.compare(std::string("abd"))) != -1) return false;
  if (sgn(s.compare(std::string("abc"))) != 1) return false;
  if (sgn(s.compare(std::string("abcdef"))) != -1) return false;
  if (sgn(s.compare("b")) != -1) return false;
  if (s.compare(std::string_view("abcde")) != 0) return false;
  if (s.compare(1, 3, std::string("bcd")) != 0) return false;
  if (s.compare(1, 100, std::string("bcde")) != 0) return false;
  if (sgn(s.compare(1, 3, std::string("bc"))) != 1) return false;
  if (s.compare(1, 2, std::string("xbcx"), 1, 2) != 0) return false;
  if (s.compare(3, 2, std::string("xxde"), 2) != 0) return false;
  if (s.compare(0, 2, std::string_view("ab")) != 0) return false;
  if (s.compare(0, 2, std::string_view("_ab_"), 1, 2) != 0) return false;
  if (s.compare(2, 3, "cde") != 0) return false;
  if (s.compare(2, 3, "cdeXX", 3) != 0) return false;
  if (sgn(s.compare(2, 3, "cdeXX", 4)) != -1) return false;
  if (s.compare(5, 0, "") != 0) return false;
  // Comparison is by traits::compare, so unsigned char order for char.
  const std::string hi("\xff", 1);
  if (sgn(hi.compare("a")) != 1) return false;
  // embedded nulls take part
  const std::string z("a\0b", 3);
  if (sgn(z.compare(std::string("a\0a", 3))) != 1) return false;
  if (sgn(z.compare("a")) != 1) return false;  // "a" has length 1
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  const std::string s = "abc";
  int threw = 0;
  try { (void)s.compare(4, 1, std::string("x")); } catch (const std::out_of_range&) { ++threw; }
  try { (void)s.compare(0, 1, std::string("x"), 2); } catch (const std::out_of_range&) { ++threw; }
  try { (void)s.compare(4, 1, "x"); } catch (const std::out_of_range&) { ++threw; }
  try { (void)s.compare(4, 1, "x", 1); } catch (const std::out_of_range&) { ++threw; }
  try { (void)s.compare(4, 1, std::string_view("x")); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 5);
  CHECK(s.compare(3, 1, "") == 0);
  return 0;
}
