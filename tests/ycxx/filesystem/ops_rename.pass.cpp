// [fs.op.rename]/1: rename(old_p, new_p) "as if by POSIX rename": an existing non-directory
// new_p is replaced; an existing empty directory new_p is replaced on POSIX; renaming to itself
// does nothing. Errors (missing source) are reported per [fs.err.report] (the ec overload is
// noexcept and sets ec; the throwing overload passes both paths to filesystem_error).
// REQUIRES: exceptions
#include <filesystem>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  write_file(tmp / "a", "A");
  fs::rename(base / "a", base / "b");
  CHECK(!fs::exists(base / "a"));
  CHECK(read_file(tmp / "b") == "A");

  write_file(tmp / "c", "C");
  fs::rename(base / "c", base / "b");  // replaces the existing file
  CHECK(read_file(tmp / "b") == "C");
  CHECK(!fs::exists(base / "c"));

  fs::rename(base / "b", base / "b");  // same file: no action
  CHECK(read_file(tmp / "b") == "C");

  fs::create_directories(base / "d1" / "inner");
  write_file(tmp / "d1/inner/f", "F");
  fs::rename(base / "d1", base / "d2");
  CHECK(read_file(tmp / "d2/inner/f") == "F");
  fs::create_directory(base / "empty");
  fs::rename(base / "d2", base / "empty");  // an existing empty directory is replaced
  CHECK(read_file(tmp / "empty/inner/f") == "F");

  std::error_code ec;
  static_assert(noexcept(fs::rename(base, base, ec)));
  fs::rename(base / "missing", base / "z", ec);
  CHECK(bool(ec));
  CHECK(ec == std::errc::no_such_file_or_directory);
  bool threw = false;
  try {
    fs::rename(base / "missing", base / "z");
  } catch (const fs::filesystem_error& e) {
    threw = e.path1() == base / "missing" && e.path2() == base / "z" &&
            e.code() == std::errc::no_such_file_or_directory;
  }
  CHECK(threw);
  ec = std::make_error_code(std::errc::io_error);
  fs::rename(base / "empty", base / "moved", ec);
  CHECK(!ec);
  return 0;
}
