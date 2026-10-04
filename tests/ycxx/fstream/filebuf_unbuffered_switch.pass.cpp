// [filebuf.virtuals]/12: "If setbuf(0, 0) is called on a stream before any I/O has occurred on
// that stream, the stream becomes unbuffered. ... "Unbuffered" means that pbase() and pptr()
// always return null and output to the file should appear as soon as possible."
// [filebuf.virtuals]/13-14 (seekoff): "Alters the file position, if possible ... If the stream
// is opened for output and the last operation was output, the put area is flushed first
// ('as if by calling overflow(traits::eof())'); the get area and position are adjusted so
// that the reported position is the logical one; [filebuf.general]/2: "the restrictions on
// reading and writing a sequence controlled by an object of class basic_filebuf<charT,
// traits> are the same as for reading and writing with the C standard library FILEs" -- in
// particular, switching between reading and writing is done with a positioning call, which
// is what seekg/seekp do. Table 146: in|out|app is "a+" (every write appends).
// [istream.unformatted] tellg()/[ostream.seeks] tellp() return rdbuf()->pubseekoff(0, cur, ...).
#include <fstream>
#include <ios>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

struct Exposed : std::filebuf {
  char* pb() const { return pbase(); }
  char* pp() const { return pptr(); }
};

int main() {
  TmpDir dir;
  using B = std::ios_base;

  {  // unbuffered output appears in the file at once
    const std::string p = dir / "unbuf";
    Exposed fb;
    CHECK(fb.pubsetbuf(nullptr, 0) == &fb);
    CHECK(fb.open(p, B::out) == &fb);
    CHECK(fb.sputc('a') == 'a');
    CHECK(fb.pb() == nullptr && fb.pp() == nullptr);
    CHECK(read_file(p) == "a");
    CHECK(fb.sputn("bcd", 3) == 3);
    CHECK(fb.pb() == nullptr && fb.pp() == nullptr);
    CHECK(read_file(p) == "abcd");
    CHECK(fb.pubseekoff(0, B::cur, B::out) == std::streampos(4));
    CHECK(fb.pubseekpos(1, B::out) == std::streampos(1));
    CHECK(fb.sputc('X') == 'X');
    CHECK(read_file(p) == "aXcd");
    CHECK(fb.close() == &fb);
  }
  {  // unbuffered input reads correctly and leaves the file position logical
    const std::string p = dir / "unbuf_in";
    write_file(p, "hello");
    std::filebuf fb;
    fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(p, B::in) == &fb);
    CHECK(fb.sgetc() == 'h');
    CHECK(fb.sgetc() == 'h');
    CHECK(fb.sbumpc() == 'h');
    CHECK(fb.sbumpc() == 'e');
    CHECK(fb.pubseekoff(0, B::cur, B::in) == std::streampos(2));
    char buf[8] = {};
    CHECK(fb.sgetn(buf, 8) == 3 && std::string(buf) == "llo");
    CHECK(fb.sgetc() == std::char_traits<char>::eof());
  }
  {  // read / write switches through positioning, with a buffered stream
    const std::string p = dir / "rw";
    std::fstream f(p, B::in | B::out | B::trunc);
    CHECK(f.is_open());
    f << "hello world";
    CHECK(f.tellp() == std::streampos(11));
    f.seekg(0);
    char c3[4] = {};
    f.read(c3, 3);
    CHECK(std::string(c3) == "hel");
    CHECK(f.tellg() == std::streampos(3));  // logical position, not the buffered one
    f.seekp(f.tellg());
    f << 'X';
    CHECK(f.tellp() == std::streampos(4));
    f.seekg(6);
    std::string w;
    f >> w;
    CHECK(w == "world");
    f.clear();
    f.seekp(0, B::beg);
    f << "HE";
    f.seekg(0);
    std::string line;
    std::getline(f, line);
    CHECK(line == "HElXo world");
    f.clear();
    f.seekp(0, B::end);
    CHECK(f.tellp() == std::streampos(11));
    f << "!";
    f.seekg(-3, B::end);
    std::getline(f, line);
    CHECK(line == "ld!");
    f.close();
    CHECK(read_file(p) == "HElXo world!");
  }
  {  // in|out|app: every write goes to the end
    const std::string p = dir / "app";
    write_file(p, "abc");
    std::fstream f(p, B::in | B::out | B::app);
    CHECK(f.is_open());
    f.seekp(0);
    f << 'X';
    f.flush();
    CHECK(read_file(p) == "abcX");
    f.seekg(0);
    std::string s;
    f >> s;
    CHECK(s == "abcX");
  }
  return 0;
}
