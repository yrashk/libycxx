// [istream.manip]: ws "skips white space ... If it stops because it reaches the end of input,
// it sets eofbit but not failbit" (it behaves as an unformatted input function except that it
// does not count). [istream.sentry]: with skipws (the default) formatted input skips leading
// white space; with noskipws it does not. [istream.extractors]: operator>>(char&) skips too.
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  std::istringstream is("   x  \t\n y   ");
  char c;
  is >> std::ws;
  CHECK(is.peek() == 'x');
  is.get(c);
  is >> std::ws;
  CHECK(is.peek() == 'y');
  is.get(c);
  is >> std::ws;
  CHECK(is.eof() && !is.fail());

  std::istringstream e("");
  e >> std::ws;
  CHECK(e.eof() && !e.fail());

  std::istringstream ns(" a b");
  ns >> std::noskipws >> c;
  CHECK(c == ' ');
  ns >> c;
  CHECK(c == 'a');
  ns >> std::skipws >> c;
  CHECK(c == 'b');
  CHECK(!(ns.flags() & std::ios_base::skipws) == false);

  std::istringstream s("  word  next");
  std::string w;
  s >> w;
  CHECK(w == "word");
  s >> std::noskipws >> w;
  CHECK(s.fail());  // the string extractor finds white space first: nothing extracted
  return 0;
}
