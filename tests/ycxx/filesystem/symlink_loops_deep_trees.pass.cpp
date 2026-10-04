// Symbolic-link loops and deep trees.
// 1. l1 -> l2, l2 -> l1. [fs.op.status]/4-6: status(p, ec) sets ec (POSIX stat: ELOOP) and,
//    since the error is neither "some element of the path does not exist" nor "can be resolved
//    but the attributes cannot be determined", returns file_status(file_type::none); /1: status(p)
//    then throws filesystem_error. [fs.op.exists]: exists(p, ec) is exists(status(p, ec)) = false
//    with ec set; exists(p) throws. [fs.op.is.symlink]: symlink_status is unaffected.
//    canonical, equivalent, file_size, create_directories report the error ([fs.err.report]);
//    remove(l1) removes the link ([fs.op.remove]: as if by POSIX remove).
// 2. d/self -> "." iterated with directory_options::follow_directory_symlink.
//    [fs.rec.dir.itr.members]/21.2: an entry is recursed into only if
//    is_directory((*this)->status()) and the directory "(*this)->path()" is then iterated; the
//    entry paths are d/self/self/... ([fs.rec.dir.itr.members]: each entry's path is its parent's
//    path / filename). Resolving such a path follows every "self" link, which POSIX limits
//    (SYMLOOP_MAX; ELOOP), so the recursion stops after finitely many levels: every entry the
//    iterator yields has a path that can be resolved up to its last element (symlink_status
//    succeeds), and the iteration ends (with increment(ec): normally or with ec set, the iterator
//    then being the end iterator, [fs.class.directory.iterator.general]/3).
// 3. A chain of 1100 nested directories: recursive_directory_iterator visits each once, and
//    [fs.op.remove.all]: remove_all removes them all and returns the number removed (1100).
#include <filesystem>
#include <string>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

int main() {
  {
    TmpDir t;
    const fs::path b = t.str();
    fs::create_symlink(b / "l2", b / "l1");
    fs::create_symlink(b / "l1", b / "l2");
    std::error_code ec;
    fs::file_status s = fs::status(b / "l1", ec);
    CHECK(ec == std::errc::too_many_symbolic_link_levels);
    CHECK(s.type() == fs::file_type::none);
    bool threw = false;
    try {
      (void)fs::status(b / "l1");
    } catch (const fs::filesystem_error& e) {
      threw = e.code() == std::errc::too_many_symbolic_link_levels && e.path1() == b / "l1";
    }
    CHECK(threw);
    CHECK(!fs::exists(b / "l1", ec) && ec);
    threw = false;
    try {
      (void)fs::exists(b / "l1");
    } catch (const fs::filesystem_error&) {
      threw = true;
    }
    CHECK(threw);
    CHECK(fs::is_symlink(b / "l1") && fs::symlink_status(b / "l1").type() == fs::file_type::symlink);
    CHECK(fs::read_symlink(b / "l1") == b / "l2");
    CHECK(fs::canonical(b / "l1", ec).empty() && ec);
    ec.clear();
    CHECK(!fs::equivalent(b / "l1", b / "l2", ec) && ec);
    ec.clear();
    CHECK(fs::file_size(b / "l1", ec) == static_cast<std::uintmax_t>(-1) && ec);
    ec.clear();
    fs::create_directories(b / "l1" / "sub", ec);
    CHECK(ec);
    CHECK(fs::remove(b / "l1"));
    CHECK(!fs::exists(fs::symlink_status(b / "l1")) && fs::is_symlink(b / "l2"));
  }
  {
    TmpDir t;
    const fs::path d = fs::path(t.str()) / "d";
    fs::create_directory(d);
    write_file(t / "d/file", "x");
    fs::create_directory_symlink(".", d / "self");
    std::error_code ec;
    fs::recursive_directory_iterator it(d, fs::directory_options::follow_directory_symlink, ec), end;
    CHECK(!ec);
    int n = 0;
    while (it != end && n < 1000) {
      ++n;
      std::error_code sec;
      (void)fs::symlink_status(it->path(), sec);
      CHECK(!sec);
      it.increment(ec);
      if (ec) {
        CHECK(it == end);
        break;
      }
    }
    CHECK(n < 1000);
    // without follow_directory_symlink: just "file" and "self"
    n = 0;
    for (fs::recursive_directory_iterator i(d), e; i != e; ++i) ++n;
    CHECK(n == 2);
  }
  {
    TmpDir t;
    // built with POSIX calls (mkdirat/openat), independent of the library under test
    int fd = ::open(t.str().c_str(), O_RDONLY | O_DIRECTORY);
    for (int i = 0; i < 1100; ++i) {
      CHECK(::mkdirat(fd, "d", 0700) == 0);
      int sub = ::openat(fd, "d", O_RDONLY | O_DIRECTORY);
      ::close(fd);
      fd = sub;
    }
    int leaf = ::openat(fd, "leaf", O_WRONLY | O_CREAT, 0600);
    CHECK(leaf >= 0);
    ::close(leaf);
    ::close(fd);
    long n = 0;
    int maxdepth = 0;
    for (fs::recursive_directory_iterator i(t.str()), e; i != e; ++i) {
      ++n;
      if (i.depth() > maxdepth) maxdepth = i.depth();
    }
    CHECK(n == 1101 && maxdepth == 1100);
    std::error_code ec;
    CHECK(fs::remove_all(fs::path(t.str()) / "d", ec) == 1101 && !ec);
    CHECK(fs::is_empty(t.str()));
    CHECK(fs::remove_all(fs::path(t.str()) / "missing") == 0);
  }
  return 0;
}
