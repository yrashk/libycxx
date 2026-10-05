// [string.insert]: insert(pos, str), insert(pos1, str, pos2, n), insert(pos, t),
// insert(pos1, t, pos2, n), insert(pos, s, n), insert(pos, s), insert(pos, n, c) return
// *this and throw out_of_range if pos > size(); insert(p, c), insert(p, n, c),
// insert(p, first, last), insert_range(p, rg), insert(p, il) return an iterator to the first
// inserted character, or p if nothing was inserted (/17, /20, /24, /27).
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::string&>().insert(0, "x")), std::string&>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().insert(std::declval<std::string&>().cbegin(), 'x')),
                             std::string::iterator>);

constexpr bool test() {
  std::string s = "ace";
  const std::string b = "b";
  if (&s.insert(1, b) != &s || s != "abce") return false;
  s.insert(4, std::string("XYZ"), 1, 1);  // at end
  if (s != "abceY") return false;
  s.insert(0, std::string("XYZ"), 2);
  if (s != "ZabceY") return false;
  s.insert(3, std::string_view("--"));
  if (s != "Zab--ceY") return false;
  s.insert(0, std::string_view("0123"), 3, 5);
  if (s != "3Zab--ceY") return false;
  s.insert(1, "nul\0x", 5);
  if (s.size() != 14 || s[4] != '\0') return false;
  s = "ad";
  s.insert(1, "bc");
  s.insert(4, 2, 'e');
  s.insert(0, 0, 'q');
  if (s != "abcdee") return false;

  std::string t = "ac";
  auto it = t.insert(t.cbegin() + 1, 'b');
  if (t != "abc" || it != t.begin() + 1 || *it != 'b') return false;
  it = t.insert(t.cend(), 3, 'd');
  if (t != "abcddd" || it != t.begin() + 3) return false;
  it = t.insert(t.cbegin() + 2, 0, 'x');
  if (t != "abcddd" || it != t.begin() + 2) return false;
  char buf[] = "XYZ";
  it = t.insert(t.cbegin(), InputIter<char>(buf), InputIter<char>(buf + 3));
  if (t != "XYZabcddd" || it != t.begin()) return false;
  it = t.insert(t.cbegin() + 4, buf, buf);
  if (it != t.begin() + 4) return false;
  it = t.insert_range(t.cend(), InputRange<char>{buf, buf + 2});
  if (t != "XYZabcdddXY" || it != t.begin() + 9) return false;
  it = t.insert_range(t.cbegin() + 1, std::string_view());
  if (it != t.begin() + 1 || t.size() != 11) return false;
  it = t.insert(t.cbegin() + 3, {'-', '-'});
  if (t != "XYZ--abcdddXY" || it != t.begin() + 3) return false;

  // Inserting a part of itself.
  std::string u = "0123456789";
  u.insert(2, u);
  if (u != "01012345678923456789") return false;
  u = "0123456789";
  u.insert(5, u.data() + 1, 3);
  if (u != "0123412356789") return false;
  u = "0123456789";
  u.insert(0, u, 8, 2);
  if (u != "890123456789") return false;
  u = "0123456789";
  u.insert(u.cbegin() + 3, u.begin(), u.begin() + 3);
  if (u != "0120123456789") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  int threw = 0;
  try { s.insert(4, "x"); } catch (const std::out_of_range&) { ++threw; }
  try { s.insert(4, 1, 'x'); } catch (const std::out_of_range&) { ++threw; }
  try { s.insert(4, std::string("x")); } catch (const std::out_of_range&) { ++threw; }
  try { s.insert(0, std::string("x"), 2); } catch (const std::out_of_range&) { ++threw; }
  try { s.insert(0, std::string_view("x"), 2, 1); } catch (const std::out_of_range&) { ++threw; }
  try { s.insert(4, "x", 1); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 6);
  CHECK(s == "abc");
  s.insert(3, "d");  // pos == size() is allowed
  CHECK(s == "abcd");
  return 0;
}
