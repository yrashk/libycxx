// [stringbuf.members]/3 init-buf-ptrs: "If ios_base::out is set in mode, pbase() points to
// buf.front() and epptr() >= pbase() + buf.size() is true; in addition, if ios_base::ate is
// set in mode, pptr() == pbase() + buf.size() is true, otherwise pptr() == pbase() is true.
// If ios_base::in is set in mode, eback() points to buf.front(), and gptr() == eback() and
// egptr() == eback() + buf.size() hold." str(s) is "buf = s; init-buf-ptrs();", so ate applies
// again after str(s). /1: high_mark: writing over existing characters does not shorten the
// sequence. [stringbuf.virtuals]/8: overflow with in set "alters the read end pointer egptr()
// to point just past the new write position". /9-11, Tables 144-145: seekoff with end uses
// high_mark - xbeg; "If the sequence's next pointer (either gptr() or pptr()) is a null pointer
// and newoff is nonzero, the positioning operation fails"; positions beyond the high mark
// (uninitialized characters) or before the beginning fail.
#include <sstream>
#include <string>
#include "check.hpp"

using B = std::ios_base;
using pos = std::stringbuf::pos_type;
using off = std::stringbuf::off_type;

struct Exposed : std::stringbuf {
  using std::stringbuf::stringbuf;
  char* eb() const { return eback(); }
  char* g() const { return gptr(); }
  char* eg() const { return egptr(); }
  char* pb() const { return pbase(); }
  char* p() const { return pptr(); }
  char* ep() const { return epptr(); }
};

int main() {
  const pos fail = pos(off(-1));
  {
    Exposed sb("abc", B::in | B::out | B::ate);
    CHECK(sb.p() == sb.pb() + 3 && sb.ep() >= sb.pb() + 3);
    CHECK(sb.g() == sb.eb() && sb.eg() == sb.eb() + 3);
    CHECK(sb.sgetc() == 'a');
    CHECK(sb.sputc('d') == 'd');
    CHECK(sb.str() == "abcd");
    // [stringbuf.virtuals]/1: "Any character in the underlying buffer which has been
    // initialized is considered to be part of the input sequence."
    char rd[5] = {};
    CHECK(sb.sgetn(rd, 5) == 4 && std::string(rd) == "abcd");
    CHECK(sb.pubseekoff(0, B::cur, B::out) == pos(4));
    CHECK(sb.pubseekoff(0, B::cur, B::in) == pos(4));
    sb.str("hello");  // init-buf-ptrs again, with ate
    CHECK(sb.p() == sb.pb() + 5);
    CHECK(sb.sputc('!') == '!' && sb.str() == "hello!");
  }
  {
    std::ostringstream os("hello", B::ate);
    CHECK(os.tellp() == pos(5));
    os << " world";
    CHECK(os.str() == "hello world");
    os.str("xyz");
    os << '1';
    CHECK(os.str() == "xyz1");
  }
  {
    std::ostringstream os("hello");  // no ate: overwrite, high mark kept
    CHECK(os.tellp() == pos(0));
    os << "J";
    CHECK(os.str() == "Jello");
    os.seekp(0, B::end);
    CHECK(os.tellp() == pos(5));
    os << "!";
    CHECK(os.str() == "Jello!");
    os.seekp(2);
    os << "LL";
    CHECK(os.str() == "JeLLo!");
    os.seekp(6);  // exactly the high mark
    CHECK(os.good() && os.tellp() == pos(6));
    os.seekp(7);  // beyond it
    CHECK(os.fail());
    os.clear();
    CHECK(os.tellp() == pos(6));
    os.seekp(-1, B::beg);
    CHECK(os.fail());
  }
  {
    // Written characters become readable; seek both sequences with beg / end.
    std::stringstream ss;
    ss << "one two";
    std::string w;
    ss >> w;
    CHECK(w == "one");
    ss << " three";
    ss >> w;
    CHECK(w == "two");
    ss >> w;
    CHECK(w == "three" && ss.eof());
    ss.clear();
    CHECK(ss.rdbuf()->pubseekoff(-5, B::end, B::in | B::out) == pos(8));
    CHECK(ss.rdbuf()->sgetc() == 't');
    CHECK(ss.rdbuf()->sputc('T') == 'T');
    CHECK(ss.str() == "one two Three");
    CHECK(ss.rdbuf()->pubseekoff(1, B::cur, B::in | B::out) == fail);
    CHECK(ss.rdbuf()->pubseekoff(14, B::beg, B::in) == fail);
    CHECK(ss.rdbuf()->pubseekoff(13, B::beg, B::in) == pos(13));
    CHECK(ss.rdbuf()->sgetc() == std::char_traits<char>::eof());
  }
  {
    // ate with in only has no effect on the get area.
    std::istringstream is("xy", B::ate);
    char c = 0;
    is >> c;
    CHECK(c == 'x');
  }
  {
    // A sequence that is not in the mode has null next pointers: newoff 0 + off 0 is fine,
    // anything that makes newoff nonzero (end on a non-empty buffer) fails.
    std::stringbuf out_only("abc", B::out);
    CHECK(out_only.pubseekoff(0, B::beg, B::in) == pos(0));
    CHECK(out_only.pubseekoff(0, B::end, B::in) == fail);
    CHECK(out_only.pubseekoff(0, B::end, B::out) == pos(3));
    std::stringbuf in_only("abc", B::in);
    CHECK(in_only.pubseekoff(0, B::beg, B::out) == pos(0));
    CHECK(in_only.pubseekoff(0, B::end, B::out) == fail);
    CHECK(in_only.pubseekoff(-1, B::end, B::in) == pos(2));
    CHECK(in_only.sgetc() == 'c');
    CHECK(in_only.sputc('x') == std::char_traits<char>::eof());  // no out: no write position
  }
  return 0;
}
