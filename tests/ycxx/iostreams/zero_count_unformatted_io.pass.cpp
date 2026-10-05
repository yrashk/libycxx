// Unformatted input and output with a count of zero (or less) and a null array pointer, and
// streams over empty character sequences.
//   [istream.unformatted]/8-9 get(s, n, delim): characters are stored until "n is less than one
//     or n - 1 characters are stored"; "If the function stores no characters, failbit is set";
//     "if n is greater than zero it then stores a null character". So get(s, 0) stores nothing
//     (s may be null), sets failbit and extracts nothing.
//   /18-21 getline(s, n, delim): the conditions are tested in the order end-of-file, delimiter
//     ("extracted but not stored"), n < 1 or n - 1 stored ("calls setstate(failbit)"): with a
//     delimiter next, getline(s, 0) extracts it (gcount() == 1) and does not fail; otherwise it
//     sets failbit and extracts nothing; nothing is stored when n <= 0.
//   /? read(s, 0): stores until "n characters are stored" -> nothing, no state change;
//     readsome(s, 0) returns 0; ignore(0) extracts nothing.
//   [ostream.unformatted] write(s, 0) inserts nothing and leaves the stream good.
//   [streambuf.pub.get]/[streambuf.virt.get]: sgetn(s, 0) = xsgetn(s, 0) returns 0;
//     [streambuf.virt.put] sputn(s, 0) returns 0.
//   [stringbuf.virtuals]/15: setbuf(0, 0) has no effect.
//   [spanbuf.virtuals]/[spanbuf.cons]: a spanbuf over an empty span: output fails (overflow
//     returns eof: [streambuf.virt.put]) and input reaches end-of-file at once.
#include <istream>
#include <ostream>
#include <span>
#include <spanstream>
#include <sstream>
#include <string>
#include "check.hpp"

template <class C>
void case_for() {
  using S = std::basic_string<C>;
  auto str = [](const char* s) {
    S r;
    for (; *s; ++s) r.push_back(C(*s));
    return r;
  };
  C buf[4] = {C('#'), C('#'), C('#'), C('#')};

  {
    std::basic_istringstream<C> in(str("abc\n"));
    in.get(nullptr, 0);
    CHECK(in.rdstate() == std::ios_base::failbit && in.gcount() == 0);
    in.clear();
    in.get(buf, 0);
    CHECK(in.rdstate() == std::ios_base::failbit && in.gcount() == 0 && buf[0] == C('#'));
    in.clear();
    in.get(buf, -5, C('x'));
    CHECK(in.rdstate() == std::ios_base::failbit && buf[0] == C('#'));
    in.clear();
    in.get(buf, 1);
    CHECK(in.rdstate() == std::ios_base::failbit && in.gcount() == 0 && buf[0] == C(0) && buf[1] == C('#'));
    in.clear();
    CHECK(in.get() == C('a'));
  }
  {
    std::basic_istringstream<C> in(str("abc"));
    in.getline(nullptr, 0);
    CHECK(in.rdstate() == std::ios_base::failbit && in.gcount() == 0);
    in.clear();
    buf[0] = C('#');
    in.getline(buf, 1);
    CHECK(in.rdstate() == std::ios_base::failbit && in.gcount() == 0 && buf[0] == C(0) && buf[1] == C('#'));
    in.clear();
    CHECK(in.get() == C('a'));
  }
  {
    std::basic_istringstream<C> in(str("\nx"));
    buf[0] = C('#');
    in.getline(buf, 0);
    CHECK(in.good() && in.gcount() == 1 && buf[0] == C('#'));
    CHECK(in.get() == C('x'));
  }
  {
    std::basic_istringstream<C> in(str("\nx"));
    in.getline(nullptr, -1, C('\n'));
    CHECK(in.good() && in.gcount() == 1);
  }
  {
    std::basic_istringstream<C> in;
    in.getline(nullptr, 0);
    CHECK(in.rdstate() == (std::ios_base::eofbit | std::ios_base::failbit) && in.gcount() == 0);
  }
  {
    std::basic_istringstream<C> in(str("ab"));
    CHECK(&in.read(nullptr, 0) == &in && in.good() && in.gcount() == 0);
    CHECK(in.readsome(nullptr, 0) == 0 && in.good() && in.gcount() == 0);
    CHECK(&in.ignore(0) == &in && in.good() && in.gcount() == 0);
    CHECK(in.rdbuf()->sgetn(nullptr, 0) == 0);
    CHECK(in.get() == C('a'));
  }
  {
    std::basic_ostringstream<C> out;
    CHECK(&out.write(nullptr, 0) == &out && out.good() && out.str().empty());
    CHECK(out.rdbuf()->sputn(nullptr, 0) == 0);
    out << str("xy");
    CHECK(out.rdbuf()->pubsetbuf(nullptr, 0) == out.rdbuf());
    out << C('z');
    CHECK(out.str() == str("xyz"));
  }
  {
    std::basic_stringbuf<C> sb(str("pq"));
    CHECK(sb.pubsetbuf(nullptr, 0) == &sb);
    CHECK(sb.sgetc() == C('p') && sb.str() == str("pq"));
  }
  {
    std::basic_ospanstream<C> os{std::span<C>()};
    CHECK(os.good());
    os.write(nullptr, 0);
    CHECK(os.good());
    os << C('x');
    CHECK(os.bad() && os.span().empty());
    std::basic_ispanstream<C> is{std::span<const C>()};
    int v = 7;
    is >> v;
    CHECK(is.eof() && is.fail() && v == 7);  // the sentry fails: num_get is not called
    std::basic_spanbuf<C> sb;
    CHECK(sb.span().empty() && sb.sputc(C('a')) == std::char_traits<C>::eof() &&
          sb.sgetc() == std::char_traits<C>::eof());
  }
}

int main() {
  case_for<char>();
  case_for<wchar_t>();
  return 0;
}
