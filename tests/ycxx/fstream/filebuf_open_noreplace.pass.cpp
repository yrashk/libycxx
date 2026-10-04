// [filebuf.members]/3, Table 146 (P2467, C++23 ios_base::noreplace): out|noreplace and
// out|trunc|noreplace -> "wx", in|out|trunc|noreplace -> "w+x", and with binary "wbx" (both
// out|noreplace and out|trunc|noreplace) and "w+bx": exclusive creation, failing when the file
// exists. Other combinations with noreplace are not in the table: "the open fails".
// [ios.openmode]: noreplace is a distinct openmode bit.
#include <fstream>
#include <ios>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

using std::ios_base;

static TmpDir dir;
static int counter = 0;

static bool try_open(ios_base::openmode mode, bool exists, std::string& content) {
  std::string p = dir / ("f" + std::to_string(counter++));
  if (exists) write_file(p, "old");
  std::filebuf fb;
  bool ok = fb.open(p.c_str(), mode) != nullptr;
  if (ok) {
    CHECK(fb.sputn("XY", 2) == 2);
    CHECK(fb.close() == &fb);
  }
  content = read_file(p);
  return ok;
}

int main() {
  const auto in = ios_base::in, out = ios_base::out, trunc = ios_base::trunc, app = ios_base::app,
             nr = ios_base::noreplace;
  static_assert((ios_base::noreplace & (ios_base::in | ios_base::out | ios_base::trunc | ios_base::app |
                                        ios_base::binary | ios_base::ate)) == ios_base::openmode{});
  std::string c;
  for (ios_base::openmode b : {ios_base::openmode{}, ios_base::binary}) {
    CHECK(try_open(b | out | nr, false, c) && c == "XY");
    CHECK(!try_open(b | out | nr, true, c) && c == "old");
    CHECK(try_open(b | out | trunc | nr, false, c) && c == "XY");
    CHECK(!try_open(b | out | trunc | nr, true, c) && c == "old");
    CHECK(try_open(b | in | out | trunc | nr, false, c) && c == "XY");
    CHECK(!try_open(b | in | out | trunc | nr, true, c) && c == "old");
    // not in the table
    CHECK(!try_open(b | nr, false, c) && c == "<missing>");
    CHECK(!try_open(b | in | nr, true, c) && c == "old");
    CHECK(!try_open(b | out | app | nr, false, c) && c == "<missing>");
    CHECK(!try_open(b | in | out | nr, true, c) && c == "old");
  }
  // through ofstream (adds out)
  std::string p = dir / "of";
  {
    std::ofstream o(p, ios_base::noreplace);
    CHECK(o.is_open());
  }
  std::ofstream o2(p, ios_base::noreplace);
  CHECK(!o2.is_open() && o2.fail());
  return 0;
}
