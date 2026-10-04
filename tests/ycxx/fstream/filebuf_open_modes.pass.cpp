// [filebuf.members]/3, Table 146: open(s, mode) opens the file "as if by a call to fopen with
// the second argument determined from mode & ~ios_base::ate": out / out|trunc -> "w",
// out|app / app -> "a", in -> "r", in|out -> "r+", in|out|trunc -> "w+", in|out|app /
// in|app -> "a+", and the same with binary (the noreplace rows: filebuf_open_noreplace); "If mode is not some combination of flags shown in the table then the open
// fails." /3: "If is_open() != false, returns a null pointer." /4: with ate, positions the file
// at the end. /6: returns this or a null pointer. /1: is_open(). /8-10: close() returns a null
// pointer if the file is not open; is_open() is false afterwards. [filebuf.cons]/2: a
// default-constructed filebuf is not open.
#include <fstream>
#include <ios>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

using std::ios_base;

static TmpDir dir;
static int counter = 0;

// Opens a fresh path (existing with "old" when `exists`) with `mode`, writes `w` if open for
// output, returns whether open succeeded and stores the resulting file content.
static bool try_open(ios_base::openmode mode, bool exists, std::string& content, const char* w = "XY") {
  std::string p = dir / ("f" + std::to_string(counter++));
  if (exists) write_file(p, "old");
  std::filebuf fb;
  CHECK(!fb.is_open());
  std::filebuf* r = fb.open(p.c_str(), mode);
  CHECK(r == nullptr || r == &fb);
  CHECK(fb.is_open() == (r != nullptr));
  if (r && (mode & (ios_base::out | ios_base::app))) {
    CHECK(fb.sputn(w, 2) == 2);
  }
  if (r) CHECK(fb.close() == &fb);
  CHECK(!fb.is_open());
  content = read_file(p);
  return r != nullptr;
}

int main() {
  const auto in = ios_base::in, out = ios_base::out, trunc = ios_base::trunc, app = ios_base::app,
             bin = ios_base::binary, ate = ios_base::ate;
  std::string c;
  for (ios_base::openmode b : {ios_base::openmode{}, bin}) {
    // "w": creates or truncates
    CHECK(try_open(b | out, false, c) && c == "XY");
    CHECK(try_open(b | out, true, c) && c == "XY");
    CHECK(try_open(b | out | trunc, true, c) && c == "XY");
    // "a": creates, appends
    CHECK(try_open(b | out | app, true, c) && c == "oldXY");
    CHECK(try_open(b | app, true, c) && c == "oldXY");
    CHECK(try_open(b | app, false, c) && c == "XY");
    // "r": fails if the file does not exist
    CHECK(!try_open(b | in, false, c) && c == "<missing>");
    CHECK(try_open(b | in, true, c) && c == "old");
    // "r+": no creation, no truncation, writes from the beginning
    CHECK(!try_open(b | in | out, false, c) && c == "<missing>");
    CHECK(try_open(b | in | out, true, c) && c == "XYd");
    // "w+": creates or truncates
    CHECK(try_open(b | in | out | trunc, true, c) && c == "XY");
    CHECK(try_open(b | in | out | trunc, false, c) && c == "XY");
    // "a+"
    CHECK(try_open(b | in | out | app, true, c) && c == "oldXY");
    CHECK(try_open(b | in | app, true, c) && c == "oldXY");
    CHECK(try_open(b | in | app, false, c) && c == "XY");
    // combinations not in the table fail (and do not create the file)
    CHECK(!try_open(b | trunc, false, c) && c == "<missing>");
    CHECK(!try_open(b | in | trunc, true, c) && c == "old");
    CHECK(!try_open(b | out | trunc | app, true, c) && c == "old");
    CHECK(!try_open(b | in | out | trunc | app, true, c) && c == "old");
    CHECK(!try_open(b, true, c) && c == "old");
    CHECK(!try_open(b | ate, true, c) && c == "old");  // ate is ignored: mode & ~ate is b
  }
  // ate: the file is positioned at the end after opening
  {
    std::string p = dir / "ate";
    write_file(p, "12345");
    std::filebuf fb;
    CHECK(fb.open(p, in | ate) == &fb);
    CHECK(fb.pubseekoff(0, ios_base::cur, in) == std::streampos(5));
    CHECK(fb.sgetc() == std::char_traits<char>::eof());
    fb.close();
    CHECK(fb.open(p, in | out | ate) == &fb);
    CHECK(fb.sputn("67", 2) == 2);
    fb.close();
    CHECK(read_file(p) == "1234567");
    CHECK(fb.open(p, out | ate) == &fb);  // "w": truncated, the end is 0
    CHECK(fb.pubseekoff(0, ios_base::cur, out) == std::streampos(0));
    fb.close();
    CHECK(read_file(p) == "");
  }
  // open while open fails and keeps the first file; close when not open fails
  {
    std::string p1 = dir / "a1", p2 = dir / "a2";
    write_file(p1, "one");
    write_file(p2, "two");
    std::filebuf fb;
    CHECK(fb.close() == nullptr);
    CHECK(fb.open(p1, in) == &fb);
    CHECK(fb.open(p2, in) == nullptr);
    CHECK(fb.is_open());
    CHECK(fb.sgetc() == 'o');
    CHECK(fb.close() == &fb);
    CHECK(fb.close() == nullptr);
    CHECK(!fb.is_open());
    // reopening after close works
    CHECK(fb.open(std::string(p2), in) == &fb && fb.sgetc() == 't');
  }
  // a path in a directory that does not exist
  {
    std::filebuf fb;
    CHECK(fb.open((dir / "no/such/dir").c_str(), out) == nullptr && !fb.is_open());
  }
  return 0;
}
