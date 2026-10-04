// [istream.extractors]/14: operator>>(basic_streambuf<charT, traits>* sb) "Behaves as an
// unformatted input function": [istream.unformatted]/1 constructs the sentry with noskipws
// true, so leading white space is not skipped, and stores the number of characters extracted
// (gcount()). The extraction stops at end-of-file, which sets eofbit.
#include <istream>
#include <sstream>
#include "check.hpp"

int main() {
  std::istringstream is("  all of it\n");
  std::stringbuf out;
  is >> &out;
  CHECK(out.str() == "  all of it\n");
  CHECK(is.gcount() == 12);
  CHECK(is.eof() && !is.fail());

  // white space only is still inserted: no failbit
  std::istringstream ws(" \t ");
  std::stringbuf out2;
  ws >> &out2;
  CHECK(out2.str() == " \t " && !ws.fail() && ws.gcount() == 3);
  return 0;
}
