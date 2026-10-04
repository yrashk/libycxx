// [fs.op.create.directory]/1-2: create_directory creates the directory; "If mkdir fails because
// p resolves to an existing directory, no error is reported"; returns true iff a new directory
// was created. [fs.op.create.directories]/1-2: creates each missing element; "Returns: true if
// a new directory was created for the directory p resolves to, otherwise false."
// [fs.op.remove]/3: true if a file was removed, false otherwise ("Absence of a file p is not an
// error"). [fs.op.remove.all]/3: "The number of files removed." (0 for a missing path).
// [fs.op.exists], [fs.op.is.directory], [fs.op.is.regular.file], [fs.op.is.empty].
#include <filesystem>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  CHECK(fs::is_directory(base));
  CHECK(fs::is_empty(base));

  fs::path d = base / "d";
  CHECK(!fs::exists(d));
  CHECK(fs::create_directory(d));
  CHECK(fs::exists(d) && fs::is_directory(d) && !fs::is_regular_file(d));
  CHECK(!fs::create_directory(d));  // exists: no error, false
  std::error_code ec = std::make_error_code(std::errc::io_error);
  CHECK(!fs::create_directory(d, ec));
  CHECK(!ec);

  fs::path deep = base / "a" / "b" / "c";
  CHECK(fs::create_directories(deep));
  CHECK(fs::is_directory(base / "a" / "b") && fs::is_directory(deep));
  CHECK(!fs::create_directories(deep));
  ec = std::make_error_code(std::errc::io_error);
  CHECK(!fs::create_directories(deep, ec));
  CHECK(!ec);
  CHECK(fs::create_directories(base / "a" / "x" / ""));  // trailing separator

  // creating a directory where a regular file exists is an error
  write_file(tmp / "file", "abc");
  CHECK(fs::is_regular_file(base / "file"));
  CHECK(!fs::is_empty(base / "file"));
  ec.clear();
  CHECK(!fs::create_directory(base / "file", ec));
  CHECK(bool(ec));
  bool threw = false;
  try {
    fs::create_directories(base / "file" / "sub");
  } catch (const fs::filesystem_error&) {
    threw = true;
  }
  CHECK(threw);

  // remove
  CHECK(fs::remove(base / "file"));
  CHECK(!fs::exists(base / "file"));
  CHECK(!fs::remove(base / "file"));
  ec = std::make_error_code(std::errc::io_error);
  CHECK(!fs::remove(base / "file", ec));
  CHECK(!ec);
  CHECK(fs::remove(d));  // empty directory

  // remove_all counts every removed file: a/, a/b/, a/b/c/, a/x/, a/b/f
  write_file(tmp / "a/b/f", "z");
  CHECK(fs::remove_all(base / "a") == 5);
  CHECK(!fs::exists(base / "a"));
  CHECK(fs::remove_all(base / "a") == 0);
  ec = std::make_error_code(std::errc::io_error);
  CHECK(fs::remove_all(base / "a", ec) == 0);
  CHECK(!ec);
  write_file(tmp / "single", "1");
  CHECK(fs::remove_all(base / "single") == 1);
  CHECK(fs::is_empty(base));
  return 0;
}
