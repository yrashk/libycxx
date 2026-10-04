// [basic.ios.members]/2-4: tie(tiestr) sets tie() and returns the previous value. /6-8:
// rdbuf(sb) calls clear() (so a null sb leaves badbit, [iostate.flags]) and returns the previous
// rdbuf(). /9-10: imbue(loc) calls ios_base::imbue(loc) and rdbuf()->pubimbue(loc) if rdbuf() is
// not null, returning the prior locale. /11-12: narrow / widen use the ctype facet of getloc().
// /13-15: fill(c) returns the previous fill. /20: move(rhs): *this gets rhs's state except that
// rdbuf() returns nullptr; rhs.rdbuf() is unchanged and rhs.tie() is nullptr. /21: swap
// exchanges the states except rdbuf(). /22-24: set_rdbuf(sb) associates sb "without calling
// clear()". (move / swap / set_rdbuf are protected: called from a derived class.)
#include <ios>
#include <istream>
#include <ostream>
#include <sstream>
#include <locale>
#include "check.hpp"

struct Tracking : std::stringbuf {
  int imbues = 0;
  std::locale last;
  void imbue(const std::locale& l) override {
    ++imbues;
    last = l;
  }
};

struct Stream : std::basic_ios<char> {
  Stream() { init(nullptr); }
  explicit Stream(std::streambuf* sb) { init(sb); }
  using std::basic_ios<char>::move;
  using std::basic_ios<char>::swap;
  using std::basic_ios<char>::set_rdbuf;
};

struct UpperWiden : std::ctype<char> {
  char do_widen(char c) const override { return c == 'a' ? 'A' : c; }
  char do_narrow(char c, char d) const override { return c == 'Z' ? d : c; }
};

int main() {
  std::stringbuf sb1, sb2;
  std::ostringstream t1, t2;
  {
    Stream s(&sb1);
    CHECK(s.tie() == nullptr);
    CHECK(s.tie(&t1) == nullptr && s.tie() == &t1);
    CHECK(s.tie(&t2) == &t1 && s.tie() == &t2);
    CHECK(s.tie(nullptr) == &t2 && s.tie() == nullptr);

    s.setstate(std::ios_base::failbit | std::ios_base::eofbit);
    CHECK(s.rdbuf(&sb2) == &sb1);
    CHECK(s.rdbuf() == &sb2 && s.good());  // clear()
    CHECK(s.rdbuf(nullptr) == &sb2);
    CHECK(s.rdstate() == std::ios_base::badbit);  // clear() with a null rdbuf()
    s.set_rdbuf(&sb1);
    CHECK(s.rdbuf() == &sb1 && s.bad());  // no clear()

    CHECK(s.fill() == ' ');
    CHECK(s.fill('*') == ' ' && s.fill() == '*');
  }
  {
    Tracking tb;
    Stream s(&tb);
    std::locale l2(std::locale::classic(), new UpperWiden);
    std::locale prev = s.imbue(l2);
    CHECK(prev == std::locale());
    CHECK(s.getloc() == l2 && tb.imbues == 1 && tb.last == l2);
    CHECK(s.widen('a') == 'A' && s.widen('b') == 'b');
    CHECK(s.narrow('Z', '?') == '?' && s.narrow('y', '?') == 'y');
    CHECK(s.imbue(std::locale::classic()) == l2);
    Stream n;  // null rdbuf(): only ios_base::imbue
    CHECK(n.imbue(l2) == std::locale() && n.getloc() == l2);
  }
  {
    // move: everything but rdbuf()
    std::ostringstream tie_target;
    Stream a(&sb1);
    a.tie(&tie_target);
    a.flags(std::ios_base::hex | std::ios_base::showbase);
    a.width(7);
    a.precision(3);
    a.fill('#');
    a.setstate(std::ios_base::eofbit);
    a.exceptions(std::ios_base::badbit);
    int idx = std::ios_base::xalloc();
    a.iword(idx) = 77;
    Stream b(&sb2);
    b.move(a);
    CHECK(b.rdbuf() == nullptr);
    CHECK(b.tie() == &tie_target && b.flags() == (std::ios_base::hex | std::ios_base::showbase));
    CHECK(b.width() == 7 && b.precision() == 3 && b.fill() == '#');
    CHECK(b.rdstate() == std::ios_base::eofbit && b.exceptions() == std::ios_base::badbit);
    CHECK(b.iword(idx) == 77);
    CHECK(a.rdbuf() == &sb1 && a.tie() == nullptr);

    // swap: everything but rdbuf()
    Stream c(&sb2);
    c.fill('-');
    b.swap(c);
    CHECK(b.rdbuf() == nullptr && c.rdbuf() == &sb2);
    CHECK(b.fill() == '-' && b.tie() == nullptr && b.width() == 0 && b.iword(idx) == 0);
    CHECK(c.fill() == '#' && c.tie() == &tie_target && c.width() == 7 && c.iword(idx) == 77);
    CHECK(c.rdstate() == std::ios_base::eofbit && b.rdstate() == std::ios_base::goodbit);
    static_assert(noexcept(b.swap(c)));
  }
  return 0;
}
