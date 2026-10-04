// [fs.op.create.symlink], [fs.op.read.symlink]: create_symlink / read_symlink;
// [fs.op.symlink.status]: symlink_status does not follow the link (file_type::symlink) while
// status does; [fs.op.is.symlink]; [fs.op.remove] Note 1: "A symbolic link is itself removed,
// rather than the file it resolves to." [fs.op.canonical]: an absolute path with no dot, dot-dot
// or symlink elements; error for a missing path. [fs.op.weakly.canonical]: the missing tail is
// appended lexically-normalized. [fs.op.equivalent]: the same file through two paths.
#include <filesystem>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

int main() {
  TmpDir tmp;
  const fs::path base = fs::canonical(tmp.str());  // the temp dir itself may be behind a link
  write_file((base / "target").string(), "T");
  fs::create_directory(base / "dir");

  fs::create_symlink("target", base / "link");
  CHECK(fs::is_symlink(base / "link"));
  CHECK(fs::read_symlink(base / "link") == "target");
  CHECK(fs::symlink_status(base / "link").type() == fs::file_type::symlink);
  CHECK(fs::status(base / "link").type() == fs::file_type::regular);
  CHECK(fs::equivalent(base / "link", base / "target"));
  CHECK(!fs::equivalent(base / "dir", base / "target"));
  CHECK(fs::canonical(base / "link") == base / "target");

  fs::create_directory_symlink(base / "dir", base / "dirlink");
  CHECK(fs::canonical(base / "dirlink" / ".." / "dir" / "." ) == base / "dir");
  CHECK(fs::is_directory(base / "dirlink"));

  std::error_code ec;
  fs::canonical(base / "nope", ec);
  CHECK(bool(ec));
  CHECK(fs::weakly_canonical(base / "dirlink" / "x" / ".." / "y") == base / "dir" / "y");
  CHECK(fs::weakly_canonical(base / "no" / "such" / ".." / "file") == base / "no" / "file");

  // a dangling link: exists follows it; symlink_status does not
  fs::create_symlink(base / "gone", base / "dangling");
  CHECK(!fs::exists(base / "dangling"));
  CHECK(fs::exists(fs::symlink_status(base / "dangling")));

  CHECK(fs::remove(base / "link"));
  CHECK(fs::exists(base / "target"));  // the link itself was removed
  CHECK(fs::remove(base / "dangling"));
  CHECK(fs::remove_all(base / "dirlink") == 1);
  CHECK(fs::exists(base / "dir"));
  return 0;
}
