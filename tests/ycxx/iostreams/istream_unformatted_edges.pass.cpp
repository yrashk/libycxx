// [istream.unformatted]/1: if the sentry is false, "the number of extracted characters is set
// to 0; unformatted input functions taking a character array of nonzero size as an argument
// shall also store a null character (using charT()) in the first location of the array"; an
// exception thrown during input sets badbit and is rethrown if (exceptions() & badbit) != 0.
// /8-9: get(s, n, delim) with n == 1 stores no characters: failbit, then a null character.
// /13-14: get(sb, delim) stops when inserting fails ("the character to be inserted is not
// extracted") or at the delimiter (not extracted); failbit if nothing is inserted.
// /18-21: getline(s, n, delim): conditions tested in the order eof, delimiter, n - 1 stored, so
// a line that exactly fills the buffer does not set failbit; n == 1 with a non-delimiter next.
// /25: ignore stops at delim (extracted), or n characters, or eof (eofbit only). /27-28 (C++26):
// ignore(n, char_type delim) compares with to_int_type(delim), so a negative char delimiter
// works. /29-30: peek returns eof() if good() is false. /31: read when !good(): failbit.
// /33: readsome: in_avail() == -1 sets eofbit and extracts nothing; !good() sets failbit.
// /35, /37: putback / unget first clear eofbit; failure of sputbackc / sungetc sets badbit;
// gcount() is 0 afterwards (Notes 2-3).
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <cstring>
#include <limits>
#include "check.hpp"

using traits = std::char_traits<char>;

struct Small : std::streambuf {  // accepts two characters, then overflow() fails
  char buf[2];
  Small() { setp(buf, buf + 2); }
  std::string str() const { return std::string(pbase(), pptr()); }
};
struct NoMore : std::streambuf {  // in_avail() == -1
  std::streamsize showmanyc() override { return -1; }
};
struct ThrowingSource : std::streambuf {
  int_type underflow() override { throw 7; }
};

int main() {
  {
    // a false sentry stores a null character into the array
    std::istringstream is("abc");
    is.setstate(std::ios_base::eofbit);
    char buf[4] = {'x', 'x', 'x', 'x'};
    is.get(buf, 4);
    CHECK(buf[0] == '\0' && is.gcount() == 0 && is.fail());
    buf[0] = 'x';
    is.getline(buf, 4);
    CHECK(buf[0] == '\0' && is.gcount() == 0);
    buf[0] = 'x';
    is.get(buf, 4, ',');
    CHECK(buf[0] == '\0');
  }
  {
    // get(s, 1): nothing can be stored: failbit and an empty string
    std::istringstream is("abc");
    char buf[2] = {'x', 'x'};
    is.get(buf, 1);
    CHECK(buf[0] == '\0' && is.fail() && !is.eof() && is.gcount() == 0);
    is.clear();
    CHECK(is.get() == 'a');
  }
  {
    // get(s, n) at the delimiter: nothing stored, failbit; the delimiter stays
    std::istringstream is("\nabc");
    char buf[4];
    is.get(buf, 4);
    CHECK(is.fail() && buf[0] == '\0');
    is.clear();
    CHECK(is.get() == '\n');
  }
  {
    // getline: a line exactly filling the buffer does not set failbit
    std::istringstream is("abc\ndefg\nh");
    char buf[4];
    is.getline(buf, 4);
    CHECK(is.good() && std::strcmp(buf, "abc") == 0 && is.gcount() == 4);
    is.getline(buf, 4);  // "def" stored, 'g' is neither eof nor the delimiter
    CHECK(is.fail() && std::strcmp(buf, "def") == 0 && is.gcount() == 3);
    is.clear();
    CHECK(is.get() == 'g');
    // getline(s, 1): the next character is the delimiter: extracted, no failbit
    char one[1];
    is.getline(one, 1);
    CHECK(!is.fail() && one[0] == '\0' && is.gcount() == 1);
    // getline(s, 1): the next character is not the delimiter: failbit
    is.getline(one, 1);
    CHECK(is.fail() && one[0] == '\0' && is.gcount() == 0);
    is.clear();
    // the last line without a delimiter: eofbit, no failbit
    is.getline(buf, 4);
    CHECK(is.eof() && !is.fail() && std::strcmp(buf, "h") == 0 && is.gcount() == 1);
  }
  {
    // get(sb): insertion failure leaves the character in the input
    std::istringstream is("abcd\nx");
    Small sb;
    is.get(sb);
    CHECK(sb.str() == "ab" && is.good() && is.gcount() == 2);
    CHECK(is.get() == 'c');
    // get(sb, delim) stops at the delimiter and does not extract it
    std::istringstream is2("ab;cd");
    std::stringbuf out;
    is2.get(out, ';');
    CHECK(out.str() == "ab" && is2.peek() == ';');
    // nothing inserted (at the delimiter): failbit
    std::stringbuf out2;
    is2.get(out2, ';');
    CHECK(is2.fail() && out2.str().empty());
  }
  {
    // ignore
    std::istringstream is("abcdef");
    is.ignore(2);
    CHECK(is.gcount() == 2 && is.peek() == 'c');
    is.ignore(10, 'd');
    CHECK(is.gcount() == 2 && is.peek() == 'e');  // 'd' extracted
    is.ignore(10);
    CHECK(is.gcount() == 2 && is.eof() && !is.fail());
    std::istringstream z("abc");
    z.ignore(0);
    CHECK(z.gcount() == 0 && z.peek() == 'a');
    std::istringstream m("abc");
    m.ignore(std::numeric_limits<std::streamsize>::max());
    CHECK(m.gcount() == 3 && m.eof() && !m.fail());
  }
  {
    // ignore(n, char_type) with a char whose value is negative
    std::string s = "ab";
    s += static_cast<char>(-1);
    s += "cd";
    std::istringstream is(s);
    is.ignore(100, static_cast<char>(-1));
    CHECK(is.gcount() == 3 && is.peek() == 'c');
  }
  {
    // peek / read / readsome on a stream that is not good
    std::istringstream is("abc");
    is.setstate(std::ios_base::eofbit);
    CHECK(is.peek() == traits::eof());
    is.clear(std::ios_base::eofbit);
    char buf[4];
    is.read(buf, 2);
    CHECK(is.fail() && is.gcount() == 0);
    is.clear(std::ios_base::eofbit);
    CHECK(is.readsome(buf, 2) == 0 && is.fail());
  }
  {
    // readsome with in_avail() == -1: eofbit, nothing extracted, no failbit
    NoMore nm;
    std::istream is(&nm);
    char buf[4];
    CHECK(is.readsome(buf, 4) == 0);
    CHECK(is.eof() && !is.fail() && is.gcount() == 0);
    // readsome with in_avail() == 0
    std::stringbuf empty_buf("");
    std::istream e(&empty_buf);
    CHECK(e.readsome(buf, 4) == 0 && e.good());
  }
  {
    // putback / unget clear eofbit first
    std::istringstream is("ab");
    is.ignore(10);
    CHECK(is.eof() && !is.fail());
    is.unget();
    CHECK(is.good() && is.gcount() == 0);
    CHECK(is.get() == 'b');
    is.ignore(10);
    CHECK(is.eof());
    is.putback('b');
    CHECK(is.good() && is.gcount() == 0 && is.peek() == 'b');
    // failure: badbit
    std::istringstream r("xy");
    r.unget();
    CHECK(r.bad());
    std::istringstream q("xy", std::ios_base::in);
    q.get();
    q.putback('z');  // read-only buffer, different character
    CHECK(q.bad());
    // with failbit already set: failbit (and nothing else happens)
    std::istringstream f("xy");
    f.get();
    f.setstate(std::ios_base::failbit);
    f.unget();
    CHECK(f.fail() && !f.bad());
    f.clear();
    CHECK(f.get() == 'y');
  }
  {
    // an exception thrown by the stream buffer during unformatted input
    ThrowingSource src;
    std::istream is(&src);
    CHECK(is.get() == traits::eof());
    CHECK(is.bad());
    std::istream js(&src);
    js.exceptions(std::ios_base::badbit);
    int caught = 0;
    try {
      js.get();
    } catch (int e) {
      caught = e;
    } catch (...) {
      caught = -1;
    }
    CHECK(caught == 7 && js.bad());
    // formatted input likewise ([istream.formatted.reqmts]/1); noskipws keeps the sentry
    // from reading, so the exception is thrown by the extraction itself
    std::istream ks(&src);
    ks.unsetf(std::ios_base::skipws);
    int n = 0;
    ks >> n;
    CHECK(ks.bad());
    std::istream ls(&src);
    ls.unsetf(std::ios_base::skipws);
    ls.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    caught = 0;
    try {
      ls >> n;
    } catch (int e) {
      caught = e;
    } catch (...) {
      caught = -1;
    }
    CHECK(caught == 7 && ls.bad());
  }
  return 0;
}
