// [fs.err.report]/2.1: the overloads without error_code& throw filesystem_error; a function with
// one path argument passes it as the path (path1(), with path2() empty), a function with two
// passes the first as path1 and the second as path2, and code() is the operating system's error.
// [fs.op.status]/1-3: status(p) throws only when the type would be file_type::none (not for
// not_found, Note 1). [fs.op.equivalent]/4, [fs.op.copy.file]/4.1.4 (an existing destination
// without copy_options is an error), [fs.op.rename], [fs.op.create.hard.lk],
// [fs.op.is.empty], [fs.op.read.symlink], [fs.op.canonical]/1 (p must exist).
// [fs.filesystem.error.members]: what() includes the paths (checked only for non-emptiness).
// REQUIRES: exceptions
#include <filesystem>
#include <string>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

template <class F>
fs::filesystem_error expect_throw(F f) {
  try {
    f();
  } catch (const fs::filesystem_error& e) {
    return e;
  }
  CHECK(!"no filesystem_error thrown");
  return fs::filesystem_error("", std::error_code());
}

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  const fs::path file = base / "file";
  const fs::path other = base / "other";
  const fs::path missing = base / "missing";
  write_file(file.string(), "abc");
  write_file(other.string(), "def");

  // One path argument.
  auto e1 = expect_throw([&] { (void)fs::is_empty(missing); });
  CHECK(e1.path1() == missing && e1.path2().empty() && e1.code() == std::errc::no_such_file_or_directory);
  CHECK(*e1.what() != '\0');
  auto e2 = expect_throw([&] { (void)fs::read_symlink(file); });
  CHECK(e2.path1() == file && e2.path2().empty() && bool(e2.code()));
  auto e3 = expect_throw([&] { (void)fs::canonical(missing); });
  CHECK(e3.path1() == missing && e3.path2().empty() && e3.code() == std::errc::no_such_file_or_directory);
  auto e4 = expect_throw([&] { (void)fs::hard_link_count(missing); });
  CHECK(e4.path1() == missing && e4.code() == std::errc::no_such_file_or_directory);
  auto e5 = expect_throw([&] { fs::create_directory(file / "sub"); });
  CHECK(e5.path1() == file / "sub" && e5.path2().empty() && bool(e5.code()));
  auto e6 = expect_throw([&] { fs::directory_iterator it(missing); });
  CHECK(e6.path1() == missing && e6.code() == std::errc::no_such_file_or_directory);

  // Two path arguments: path1 is the first, path2 the second.
  auto t1 = expect_throw([&] { fs::rename(missing, base / "target"); });
  CHECK(t1.path1() == missing && t1.path2() == base / "target");
  CHECK(t1.code() == std::errc::no_such_file_or_directory);
  auto t2 = expect_throw([&] { (void)fs::copy_file(file, other); });
  CHECK(t2.path1() == file && t2.path2() == other && bool(t2.code())); // no OS error: any code
  CHECK(read_file(other.string()) == "def"); // not overwritten
  auto t3 = expect_throw([&] { (void)fs::equivalent(file, missing); });
  CHECK(t3.path1() == file && t3.path2() == missing && bool(t3.code()));
  auto t4 = expect_throw([&] { fs::create_hard_link(missing, base / "link"); });
  CHECK(t4.path1() == missing && t4.path2() == base / "link");
  CHECK(t4.code() == std::errc::no_such_file_or_directory);
  auto t5 = expect_throw([&] { fs::create_symlink("x", other); }); // the link name exists
  CHECK(t5.path1() == "x" && t5.path2() == other && t5.code() == std::errc::file_exists);

  // Not errors: status of a missing file, exists, remove of a missing file.
  CHECK(fs::status(missing).type() == fs::file_type::not_found);
  CHECK(fs::symlink_status(missing).type() == fs::file_type::not_found);
  CHECK(!fs::exists(missing) && !fs::remove(missing) && fs::remove_all(missing) == 0);
  return 0;
}
