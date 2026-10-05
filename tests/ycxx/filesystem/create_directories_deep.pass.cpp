// [fs.op.create.directories]/1: "Calls create_directory for each element of p that does not
// exist"; /2: returns true if a new directory was created. Nothing limits the number of elements:
// a path of up to 1100 elements works like a short one, and the deepest directory then exists
// ([fs.op.is.dir]). create_directory works "as if by POSIX mkdir" ([fs.op.create.directory]/1),
// which may fail for a path of PATH_MAX bytes or more (ENAMETOOLONG), so the path stays below
// the platform's PATH_MAX (<limits.h>: 4096 on Linux, where 1100 elements fit; 1024 on Darwin,
// whose $TMPDIR is long too).
#include <filesystem>
#include <limits.h>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir t;
  fs::path p = t.str();
  const std::size_t room = PATH_MAX - 1 - t.str().size() - 4;  // "/e/f" below
  const std::size_t depth = room / 2 < 1100 ? room / 2 : 1100;   // "/d" each
  CHECK(depth >= 400);
  for (std::size_t i = 0; i < depth; ++i) p /= "d";
  CHECK(p.native().size() + 4 < PATH_MAX);
  std::error_code ec;
  CHECK(fs::create_directories(p, ec) && !ec);
  CHECK(fs::is_directory(p));
  CHECK(!fs::create_directories(p, ec) && !ec);  // nothing new
  fs::path q = p / "e" / "f";
  CHECK(fs::create_directories(q));
  CHECK(fs::is_directory(q));
  return 0;
}
