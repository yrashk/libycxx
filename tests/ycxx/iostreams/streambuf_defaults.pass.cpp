// [streambuf.cons]/1: the default constructor sets all six pointers to null and getloc() to a
// copy of the global locale. [streambuf.virt.buffer]: setbuf "Does nothing. Returns this";
// seekoff / seekpos return pos_type(off_type(-1)); sync returns zero.
// [streambuf.virt.get]/2: showmanyc returns zero; /13: underflow returns traits::eof(); /16:
// uflow "Calls underflow(). If underflow() returns traits::eof(), returns traits::eof()".
// [streambuf.virt.pback]/5: pbackfail returns traits::eof(). [streambuf.virt.put]/6: overflow
// returns traits::eof(). [streambuf.virt.locales]: imbue does nothing.
// Observed through the public members ([streambuf.members]) of a derived class that overrides
// nothing: sgetc / sbumpc / snextc / sputc / sputbackc / sungetc / sgetn / sputn fail with eof()
// or 0, in_avail() is showmanyc() (0) when no read position is available ([streambuf.pub.get]/1),
// pubseekoff / pubseekpos / pubsetbuf / pubsync forward to the virtuals ([streambuf.pub.buffer]).
#include <streambuf>
#include <locale>
#include <ios>
#include "check.hpp"

struct Plain : std::streambuf {
  bool all_null() const {
    return eback() == nullptr && gptr() == nullptr && egptr() == nullptr && pbase() == nullptr &&
           pptr() == nullptr && epptr() == nullptr;
  }
  using std::streambuf::setbuf;
  using std::streambuf::showmanyc;
  using std::streambuf::underflow;
  using std::streambuf::uflow;
  using std::streambuf::pbackfail;
  using std::streambuf::overflow;
  using std::streambuf::sync;
};

struct WPlain : std::wstreambuf {
  bool all_null() const {
    return eback() == nullptr && gptr() == nullptr && egptr() == nullptr && pbase() == nullptr &&
           pptr() == nullptr && epptr() == nullptr;
  }
};

int main() {
  using traits = std::char_traits<char>;
  const auto eof = traits::eof();
  const std::streampos bad(std::streamoff(-1));

  Plain p;
  CHECK(p.all_null());
  CHECK(p.getloc() == std::locale());
  CHECK(p.setbuf(nullptr, 0) == &p);
  CHECK(p.pubsetbuf(nullptr, 0) == &p);
  CHECK(p.showmanyc() == 0);
  CHECK(p.in_avail() == 0);
  CHECK(p.underflow() == eof);
  CHECK(p.uflow() == eof);
  CHECK(p.pbackfail() == eof);
  CHECK(p.pbackfail('x') == eof);
  CHECK(p.overflow() == eof);
  CHECK(p.overflow('x') == eof);
  CHECK(p.sync() == 0);
  CHECK(p.pubsync() == 0);
  CHECK(p.pubseekoff(0, std::ios_base::beg) == bad);
  CHECK(p.pubseekoff(3, std::ios_base::cur, std::ios_base::in) == bad);
  CHECK(p.pubseekpos(0) == bad);
  CHECK(p.pubseekpos(0, std::ios_base::out) == bad);

  CHECK(p.sgetc() == eof);
  CHECK(p.sbumpc() == eof);
  CHECK(p.snextc() == eof);
  CHECK(p.sputc('a') == eof);
  CHECK(p.sputbackc('a') == eof);
  CHECK(p.sungetc() == eof);
  char buf[4] = {'1', '2', '3', '4'};
  CHECK(p.sgetn(buf, 4) == 0);
  CHECK(buf[0] == '1');  // nothing assigned
  CHECK(p.sputn("abc", 3) == 0);
  CHECK(p.all_null());

  // [streambuf.locales]/1-2: pubimbue calls imbue(loc) and returns the previous getloc();
  // afterwards getloc() returns loc.
  std::locale l2(std::locale::classic(), new std::numpunct<char>);
  std::locale prev = p.pubimbue(l2);
  CHECK(prev == std::locale());
  CHECK(p.getloc() == l2);

  WPlain w;
  CHECK(w.all_null());
  CHECK(w.sgetc() == std::char_traits<wchar_t>::eof());
  CHECK(w.sputc(L'x') == std::char_traits<wchar_t>::eof());
  return 0;
}
