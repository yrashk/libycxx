// [streambuf.get.area]: setg(gbeg, gnext, gend) makes eback() == gbeg, gptr() == gnext,
// egptr() == gend; gbump(n) adds n to gptr(). [streambuf.put.area]: setp(pbeg, pend) makes
// pbase() == pptr() == pbeg, epptr() == pend; pbump(n) adds n to pptr().
// [streambuf.pub.get]/1: in_avail() is egptr() - gptr() when a read position is available,
// otherwise showmanyc(); /2-5: snextc / sbumpc / sgetc use the get area and call uflow() /
// underflow() only when no read position is available. [streambuf.pub.pback]: sputbackc(c)
// backs up when c matches gptr()[-1], otherwise calls pbackfail(c); sungetc() calls pbackfail()
// with eof() only when no putback position is available. [streambuf.pub.put]/1: sputc stores at
// pptr() or calls overflow(c). [streambuf.cons]/3: the copy constructor copies the six pointers
// and the locale. [streambuf.assign]: operator= assigns them; swap exchanges them.
// [streambuf.cons]/1: getloc() is a copy of the global locale "at the time of construction".
#include <streambuf>
#include <locale>
#include <utility>
#include "check.hpp"

using traits = std::char_traits<char>;

struct Probe : std::streambuf {
  int under = 0, ufl = 0, pback = 0, over = 0, many = 0;
  int pback_arg = 0, over_arg = 0;
  Probe() = default;
  Probe(const Probe& o) : std::streambuf(o) {}
  Probe& operator=(const Probe& o) {
    std::streambuf::operator=(o);
    return *this;
  }
  void swap_with(Probe& o) { std::streambuf::swap(o); }

  using std::streambuf::eback;
  using std::streambuf::gptr;
  using std::streambuf::egptr;
  using std::streambuf::pbase;
  using std::streambuf::pptr;
  using std::streambuf::epptr;
  using std::streambuf::setg;
  using std::streambuf::setp;
  using std::streambuf::gbump;
  using std::streambuf::pbump;

protected:
  std::streamsize showmanyc() override { ++many; return 7; }
  int_type underflow() override { ++under; return traits::eof(); }
  int_type uflow() override { ++ufl; return traits::eof(); }
  int_type pbackfail(int_type c) override { ++pback; pback_arg = c; return traits::eof(); }
  int_type overflow(int_type c) override { ++over; over_arg = c; return traits::eof(); }
};

int main() {
  char g[] = "abcdef";
  char out[4] = {};
  Probe p;
  CHECK(p.in_avail() == 7 && p.many == 1);  // no read position: showmanyc()

  p.setg(g, g + 2, g + 6);
  CHECK(p.eback() == g && p.gptr() == g + 2 && p.egptr() == g + 6);
  CHECK(p.in_avail() == 4 && p.many == 1);
  CHECK(p.sgetc() == 'c' && p.gptr() == g + 2);
  CHECK(p.sbumpc() == 'c' && p.gptr() == g + 3);
  CHECK(p.snextc() == 'e' && p.gptr() == g + 4);
  p.gbump(-3);
  CHECK(p.gptr() == g + 1);
  p.gbump(2);
  CHECK(p.gptr() == g + 3);

  // putback within the buffer: the same character only backs up
  CHECK(p.sputbackc('c') == 'c' && p.gptr() == g + 2 && p.pback == 0);
  CHECK(p.sungetc() == 'b' && p.gptr() == g + 1 && p.pback == 0);
  // a different character: pbackfail(c), pointer unchanged
  CHECK(p.sputbackc('z') == traits::eof());
  CHECK(p.pback == 1 && p.pback_arg == 'z' && p.gptr() == g + 1);
  CHECK(p.sungetc() == 'a' && p.gptr() == g);
  // at eback(): no putback position, pbackfail() with eof()
  CHECK(p.sungetc() == traits::eof());
  CHECK(p.pback == 2 && p.pback_arg == traits::eof());
  CHECK(p.sputbackc('q') == traits::eof() && p.pback == 3 && p.pback_arg == 'q');

  // reaching egptr(): sgetc calls underflow, sbumpc calls uflow
  p.setg(g, g + 6, g + 6);
  CHECK(p.in_avail() == 7);  // gptr() == egptr(): showmanyc()
  CHECK(p.sgetc() == traits::eof() && p.under == 1 && p.ufl == 0);
  CHECK(p.sbumpc() == traits::eof() && p.ufl == 1);
  CHECK(p.snextc() == traits::eof() && p.ufl == 2 && p.under == 1);  // sbumpc failed: no sgetc

  // snextc at the last character: sbumpc succeeds, then sgetc calls underflow
  p.setg(g, g + 5, g + 6);
  CHECK(p.snextc() == traits::eof() && p.under == 2 && p.gptr() == g + 6);

  p.setp(out, out + 2);
  CHECK(p.pbase() == out && p.pptr() == out && p.epptr() == out + 2);
  CHECK(p.sputc('x') == 'x' && p.pptr() == out + 1 && out[0] == 'x');
  CHECK(p.sputc('y') == 'y' && p.pptr() == out + 2);
  CHECK(p.sputc('z') == traits::eof() && p.over == 1 && p.over_arg == 'z');
  CHECK(p.sputn("12", 2) == 0);  // xsputn stops when sputc would fail
  p.pbump(-2);
  CHECK(p.pptr() == out);
  CHECK(p.sputn("12", 2) == 2 && out[0] == '1' && out[1] == '2');

  // copy construction / assignment / swap
  p.setg(g, g + 1, g + 3);
  p.setp(out, out + 4);
  p.pbump(1);
  Probe c(p);
  CHECK(c.eback() == g && c.gptr() == g + 1 && c.egptr() == g + 3);
  CHECK(c.pbase() == out && c.pptr() == out + 1 && c.epptr() == out + 4);
  CHECK(c.getloc() == p.getloc());
  Probe d;
  d = p;
  CHECK(d.gptr() == g + 1 && d.pptr() == out + 1);
  Probe e;
  std::locale l2(std::locale::classic(), new std::numpunct<char>);
  e.pubimbue(l2);
  e.swap_with(d);
  CHECK(e.gptr() == g + 1 && e.pptr() == out + 1 && e.getloc() == p.getloc());
  CHECK(d.gptr() == nullptr && d.pbase() == nullptr);
  CHECK(d.getloc() == l2);

  // the locale is the global locale at the time of construction
  std::locale old = std::locale::global(l2);
  Probe f;
  std::locale::global(old);
  CHECK(f.getloc() == l2);
  Probe h;
  CHECK(h.getloc() == old);
  return 0;
}
