// [string.io]: operator>>(is, str): skips white space, erases str, extracts until white space
// or n characters where n is is.width() if positive, otherwise str.max_size(); width(0) is
// called; failbit if nothing is extracted. operator<<(os, str) behaves as a formatted output
// function: padding to width() with fill(), adjusted per adjustfield, then width(0).
// [ostream.inserters.character]: char / const char* inserters pad the same way.
#include <sstream>
#include <string>
#include <string_view>
#include "check.hpp"

int main() {
  std::istringstream is("abcdefgh ij");
  std::string s = "old";
  is.width(3);
  is >> s;
  CHECK(s == "abc" && is.width() == 0);
  is >> s;
  CHECK(s == "defgh");
  is >> s;
  CHECK(s == "ij" && is.eof() && !is.fail());
  is >> s;
  CHECK(is.fail());

  std::istringstream cs("xyzzy");
  char arr[4];
  cs.width(4);
  cs >> arr;  // at most width - 1 characters plus a null
  CHECK(std::string(arr) == "xyz");

  std::ostringstream os;
  os.width(6);
  os << std::string("ab");
  CHECK(os.str() == "    ab" && os.width() == 0);
  os << std::string("cd");
  CHECK(os.str() == "    abcd");

  std::ostringstream l;
  l.setf(std::ios_base::left, std::ios_base::adjustfield);
  l.fill('-');
  l.width(5);
  l << std::string_view("sv");
  l.width(4);
  l << "cs";
  l.width(3);
  l << 'c';
  CHECK(l.str() == "sv---cs--c--");

  std::ostringstream in;
  in.setf(std::ios_base::internal, std::ios_base::adjustfield);
  in.width(4);
  in << "ab";  // internal for strings: pad before
  CHECK(in.str() == "  ab");

  std::ostringstream ch;
  ch.width(3);
  ch << 'x' << 'y';
  CHECK(ch.str() == "  xy");
  return 0;
}
