// [facet.ctype.special], [category.ctype]: ctype<char> in the classic locale classifies with
// the "C" table: is(m, c) tests table()[(unsigned char)c] & m (masks space, print, cntrl,
// upper, lower, alpha, digit, punct, xdigit, blank, alnum = alpha | digit, graph = alnum |
// punct); toupper/tolower convert one character or a range in place; widen and narrow are the
// identity for char; scan_is/scan_not find the first character that does / does not match;
// table_size is at least 256; classic_table() is the "C" table.
#include <locale>
#include <cstring>
#include "check.hpp"

int main() {
  const std::ctype<char>& ct = std::use_facet<std::ctype<char>>(std::locale::classic());
  using B = std::ctype_base;
  CHECK(ct.is(B::alpha, 'a') && ct.is(B::upper, 'Q') && ct.is(B::lower, 'q') && !ct.is(B::upper, 'q'));
  CHECK(ct.is(B::digit, '7') && ct.is(B::xdigit, 'f') && !ct.is(B::xdigit, 'g'));
  CHECK(ct.is(B::space, ' ') && ct.is(B::space, '\n') && ct.is(B::blank, '\t') && !ct.is(B::blank, '\n'));
  CHECK(ct.is(B::punct, '!') && ct.is(B::cntrl, '\x07') && ct.is(B::print, ' ') && !ct.is(B::graph, ' '));
  CHECK(ct.is(B::alnum, 'z') && ct.is(B::alnum, '0') && !ct.is(B::alnum, '_'));
  CHECK(ct.is(B::graph, '~') && !ct.is(B::print, '\x7f'));
  CHECK(!ct.is(B::alpha, '\xe9'));  // not a letter in "C"
  static_assert(B::alnum == (B::alpha | B::digit) && B::graph == (B::alnum | B::punct));

  const char text[] = "Ab1 ";
  std::ctype_base::mask v[4];
  CHECK(ct.is(text, text + 4, v) == text + 4);
  CHECK((v[0] & B::upper) && (v[1] & B::lower) && (v[2] & B::digit) && (v[3] & B::space));

  CHECK(ct.toupper('a') == 'A' && ct.toupper('A') == 'A' && ct.toupper('1') == '1');
  CHECK(ct.tolower('Z') == 'z' && ct.tolower('\xc9') == '\xc9');
  char buf[] = "Hello, World";
  CHECK(ct.toupper(buf, buf + std::strlen(buf)) == buf + std::strlen(buf));
  CHECK(std::strcmp(buf, "HELLO, WORLD") == 0);
  ct.tolower(buf, buf + 5);
  CHECK(std::strcmp(buf, "hello, WORLD") == 0);

  CHECK(ct.widen('x') == 'x' && ct.narrow('y', '?') == 'y');
  char out[4];
  const char in[] = "abc";
  CHECK(ct.widen(in, in + 3, out) == in + 3 && std::memcmp(out, "abc", 3) == 0);
  CHECK(ct.narrow(in, in + 3, '?', out) == in + 3 && std::memcmp(out, "abc", 3) == 0);

  const char s[] = "   x1";
  CHECK(ct.scan_not(B::space, s, s + 5) == s + 3);
  CHECK(ct.scan_is(B::digit, s, s + 5) == s + 4);
  CHECK(ct.scan_is(B::punct, s, s + 5) == s + 5);

  CHECK(std::ctype<char>::table_size >= 256);
  CHECK(ct.table() == std::ctype<char>::classic_table());
  CHECK((std::ctype<char>::classic_table()[static_cast<unsigned char>('5')] & B::digit) != 0);

  // The convenience functions use the locale's ctype facet.
  const std::locale& c = std::locale::classic();
  CHECK(std::isalpha('k', c) && std::isdigit('3', c) && std::isspace('\v', c) && std::ispunct('#', c));
  CHECK(std::toupper('m', c) == 'M' && std::tolower('M', c) == 'm' && !std::isupper('m', c));
  CHECK(std::isxdigit('C', c) && std::isblank(' ', c) && std::iscntrl('\0', c) && std::isalnum('9', c));
  return 0;
}
