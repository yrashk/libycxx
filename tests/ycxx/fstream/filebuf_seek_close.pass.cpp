// [filebuf.members]/8-10: close() flushes the put area with overflow(eof()), closes the file
// even if that fails, and returns this on success; [filebuf.cons]/5: the destructor calls
// close(). [filebuf.virtuals]/13-14: seekoff fails when is_open() is false, returning
// pos_type(off_type(-1)); otherwise it flushes pending output and seeks as fseek(file, off,
// whence) with whence from Table 147 (beg SEEK_SET, cur SEEK_CUR, end SEEK_END); /16-18:
// seekpos(sp) returns sp on success and fails when the file is not open. /19: sync() writes the
// put area to the file ("then flushes the file as if by calling fflush"). /11: overflow fails
// when the file is not open. /7: pbackfail always fails when the file is not open.
// [filebuf.cons]/3-4: the move constructor transfers the file; the source is no longer open.
// [filebuf.assign]: move assignment closes the target's file first and transfers; swap.
// COUNTERPART: libcxx:input.output/file.streams/fstreams/filebuf.members/close.pass.cpp
// COUNTERPART: libcxx:input.output/file.streams/fstreams/filebuf.virtuals/seekoff.pass.cpp
#include <fstream>
#include <string>
#include <utility>
#include "fs_tmpdir.hpp"
#include "check.hpp"

using std::ios_base;
using traits = std::char_traits<char>;
static const std::streampos bad(std::streamoff(-1));

int main() {
  TmpDir dir;
  {
    std::filebuf fb;  // not open
    CHECK(fb.pubseekoff(0, ios_base::beg) == bad);
    CHECK(fb.pubseekoff(0, ios_base::cur) == bad);
    CHECK(fb.pubseekpos(0) == bad);
    CHECK(fb.sputc('a') == traits::eof());
    CHECK(fb.sgetc() == traits::eof());
    CHECK(fb.sputbackc('a') == traits::eof());
  }
  {
    const std::string p = dir / "seek";
    std::filebuf fb;
    CHECK(fb.open(p, ios_base::in | ios_base::out | ios_base::trunc) == &fb);
    CHECK(fb.sputn("0123456789", 10) == 10);
    CHECK(fb.pubseekoff(0, ios_base::cur) == std::streampos(10));
    CHECK(fb.pubseekoff(3, ios_base::beg) == std::streampos(3));
    CHECK(fb.sgetc() == '3');
    CHECK(fb.pubseekoff(-2, ios_base::end) == std::streampos(8));
    CHECK(fb.sbumpc() == '8');
    CHECK(fb.pubseekoff(-4, ios_base::cur) == std::streampos(5));
    CHECK(fb.sgetc() == '5');
    std::streampos here = fb.pubseekoff(0, ios_base::cur);
    CHECK(here == std::streampos(5));
    CHECK(fb.pubseekoff(0, ios_base::beg) == std::streampos(0));
    CHECK(fb.pubseekpos(here) == here);
    CHECK(fb.sgetc() == '5');
    // writing after a seek overwrites in place
    CHECK(fb.pubseekpos(here) == here);
    CHECK(fb.sputc('X') == 'X');
    CHECK(fb.pubseekoff(0, ios_base::beg) == std::streampos(0));
    char buf[11] = {};
    CHECK(fb.sgetn(buf, 10) == 10);
    CHECK(std::string(buf) == "01234X6789");
    CHECK(fb.sgetc() == traits::eof());
    // a seek before the beginning fails
    CHECK(fb.pubseekoff(-1, ios_base::beg) == bad);
    CHECK(fb.close() == &fb);
    CHECK(read_file(p) == "01234X6789");
  }
  {
    // pending output reaches the file on sync(), and on close()
    const std::string p = dir / "flush";
    std::filebuf fb;
    fb.open(p, ios_base::out);
    fb.sputn("abc", 3);
    CHECK(fb.pubsync() == 0);
    CHECK(read_file(p) == "abc");
    fb.sputn("def", 3);
    CHECK(fb.close() == &fb);
    CHECK(read_file(p) == "abcdef");
  }
  {
    // the destructor closes and flushes
    const std::string p = dir / "dtor";
    {
      std::filebuf fb;
      fb.open(p, ios_base::out);
      fb.sputn("xyz", 3);
    }
    CHECK(read_file(p) == "xyz");
  }
  {
    // a successful pbackfail of the same character while open
    const std::string p = dir / "pb";
    write_file(p, "ab");
    std::filebuf fb;
    fb.open(p, ios_base::in);
    CHECK(fb.sbumpc() == 'a');
    CHECK(fb.sungetc() == 'a');
    CHECK(fb.sbumpc() == 'a' && fb.sbumpc() == 'b');
    CHECK(fb.sputbackc('b') == 'b');
    CHECK(fb.sgetc() == 'b');
  }
  {
    // move construction and assignment
    const std::string p = dir / "mv", q = dir / "mv2";
    write_file(p, "move me");
    std::filebuf a;
    a.open(p, ios_base::in);
    CHECK(a.sbumpc() == 'm');
    std::filebuf b(std::move(a));
    CHECK(b.is_open() && !a.is_open());
    CHECK(b.sbumpc() == 'o');  // same position
    std::filebuf c;
    c.open(q, ios_base::out);
    c.sputn("pending", 7);
    c = std::move(b);  // closes c's file first (flushing it)
    CHECK(read_file(q) == "pending");
    CHECK(c.is_open() && !b.is_open());
    CHECK(c.sbumpc() == 'v');
    std::filebuf d;
    d.swap(c);
    CHECK(d.is_open() && !c.is_open() && d.sbumpc() == 'e');
    swap(c, d);
    CHECK(c.is_open() && !d.is_open() && c.sbumpc() == ' ');
  }
  return 0;
}
