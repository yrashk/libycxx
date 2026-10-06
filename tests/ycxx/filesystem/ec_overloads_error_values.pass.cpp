// The error_code overloads: which conditions are errors, the value returned on error, and that
// ec is cleared on success ([fs.err.report]/3.1: "Otherwise, clear() is called on the
// error_code& argument"; every call below starts from a non-zero ec).
//   [fs.op.equivalent]/2,4: !exists(p1) || !exists(p2) is an error; false on error.
//   [fs.op.is.empty]/1: false on error (a missing file); otherwise, for a directory, whether
//     iterating it yields nothing, else whether file_size is 0.
//   [fs.op.read.symlink]/1-2: an error if p is not a symbolic link; path() on error.
//   [fs.op.hard.lk.ct]/1: static_cast<uintmax_t>(-1) on error.
//   [fs.op.last.write.time]/1: file_time_type::min() on error.
//   [fs.op.remove]/3 (Note 2): a missing file is not an error, false. [fs.op.remove.all]/1-3:
//     the number of files removed (0 for a missing p, which is no error).
//   [fs.op.create.directory]/1-2: an existing directory is not an error (false); an existing
//     non-directory, or a missing parent, is. /4-5: the same for the two-path form.
//   [fs.op.symlink.status]/3: a missing file is file_type::not_found, a symlink is symlink.
//   [fs.dir.itr.members]/2-3, [fs.err.report]/3: directory_iterator(missing, ec) sets ec
//     (and, [fs.class.directory.iterator.general]/3, is then the end iterator), as for a
//     regular file; [fs.dir.itr.nonmembers]/2-3: begin(it) is it, end(it) the end iterator.
#include <filesystem>
#include <cstdint>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

std::error_code dirty() { return std::make_error_code(std::errc::interrupted); }

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  const fs::path file = base / "file";
  const fs::path empty_file = base / "empty";
  const fs::path dir = base / "dir";
  const fs::path empty_dir = base / "emptydir";
  const fs::path missing = base / "missing";
  write_file(file.string(), "abc");
  write_file(empty_file.string(), "");
  CHECK(fs::create_directory(dir));
  CHECK(fs::create_directory(empty_dir));
  write_file((dir / "inner").string(), "x");

  std::error_code ec = dirty();
  // equivalent
  CHECK(fs::equivalent(file, base / "." / "file", ec) && !ec);
  ec = dirty();
  CHECK(!fs::equivalent(file, dir, ec) && !ec);
  CHECK(!fs::equivalent(file, missing, ec) && ec);
  ec.clear();
  CHECK(!fs::equivalent(missing, file, ec) && ec);
  ec.clear();
  CHECK(!fs::equivalent(missing, base / "missing2", ec) && ec);

  // is_empty
  ec = dirty();
  CHECK(fs::is_empty(empty_file, ec) && !ec);
  ec = dirty();
  CHECK(!fs::is_empty(file, ec) && !ec);
  ec = dirty();
  CHECK(fs::is_empty(empty_dir, ec) && !ec);
  ec = dirty();
  CHECK(!fs::is_empty(dir, ec) && !ec);
  CHECK(!fs::is_empty(missing, ec) && ec);

  // read_symlink of a non-symlink and of a missing file
  ec.clear();
  CHECK(fs::read_symlink(file, ec) == fs::path() && ec);
  ec.clear();
  CHECK(fs::read_symlink(missing, ec).empty() && ec);
  // and of a symlink, even a dangling one
  fs::create_symlink("nowhere/at/all", base / "dangling");
  ec = dirty();
  CHECK(fs::read_symlink(base / "dangling", ec) == fs::path("nowhere/at/all") && !ec);
  ec = dirty();
  CHECK(fs::symlink_status(base / "dangling", ec).type() == fs::file_type::symlink && !ec);
  CHECK(fs::status(base / "dangling", ec).type() == fs::file_type::not_found && ec);
  ec = dirty();
  CHECK(fs::symlink_status(missing, ec).type() == fs::file_type::not_found && ec);

  // hard_link_count, last_write_time
  ec = dirty();
  CHECK(fs::hard_link_count(file, ec) == 1 && !ec);
  CHECK(fs::hard_link_count(missing, ec) == static_cast<std::uintmax_t>(-1) && ec);
  ec = dirty();
  CHECK(fs::last_write_time(file, ec) != fs::file_time_type::min() && !ec);
  CHECK(fs::last_write_time(missing, ec) == fs::file_time_type::min() && ec);
  ec.clear();
  fs::last_write_time(missing, fs::file_time_type::clock::now(), ec);
  CHECK(bool(ec));

  // remove / remove_all of a missing file: no error.
  ec = dirty();
  CHECK(!fs::remove(missing, ec) && !ec);
  ec = dirty();
  CHECK(fs::remove_all(missing, ec) == 0 && !ec);
  // remove of a dangling symlink removes the link (true), not an error.
  ec = dirty();
  CHECK(fs::remove(base / "dangling", ec) && !ec);
  CHECK(!fs::exists(fs::symlink_status(base / "dangling")));
  // remove_all counts the directory and its contents.
  write_file((dir / "inner2").string(), "y");
  CHECK(fs::create_directory(dir / "sub"));
  write_file((dir / "sub" / "leaf").string(), "z");
  ec = dirty();
  CHECK(fs::remove_all(dir, ec) == 5 && !ec); // dir, inner, inner2, sub, leaf
  CHECK(!fs::exists(dir));

  // create_directory
  ec = dirty();
  CHECK(!fs::create_directory(empty_dir, ec) && !ec);
  CHECK(!fs::create_directory(file, ec) && ec);
  ec.clear();
  CHECK(!fs::create_directory(missing / "child", ec) && ec);
  ec = dirty();
  CHECK(fs::create_directory(base / "copyattr", empty_dir, ec) && !ec);
  CHECK(fs::is_directory(base / "copyattr"));
  ec = dirty();
  CHECK(!fs::create_directory(base / "copyattr", empty_dir, ec) && !ec); // exists already
  CHECK(!fs::create_directory(base / "other", missing, ec) && ec);
  CHECK(!fs::exists(base / "other"));

  // directory_iterator
  ec.clear();
  fs::directory_iterator it(missing, ec);
  CHECK(bool(ec) && it == fs::directory_iterator());
  ec = dirty();
  fs::directory_iterator e(empty_dir, ec);
  CHECK(!ec && e == fs::directory_iterator());
  CHECK(begin(e) == e && end(e) == fs::directory_iterator());
  ec.clear();
  fs::directory_iterator nd(file, ec); // not a directory
  CHECK(bool(ec) && nd == fs::directory_iterator());
  return 0;
}
