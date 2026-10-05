// [istream.extractors]/7-10: operator>>(in, charT(&s)[N]) (and the unsigned char / signed char
// overloads for char streams): n is min(size_t(width()), N) if width() > 0, otherwise N;
// extraction stops after n - 1 characters, at end of file, or at white space (ctype of the
// imbued locale); a null character is stored next "which may be the first position"; width(0)
// is called; failbit if nothing was extracted. /12-13: operator>>(in, charT& c): failbit when no
// character is available.
// /14-15: operator>>(basic_streambuf* sb) (its unformatted-input aspects are checked in
// extract_streambuf_unformatted); null sb: failbit; extraction stops at end of file or when insertion fails
// (the character is not extracted); failbit if nothing is inserted.
// [ostream.inserters]/7-9: operator<<(basic_streambuf* sb): null sb: badbit; reads from sb until
// end of file or until insertion fails; failbit if nothing is inserted; an exception thrown
// while getting a character sets failbit and is rethrown only if failbit is in exceptions().
// REQUIRES: exceptions
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <cstring>
#include "check.hpp"

using traits = std::char_traits<char>;

struct Small : std::streambuf {  // accepts three characters
  char buf[3];
  Small() { setp(buf, buf + 3); }
  std::string str() const { return std::string(pbase(), pptr()); }
};
struct ThrowingSource : std::streambuf {
  int n = 0;
  int_type underflow() override {
    if (n == 2) throw 9;
    static char c[2] = {'a', 'b'};
    setg(c + n, c + n, c + n + 1);
    ++n;
    return traits::to_int_type(c[n - 1]);
  }
};

int main() {
  {
    std::istringstream is("  hello world");
    char buf[8];
    is >> buf;
    CHECK(std::strcmp(buf, "hello") == 0 && is.good());
    is.width(3);
    is >> buf;  // at most 2 characters
    CHECK(std::strcmp(buf, "wo") == 0 && is.width() == 0);
    is >> buf;
    CHECK(std::strcmp(buf, "rld") == 0 && is.eof() && !is.fail());
  }
  {
    // N limits when width() is 0 or larger than N
    std::istringstream is("abcdefgh");
    char buf[4];
    is.width(100);
    is >> buf;
    CHECK(std::strcmp(buf, "abc") == 0 && is.width() == 0);
    is >> buf;
    CHECK(std::strcmp(buf, "def") == 0);
    CHECK(is.peek() == 'g');
    // width(1): n == 1, nothing can be stored: failbit, empty string
    char one[4] = {'x', 'x', 'x', 'x'};
    is.width(1);
    is >> one;
    CHECK(one[0] == '\0' && is.fail() && is.width() == 0);
  }
  {
    // only white space: failbit | eofbit
    std::istringstream is("   ");
    char buf[4] = {'x', 'x', 'x', 'x'};
    is >> buf;
    CHECK(is.fail() && is.eof());
  }
  {
    std::istringstream is("xyz w");
    unsigned char ub[3];
    signed char sb[3];
    is >> ub >> sb;
    CHECK(ub[0] == 'x' && ub[1] == 'y' && ub[2] == 0);
    CHECK(sb[0] == 'z' && sb[1] == 0);
    unsigned char uc = 0;
    signed char sc = 0;
    is >> uc;
    CHECK(uc == 'w');
    is >> sc;
    CHECK(is.fail() && is.eof());
  }
  {
    std::wistringstream is(L" wide  chars");
    wchar_t buf[8];
    is >> buf;
    CHECK(std::wstring(buf) == L"wide");
    wchar_t c = 0;
    is >> c;
    CHECK(c == L'c');
  }
  {
    // operator>>(streambuf*)
    std::istringstream is("all of it\nand more");
    std::stringbuf out;
    is >> &out;
    CHECK(out.str() == "all of it\nand more");  // everything, across white space
    CHECK(is.eof() && !is.fail());
    std::istringstream e("");
    std::stringbuf out2;
    e >> &out2;
    CHECK(e.fail());
    std::istringstream n("abc");
    n >> static_cast<std::streambuf*>(nullptr);
    CHECK(n.fail());
    std::istringstream s("abcdef");
    Small small;
    s >> &small;
    CHECK(small.str() == "abc" && !s.fail());
    CHECK(s.get() == 'd');  // the character that could not be inserted is still there
  }
  {
    // operator<<(streambuf*)
    std::stringbuf src("copied text");
    std::ostringstream os;
    os << &src;
    CHECK(os.str() == "copied text" && os.good());
    std::stringbuf empty_src("");
    os << &empty_src;
    CHECK(os.fail() && !os.bad());
    std::ostringstream n;
    n << static_cast<std::streambuf*>(nullptr);
    CHECK(n.bad());
    // insertion fails: the character stays in the source
    std::stringbuf src2("123456");
    Small small;
    std::ostream sm(&small);
    sm << &src2;
    CHECK(small.str() == "123");
    CHECK(src2.sgetc() == '4');
    // an exception while getting a character: failbit, not rethrown...
    ThrowingSource ts;
    std::ostringstream o1;
    o1 << &ts;
    CHECK(o1.fail() && o1.str() == "ab");
    // ...unless failbit is in exceptions()
    ThrowingSource ts2;
    std::ostringstream o2;
    o2.exceptions(std::ios_base::failbit);
    int caught = 0;
    try {
      o2 << &ts2;
    } catch (int e) {
      caught = e;
    } catch (...) {
      caught = -1;
    }
    CHECK(caught == 9 && o2.fail());
  }
  return 0;
}
