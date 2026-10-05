// [fs.op.copy]/4, branch by branch, with the copy_options of [fs.enum.copy.opts] (at most one
// per option group, /3), each in a fresh tree:
//   src/            dir1/ { file1, file2, dir2/ { file3 } }, file "f", symlink "lf" -> "f",
//                   symlink "ld" -> "dir1", dangling symlink "dang" -> "nowhere"
// 4.5: errors for !exists(f) (a missing source, a dangling symlink followed), equivalent(from, to),
//      is_directory(f) && is_regular_file(t)
// 4.6: a symlink source (f from symlink_status when copy_symlinks/skip_symlinks/create_symlinks):
//      skip_symlinks: nothing; copy_symlinks and !exists(t): copy_symlink; otherwise an error
// 4.7: a regular file: directories_only: nothing; create_symlinks: a symlink to the source;
//      create_hard_links: a hard link; into an existing directory: to/from.filename()
// 4.8: a directory with create_symlinks: error is_a_directory
// 4.9: a directory with recursive or exactly none: create_directory(to, from) if needed, then
//      copy each entry with options | in-recursive-copy (so with none, subdirectories are not
//      copied: Example 1); 4.10/4.11: otherwise nothing (e.g. directories_only alone)
// Each error case is checked through both signatures: the ec form sets ec, the other throws
// filesystem_error ([fs.err.report]).
// REQUIRES: exceptions
#include <filesystem>
#include <string>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;
using co = fs::copy_options;

struct Tree {
  TmpDir tmp;
  fs::path b;
  Tree() : b(tmp.str()) {
    fs::create_directories(b / "dir1" / "dir2");
    write_file(tmp / "dir1/file1", "1");
    write_file(tmp / "dir1/file2", "22");
    write_file(tmp / "dir1/dir2/file3", "333");
    write_file(tmp / "f", "ffff");
    fs::create_symlink("f", b / "lf");
    fs::create_directory_symlink("dir1", b / "ld");
    fs::create_symlink("nowhere", b / "dang");
  }
};

template <class F>
void error_both_ways(F f) {
  std::error_code ec;
  f(&ec);
  CHECK(ec);
  bool threw = false;
  try {
    f(nullptr);
  } catch (const fs::filesystem_error&) {
    threw = true;
  }
  CHECK(threw);
}
void cp(const fs::path& a, const fs::path& b, co o, std::error_code* ec) {
  if (ec)
    fs::copy(a, b, o, *ec);
  else
    fs::copy(a, b, o);
}

int main() {
  // 4.5 errors
  {
    Tree t;
    error_both_ways([&](std::error_code* ec) { cp(t.b / "missing", t.b / "x", co::none, ec); });
    error_both_ways([&](std::error_code* ec) { cp(t.b / "dang", t.b / "x", co::none, ec); });  // status follows
    error_both_ways([&](std::error_code* ec) { cp(t.b / "f", t.b / "f", co::overwrite_existing, ec); });
    error_both_ways([&](std::error_code* ec) { cp(t.b / "f", t.b / "lf", co::overwrite_existing, ec); });
    error_both_ways([&](std::error_code* ec) { cp(t.b / "dir1", t.b / "f", co::recursive, ec); });
    CHECK(!fs::exists(t.b / "x"));
    CHECK(read_file(t.tmp / "f") == "ffff");
  }
  // 4.6 symlink sources
  {
    Tree t;
    std::error_code ec = std::make_error_code(std::errc::io_error);
    fs::copy(t.b / "lf", t.b / "s1", co::skip_symlinks, ec);
    CHECK(!ec && !fs::exists(fs::symlink_status(t.b / "s1")));
    fs::copy(t.b / "lf", t.b / "s2", co::copy_symlinks);
    CHECK(fs::is_symlink(fs::symlink_status(t.b / "s2")) && fs::read_symlink(t.b / "s2") == "f");
    fs::copy(t.b / "dang", t.b / "s3", co::copy_symlinks);  // f is the link itself: it exists
    CHECK(fs::is_symlink(fs::symlink_status(t.b / "s3")) && fs::read_symlink(t.b / "s3") == "nowhere");
    fs::copy(t.b / "ld", t.b / "s4", co::copy_symlinks);
    CHECK(fs::is_symlink(fs::symlink_status(t.b / "s4")) && fs::read_symlink(t.b / "s4") == "dir1");
    // copy_symlinks, t exists: error (4.6.3)
    error_both_ways([&](std::error_code* e) { cp(t.b / "lf", t.b / "f", co::copy_symlinks, e); });
    // skip_symlinks also skips a dangling link
    fs::copy(t.b / "dang", t.b / "s5", co::skip_symlinks);
    CHECK(!fs::exists(fs::symlink_status(t.b / "s5")));
    // without symlink options the link is followed: a regular file is copied
    fs::copy(t.b / "lf", t.b / "s6");
    CHECK(fs::is_regular_file(fs::symlink_status(t.b / "s6")) && read_file(t.tmp / "s6") == "ffff");
    // a symlink with create_symlinks: f = symlink_status, so 4.6: neither skip nor copy: error
    error_both_ways([&](std::error_code* e) { cp(t.b / "lf", t.b / "s7", co::create_symlinks, e); });
  }
  // 4.7 regular files
  {
    Tree t;
    fs::copy(t.b / "f", t.b / "r1", co::directories_only);
    CHECK(!fs::exists(t.b / "r1"));
    fs::copy(t.b / "f", t.b / "r2", co::create_symlinks);
    CHECK(fs::is_symlink(fs::symlink_status(t.b / "r2")) && fs::equivalent(t.b / "r2", t.b / "f"));
    fs::copy(t.b / "f", t.b / "r3", co::create_hard_links);
    CHECK(fs::is_regular_file(fs::symlink_status(t.b / "r3")) && fs::equivalent(t.b / "r3", t.b / "f"));
    CHECK(fs::hard_link_count(t.b / "f") == 2);
    fs::copy(t.b / "f", t.b / "dir1");  // into a directory: dir1/f
    CHECK(read_file(t.tmp / "dir1/f") == "ffff");
  }
  {
    Tree t;
    fs::copy(t.b / "f", t.b / "ld");  // ld resolves to dir1: copies to ld/f, i.e. dir1/f
    CHECK(read_file(t.tmp / "dir1/f") == "ffff");
    error_both_ways([&](std::error_code* e) { cp(t.b / "f", t.b / "dir1", co::none, e); });  // dir1/f exists
    write_file(t.tmp / "f", "new!");
    fs::copy(t.b / "f", t.b / "dir1", co::overwrite_existing);
    CHECK(read_file(t.tmp / "dir1/f") == "new!");
    fs::copy(t.b / "f", t.b / "dir1", co::skip_existing);
    CHECK(read_file(t.tmp / "dir1/f") == "new!");
  }
  // 4.8, 4.9, 4.10
  {
    Tree t;
    error_both_ways([&](std::error_code* e) { cp(t.b / "dir1", t.b / "d0", co::create_symlinks, e); });
    {
      std::error_code ec;
      fs::copy(t.b / "dir1", t.b / "dx", co::create_symlinks, ec);
      CHECK(ec == std::errc::is_a_directory);
    }
    CHECK(!fs::exists(t.b / "d0"));
    std::error_code ec = std::make_error_code(std::errc::io_error);
    fs::copy(t.b / "dir1", t.b / "d1", co::directories_only, ec);  // neither recursive nor none
    CHECK(!ec && !fs::exists(t.b / "d1"));
    fs::copy(t.b / "dir1", t.b / "d2");  // Example 1: files only
    CHECK(read_file(t.tmp / "d2/file1") == "1" && read_file(t.tmp / "d2/file2") == "22");
    CHECK(!fs::exists(t.b / "d2/dir2"));
    fs::copy(t.b / "dir1", t.b / "d3", co::recursive);  // Example 1: the whole tree
    CHECK(read_file(t.tmp / "d3/dir2/file3") == "333");
    fs::copy(t.b / "dir1", t.b / "d4", co::recursive | co::directories_only);
    CHECK(fs::is_directory(t.b / "d4/dir2") && !fs::exists(t.b / "d4/file1") && !fs::exists(t.b / "d4/dir2/file3"));
    fs::copy(t.b / "dir1", t.b / "d5", co::recursive | co::create_hard_links);
    CHECK(fs::equivalent(t.b / "d5/dir2/file3", t.b / "dir1/dir2/file3"));
    // into an existing directory: no creation, files copied; an existing file is an error
    fs::copy(t.b / "dir1", t.b / "d3", co::recursive | co::skip_existing);
    error_both_ways([&](std::error_code* e) { cp(t.b / "dir1", t.b / "d3", co::recursive, e); });
    write_file(t.tmp / "dir1/dir2/file3", "changed");
    fs::copy(t.b / "dir1", t.b / "d3", co::recursive | co::overwrite_existing);
    CHECK(read_file(t.tmp / "d3/dir2/file3") == "changed");
    // a directory reached through a symlink (status follows), recursive
    fs::copy(t.b / "ld", t.b / "d6", co::recursive);
    CHECK(fs::is_directory(fs::symlink_status(t.b / "d6")) && read_file(t.tmp / "d6/dir2/file3") == "changed");
    // symlinks inside a recursive copy
    fs::create_symlink("file1", t.b / "dir1/lnk");
    fs::copy(t.b / "dir1", t.b / "d7", co::recursive | co::copy_symlinks);
    CHECK(fs::is_symlink(fs::symlink_status(t.b / "d7/lnk")) && fs::read_symlink(t.b / "d7/lnk") == "file1");
    fs::copy(t.b / "dir1", t.b / "d8", co::recursive | co::skip_symlinks);
    CHECK(!fs::exists(fs::symlink_status(t.b / "d8/lnk")) && fs::exists(t.b / "d8/file1"));
    fs::copy(t.b / "dir1", t.b / "d9", co::recursive);
    CHECK(fs::is_regular_file(fs::symlink_status(t.b / "d9/lnk")) && read_file(t.tmp / "d9/lnk") == "1");
    // copy with ec clears ec on success
    ec = std::make_error_code(std::errc::io_error);
    fs::copy(t.b / "f", t.b / "ok", ec);
    CHECK(!ec && read_file(t.tmp / "ok") == "ffff");
  }
  return 0;
}
