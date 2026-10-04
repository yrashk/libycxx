// [fs.op.copy.file]/4-5: copy_file reports an error if to exists and none of skip_existing /
// overwrite_existing / update_existing is set; copies if to does not exist or overwrite_existing
// is set; skip_existing: no effects; returns true iff copied (false with ec on error).
// [fs.op.copy]/4.7, 4.9: copy of a regular file into an existing directory copies to
// to/from.filename(); copy of a directory with options none copies its top-level files only;
// with copy_options::recursive the whole tree; directories_only copies no regular files.
#include <filesystem>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;
using co = fs::copy_options;

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  write_file(tmp / "src", "hello");

  CHECK(fs::copy_file(base / "src", base / "dst"));
  CHECK(read_file(tmp / "dst") == "hello");

  // exists, no option: error
  bool threw = false;
  try {
    fs::copy_file(base / "src", base / "dst");
  } catch (const fs::filesystem_error& e) {
    threw = e.path1() == base / "src" && e.path2() == base / "dst";
  }
  CHECK(threw);
  std::error_code ec;
  CHECK(!fs::copy_file(base / "src", base / "dst", ec));
  CHECK(bool(ec));

  write_file(tmp / "src", "changed");
  CHECK(!fs::copy_file(base / "src", base / "dst", co::skip_existing));
  CHECK(read_file(tmp / "dst") == "hello");
  CHECK(fs::copy_file(base / "src", base / "dst", co::overwrite_existing, ec));
  CHECK(!ec);
  CHECK(read_file(tmp / "dst") == "changed");

  // copy_file on a non-regular source is an error
  fs::create_directory(base / "dir");
  CHECK(!fs::copy_file(base / "dir", base / "x", ec));
  CHECK(bool(ec));
  // the source missing
  CHECK(!fs::copy_file(base / "missing", base / "x", ec));
  CHECK(bool(ec));
  CHECK(!fs::exists(base / "x"));

  // copy a file into a directory
  fs::copy(base / "src", base / "dir");
  CHECK(read_file(tmp / "dir/src") == "changed");

  // tree: t/f1, t/f2, t/sub/f3
  fs::create_directories(base / "t" / "sub");
  write_file(tmp / "t/f1", "1");
  write_file(tmp / "t/f2", "2");
  write_file(tmp / "t/sub/f3", "3");

  fs::copy(base / "t", base / "flat");
  CHECK(read_file(tmp / "flat/f1") == "1" && read_file(tmp / "flat/f2") == "2");
  CHECK(!fs::exists(base / "flat" / "sub" / "f3"));

  fs::copy(base / "t", base / "deep", co::recursive);
  CHECK(read_file(tmp / "deep/sub/f3") == "3");
  CHECK(read_file(tmp / "deep/f1") == "1");

  fs::copy(base / "t", base / "dirs", co::recursive | co::directories_only);
  CHECK(fs::is_directory(base / "dirs" / "sub"));
  CHECK(!fs::exists(base / "dirs" / "f1"));
  CHECK(!fs::exists(base / "dirs" / "sub" / "f3"));

  // copying onto an existing tree: error for existing files unless an option says otherwise
  fs::copy(base / "t", base / "deep", co::recursive | co::skip_existing, ec);
  CHECK(!ec);
  write_file(tmp / "t/f1", "new");
  fs::copy(base / "t", base / "deep", co::recursive | co::overwrite_existing);
  CHECK(read_file(tmp / "deep/f1") == "new");

  // a missing source is an error; copying a directory onto a regular file too
  fs::copy(base / "nope", base / "y", ec);
  CHECK(bool(ec));
  fs::copy(base / "t", base / "src", ec);
  CHECK(bool(ec));
  return 0;
}
