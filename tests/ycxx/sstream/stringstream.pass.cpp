// [stringstream]: basic_stringstream reads and writes the same buffer, with independent get
// and put positions; reading consumes what was written; the default mode is in|out.
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  std::stringstream ss;
  ss << 10 << ' ' << 20;
  int a = 0, b = 0;
  ss >> a >> b;
  CHECK(a == 10 && b == 20);
  CHECK(ss.eof());
  ss.clear();
  ss << " 30";
  int c = 0;
  ss >> c;
  CHECK(c == 30);
  CHECK(ss.str() == "10 20 30");

  std::stringstream rw("abc");
  rw << "X";  // put position starts at 0
  CHECK(rw.str() == "Xbc");
  char ch;
  rw >> ch;
  CHECK(ch == 'X');
  CHECK(rw.tellg() == std::streampos(1));
  CHECK(rw.tellp() == std::streampos(1));

  std::stringstream in_only("5 6", std::ios_base::in);
  in_only << 1;
  CHECK(in_only.bad() || in_only.fail());  // no output sequence
  return 0;
}
