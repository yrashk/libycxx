// [fs.op.create.directories]/1: "Calls create_directory for each element of p that does not
// exist"; /2: returns true if a new directory was created. Nothing limits the number of elements:
// a path of 1100 elements (about 2200 characters, below POSIX PATH_MAX = 4096 on Linux) works
// like a short one, and the deepest directory then exists ([fs.op.is.dir]).
#include <filesystem>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir t;
  fs::path p = t.str();
  for (int i = 0; i < 1100; ++i) p /= "d";
  CHECK(p.native().size() < 4000);
  std::error_code ec;
  CHECK(fs::create_directories(p, ec) && !ec);
  CHECK(fs::is_directory(p));
  CHECK(!fs::create_directories(p, ec) && !ec);  // nothing new
  fs::path q = p / "e" / "f";
  CHECK(fs::create_directories(q));
  CHECK(fs::is_directory(q));
  return 0;
}
