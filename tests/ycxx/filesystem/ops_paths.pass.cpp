// [fs.op.temp.dir.path]: temp_directory_path() is an existing directory (here: $TMPDIR when set
// to a directory). [fs.op.current.path]: current_path() is absolute; current_path(p) changes it.
// [fs.op.absolute]: absolute(p) for a relative p is current_path() / p (POSIX); an absolute p
// is unchanged. [fs.op.relative], [fs.op.proximate]: relative(p, base) is
// weakly_canonical(p).lexically_relative(weakly_canonical(base)).
#include <filesystem>
#include <cstdlib>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir tmp;
  setenv("TMPDIR", tmp.str().c_str(), 1);
  CHECK(fs::temp_directory_path() == fs::path(tmp.str()));
  CHECK(fs::is_directory(fs::temp_directory_path()));

  const fs::path old = fs::current_path();
  CHECK(old.is_absolute());
  const fs::path base = fs::canonical(tmp.str());
  fs::create_directories(base / "w" / "x");
  fs::current_path(base / "w");
  CHECK(fs::current_path() == base / "w");
  CHECK(fs::absolute("x") == base / "w" / "x");
  CHECK(fs::absolute("/abs/olute") == "/abs/olute");
  CHECK(fs::relative(base / "w" / "x", base) == "w/x");
  CHECK(fs::relative("x") == "x");
  CHECK(fs::relative(base, base / "w" / "x") == "../..");
  CHECK(fs::proximate("x", base / "w") == "x");
  std::error_code ec;
  fs::current_path(base / "missing", ec);
  CHECK(bool(ec));
  CHECK(fs::current_path() == base / "w");
  fs::current_path(old);
  return 0;
}
