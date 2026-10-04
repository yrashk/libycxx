// The InputIterator templates of basic_string ([string.cons], [string.append],
// [string.assign], [string.insert], [string.replace]) have "Constraints: InputIterator is a
// type that qualifies as an input iterator", and [container.requirements.general]/69: "as a
// minimum integral types shall not qualify as input iterators". So calls with two integers
// select the (size_type n, charT c) overloads, the second integer converted to charT.
#include <string>
#include "check.hpp"

constexpr bool test() {
  std::string s(3, 65);
  if (s != "AAA") return false;
  s.assign(2, 66);
  if (s != "BB") return false;
  s.append(2, 67);
  if (s != "BBCC") return false;
  s.insert(s.begin(), 2, 68);
  if (s != "DDBBCC") return false;
  s.insert(s.cbegin() + 2, 1, 69);
  if (s != "DDEBBCC") return false;
  s.replace(s.cbegin(), s.cbegin() + 3, 1, 70);
  if (s != "FBBCC") return false;
  // char-sized integer types and unsigned ones too
  unsigned short n = 2;
  unsigned char c = 'x';
  std::string t(n, c);
  if (t != "xx") return false;
  t.append(n, c);
  t.assign(n, c);
  if (t != "xx") return false;
  long ln = 1;
  long lc = 'y';
  t.replace(t.begin(), t.end(), ln, lc);
  if (t != "y") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::wstring w(2, 0x41);
  CHECK(w == L"AA");
  w.append(1, 0x42);
  CHECK(w == L"AAB");
  return 0;
}
