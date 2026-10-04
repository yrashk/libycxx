// [string.replace]: replace(pos1, n1, str), replace(pos1, n1, str, pos2, n2),
// replace(pos1, n1, t), replace(pos1, n1, t, pos2, n2), replace(pos, n1, s, n2),
// replace(pos, n1, s), replace(pos1, n1, n2, c): remove xlen = min(n1, size() - pos1)
// characters at pos1 and insert the replacement; throw out_of_range if pos1 > size().
// Iterator forms: replace(i1, i2, str / t / s, n / s / n, c / j1, j2 / il) and
// replace_with_range(i1, i2, rg). All return *this.
#include <string>
#include <string_view>
#include <stdexcept>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  std::string s = "hello world";
  if (&s.replace(0, 5, std::string("HELLO")) != &s || s != "HELLO world") return false;
  s.replace(6, 100, std::string("there"));
  if (s != "HELLO there") return false;
  s.replace(0, 5, std::string("abcdef"), 1, 2);
  if (s != "bc there") return false;
  s.replace(0, 2, std::string("abcdef"), 4);
  if (s != "ef there") return false;
  s.replace(3, 5, std::string_view("place"));
  if (s != "ef place") return false;
  s.replace(0, 2, std::string_view("0123"), 1, 2);
  if (s != "12 place") return false;
  s.replace(2, 1, "\0", 1);
  if (s.size() != 8 || s[2] != '\0') return false;
  s.replace(2, 1, "--");
  if (s != "12--place") return false;
  s.replace(0, 2, 3, '*');
  if (s != "***--place") return false;
  s.replace(s.size(), 0, "!");  // at end
  if (s != "***--place!") return false;
  s.replace(0, 0, 0, 'x');
  if (s != "***--place!") return false;

  s = "0123456789";
  s.replace(s.cbegin(), s.cbegin() + 2, std::string("ab"));
  if (s != "ab23456789") return false;
  s.replace(s.cbegin() + 2, s.cbegin() + 4, std::string_view("CD"));
  if (s != "abCD456789") return false;
  s.replace(s.cbegin() + 4, s.cbegin() + 5, "xyz", 2);
  if (s != "abCDxy56789") return false;
  s.replace(s.cbegin() + 6, s.cend(), "!");
  if (s != "abCDxy!") return false;
  s.replace(s.cbegin(), s.cbegin() + 1, 3, 'A');
  if (s != "AAAbCDxy!") return false;
  char buf[] = "pqr";
  s.replace(s.cbegin(), s.cbegin() + 3, InputIter<char>(buf), InputIter<char>(buf + 3));
  if (s != "pqrbCDxy!") return false;
  s.replace(s.cbegin(), s.cend(), buf, buf + 1);
  if (s != "p") return false;
  if (&s.replace_with_range(s.cbegin(), s.cend(), InputRange<char>{buf, buf + 3}) != &s) return false;
  if (s != "pqr") return false;
  s.replace_with_range(s.cbegin() + 1, s.cbegin() + 2, std::string_view("QQ"));
  if (s != "pQQr") return false;
  s.replace(s.cbegin(), s.cbegin() + 2, {'1', '2', '3'});
  if (s != "123Qr") return false;

  // Replacement taken from the string itself, both growing and shrinking.
  std::string u = "0123456789";
  u.replace(0, 2, u);
  if (u != "0123456789" "23456789") return false;
  u = "0123456789";
  u.replace(2, 6, u.data() + 1, 3);
  if (u != "0112389") return false;
  u = "0123456789";
  u.replace(1, 1, u.data() + 4, 5);
  if (u != "045678234567" "89") return false;
  u = "0123456789";
  u.replace(u.cbegin() + 5, u.cend(), u.cbegin(), u.cbegin() + 5);
  if (u != "0123401234") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  int threw = 0;
  try { s.replace(4, 0, "x"); } catch (const std::out_of_range&) { ++threw; }
  try { s.replace(4, 0, 1, 'x'); } catch (const std::out_of_range&) { ++threw; }
  try { s.replace(0, 1, std::string("x"), 2); } catch (const std::out_of_range&) { ++threw; }
  try { s.replace(0, 1, std::string_view("x"), 2, 1); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 4);
  CHECK(s == "abc");
  return 0;
}
