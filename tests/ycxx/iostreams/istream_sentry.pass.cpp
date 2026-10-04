// [istream.sentry]/2: "If is.good() is false, calls is.setstate(failbit)." Otherwise, if
// is.tie() is not null, calls is.tie()->flush() (which may be suppressed when the tied put area
// is empty, or deferred until is.rdbuf()->underflow() is called); "If noskipws is zero and
// is.flags() & ios_base::skipws is nonzero, the function extracts and discards each character
// as long as the next available input character c is a whitespace character. If
// is.rdbuf()->sbumpc() or is.rdbuf()->sgetc() returns traits::eof(), the function calls
// setstate(failbit | eofbit)". /3-4: whitespace is decided by ctype<charT>::is(space, c) of the
// imbued locale. /5: the sentry converts to is.good() after preparation; /7 operator bool.
// [istream.formatted.reqmts]/1, [istream.unformatted]/1: formatted input skips white space,
// unformatted input does not.
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <locale>
#include <string>
#include <type_traits>
#include "check.hpp"

using traits = std::char_traits<char>;

// A source that only provides characters through underflow() (empty get area initially).
struct Source : std::streambuf {
  std::string s;
  std::size_t pos = 0;
  char c;
  bool underflowed = false;
  explicit Source(std::string str) : s(std::move(str)) {}
  int_type underflow() override {
    underflowed = true;
    if (pos == s.size()) return traits::eof();
    c = s[pos++];
    setg(&c, &c, &c + 1);
    return traits::to_int_type(c);
  }
};

// A sink whose put area holds the characters until sync().
struct Sink : std::streambuf {
  char buf[64];
  std::string flushed;
  int syncs = 0;
  bool underflow_seen_flushed = false;
  Sink() { setp(buf, buf + 64); }
  int sync() override {
    ++syncs;
    flushed.append(pbase(), pptr());
    setp(buf, buf + 64);
    return 0;
  }
};

// '_' is classified as white space, ' ' is not.
struct UnderscoreSpace : std::ctype<char> {
  static const mask* make_table() {
    static mask t[table_size];
    const mask* c = classic_table();
    for (std::size_t i = 0; i < table_size; ++i) t[i] = c[i];
    t[static_cast<unsigned char>('_')] = space;
    t[static_cast<unsigned char>(' ')] = punct | print;
    return t;
  }
  UnderscoreSpace() : std::ctype<char>(make_table()) {}
};

int main() {

  {
    std::istringstream is("  \t\n x y");
    std::istream::sentry s(is);
    CHECK(static_cast<bool>(s));
    CHECK(is.peek() == 'x');
  }
  {
    std::istringstream is("  x");
    std::istream::sentry s(is, true);  // noskipws argument
    CHECK(static_cast<bool>(s) && is.peek() == ' ');
  }
  {
    std::istringstream is("  x");
    is.unsetf(std::ios_base::skipws);
    std::istream::sentry s(is);
    CHECK(static_cast<bool>(s) && is.peek() == ' ');
  }
  {
    // only white space: failbit | eofbit
    std::istringstream is(" \n\t ");
    std::istream::sentry s(is);
    CHECK(!s);
    CHECK(is.rdstate() == (std::ios_base::failbit | std::ios_base::eofbit));
  }
  {
    // empty input with skipws: eof reached while skipping
    std::istringstream is("");
    std::istream::sentry s(is);
    CHECK(!s && is.eof() && is.fail() && !is.bad());
  }
  {
    // empty input with noskipws: nothing is read, the stream stays good
    std::istringstream is("");
    std::istream::sentry s(is, true);
    CHECK(static_cast<bool>(s) && is.good());
  }
  {
    // not good: failbit is set, the state is kept otherwise
    std::istringstream is("abc");
    is.setstate(std::ios_base::eofbit);
    std::istream::sentry s(is, true);
    CHECK(!s);
    CHECK(is.rdstate() == (std::ios_base::eofbit | std::ios_base::failbit));
    CHECK(is.rdbuf()->sgetc() == 'a');  // nothing extracted
  }
  {
    // the failbit | eofbit from skipping may throw
    std::istringstream is("   ");
    is.exceptions(std::ios_base::eofbit);
    bool thrown = false;
    try {
      std::istream::sentry s(is);
    } catch (const std::ios_base::failure&) {
      thrown = true;
    }
    CHECK(thrown && is.eof() && is.fail());
  }
  {
    // the imbued ctype facet decides what is white space
    std::istringstream is("__ _x");
    is.imbue(std::locale(std::locale::classic(), new UnderscoreSpace));
    std::istream::sentry s(is);
    CHECK(static_cast<bool>(s) && is.peek() == ' ');
    std::string w;
    is >> w;
    CHECK(w == " ");  // stops at '_'
  }
  {
    // formatted input skips, unformatted input does not
    std::istringstream is("   7   8");
    int n = 0;
    is >> n;
    CHECK(n == 7);
    CHECK(is.get() == ' ');
    char c = 0;
    is >> c;
    CHECK(c == '8');
  }
  {
    // tie(): the tied stream is flushed before input that needs underflow()
    Sink sk;
    std::ostream out(&sk);
    Source src("42");
    std::istream in(&src);
    CHECK(in.tie() == nullptr);
    in.tie(&out);
    out << "prompt";
    CHECK(sk.flushed.empty());
    int n = 0;
    in >> n;
    CHECK(n == 42 && src.underflowed);
    CHECK(sk.syncs >= 1 && sk.flushed == "prompt");
    // the same for unformatted input
    out << "again";
    Source src2("z");
    in.rdbuf(&src2);
    CHECK(in.get() == 'z');
    CHECK(sk.flushed == "promptagain");
  }
  return 0;
}
