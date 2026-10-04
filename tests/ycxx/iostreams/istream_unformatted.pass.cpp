// [istream.unformatted]: get() / get(c) / get(s, n) / get(s, n, delim) (the delimiter is left
// in the stream) / get(streambuf&) / read(s, n) (failbit|eofbit when fewer are available) /
// readsome / peek (eof() at the end without failbit... sets eofbit) / ignore(n, delim) /
// putback / unget / gcount; unformatted functions do not skip white space.
#include <sstream>
#include <cstring>
#include <string>
#include <limits>
#include "check.hpp"

using traits = std::char_traits<char>;

int main() {
  std::istringstream is(" ab,cd\nefgh");
  CHECK(is.get() == ' ');  // no skipping
  CHECK(is.gcount() == 1);
  char c;
  is.get(c);
  CHECK(c == 'a');
  CHECK(is.peek() == 'b');
  char buf[8];
  is.get(buf, 8, ',');
  CHECK(std::strcmp(buf, "b") == 0 && is.peek() == ',');  // delimiter not extracted
  is.ignore();
  is.get(buf, 8);
  CHECK(std::strcmp(buf, "cd") == 0 && is.peek() == '\n');
  is.ignore(100, 'f');
  CHECK(is.gcount() == 3);  // "\nef"
  CHECK(is.get() == 'g');
  is.unget();
  CHECK(is.get() == 'g');
  is.putback('g');
  CHECK(is.read(buf, 2) && std::string(buf, 2) == "gh");
  CHECK(is.gcount() == 2);
  CHECK(is.peek() == traits::eof() && is.eof() && !is.fail());

  std::istringstream r("xyz");
  r.read(buf, 5);
  CHECK(r.gcount() == 3 && r.eof() && r.fail());

  std::istringstream rs("12345");
  CHECK(rs.readsome(buf, 3) == 3 && std::string(buf, 3) == "123");
  CHECK(rs.readsome(buf, 8) == 2);
  CHECK(rs.readsome(buf, 8) == 0);

  std::istringstream sb_src("copy this\nrest");
  std::stringbuf dest;
  sb_src.get(dest);  // up to '\n'
  CHECK(dest.str() == "copy this" && sb_src.peek() == '\n');

  std::istringstream e("");
  CHECK(e.get() == traits::eof());
  CHECK(e.fail() && e.eof() && e.gcount() == 0);

  std::istringstream ig("aaaa|b");
  ig.ignore(std::numeric_limits<std::streamsize>::max(), '|');
  CHECK(ig.get() == 'b');
  return 0;
}
