// [stringbuf.virtuals]/pbackfail: when the input position is not at the beginning, putting back
// the same character just backs up; putting back a different character overwrites it only if
// the buffer is writable (out set), otherwise fails; with eof() it backs up. [streambuf.pub.get]
// sputbackc / sungetc; [istream.unformatted]: putback / unget set badbit on failure.
#include <sstream>
#include <string>
#include "check.hpp"

using traits = std::char_traits<char>;

int main() {
  std::stringbuf rw("abc");
  CHECK(rw.sbumpc() == 'a');
  CHECK(rw.sputbackc('a') == 'a');
  CHECK(rw.sgetc() == 'a');
  CHECK(rw.sputbackc('z') == traits::eof());  // at the beginning: nothing to back up into
  rw.sbumpc();
  CHECK(rw.sputbackc('Q') == 'Q');  // writable: overwrites
  CHECK(rw.str() == "Qbc");
  rw.sbumpc();
  CHECK(rw.sungetc() == 'Q');

  std::stringbuf ro("abc", std::ios_base::in);
  ro.sbumpc();
  CHECK(ro.sputbackc('x') == traits::eof());  // read-only: different char fails
  CHECK(ro.sputbackc('a') == 'a');
  CHECK(ro.str() == "abc");

  std::istringstream is("xy");
  char c;
  is.get(c);
  is.putback('x');
  CHECK(is.good());
  is.get(c);
  CHECK(c == 'x');
  is.putback('q');  // fails: read-only
  CHECK(is.bad());
  std::istringstream is2("k");
  is2.unget();
  CHECK(is2.bad());
  return 0;
}
