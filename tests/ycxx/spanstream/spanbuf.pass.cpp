// [spanbuf.cons]: basic_spanbuf(s, which) initializes the pointers as if by span(s);
// [spanbuf.members]: span() returns span(pbase(), pptr()) when out is set, otherwise the whole
// buffer; span(s) re-initializes (/3: with ate, pptr() == pbase() + s.size()); "the underlying
// sequence never grows": output beyond the end fails (sputc returns eof). [spanbuf.virtuals]:
// seekoff positions within the buffer; fails beyond it.
#include <spanstream>
#include <span>
#include <string_view>
#include "check.hpp"

using traits = std::char_traits<char>;

int main() {
  char buf[5] = {'a', 'b', 'c', 'd', 'e'};
  std::spanbuf sb(std::span<char>(buf), std::ios_base::out);
  CHECK(sb.span().size() == 0);  // out: [pbase, pptr)
  CHECK(sb.sputn("XY", 2) == 2);
  CHECK(sb.span().size() == 2 && sb.span().data() == buf);
  CHECK(std::string_view(buf, 5) == "XYcde");
  CHECK(sb.sputn("1234", 4) == 3);  // only three fit
  CHECK(sb.sputc('!') == traits::eof());
  CHECK(std::string_view(buf, 5) == "XY123");

  std::spanbuf in(std::span<char>(buf), std::ios_base::in);
  CHECK(in.span().size() == 5);  // in only: the whole buffer
  CHECK(in.sbumpc() == 'X');
  CHECK(in.sgetc() == 'Y');
  CHECK(in.pubseekoff(4, std::ios_base::beg, std::ios_base::in) == std::streampos(4));
  CHECK(in.sbumpc() == '3');
  CHECK(in.sgetc() == traits::eof());
  CHECK(in.pubseekoff(6, std::ios_base::beg, std::ios_base::in) == std::streampos(std::streamoff(-1)));

  char b2[3] = {'x', 'y', 'z'};
  std::spanbuf ate(std::span<char>(b2), std::ios_base::out | std::ios_base::ate);
  CHECK(ate.span().size() == 3);
  CHECK(ate.sputc('q') == traits::eof());
  ate.span(std::span<char>(buf, 2));
  CHECK(ate.span().size() == 2);  // ate: pptr == pbase + size

  std::spanbuf def;
  CHECK(def.span().empty());
  CHECK(def.sputc('a') == traits::eof());
  return 0;
}
