// The error_code overloads of the filesystem operations that are NOT noexcept, and the
// error_code members of directory_iterator, recursive_directory_iterator and directory_entry,
// while operator new fails at its k-th call, for every k until the call makes no allocation
// that fails.
//   [fs.err.report]/3: "Functions having an argument of type error_code& handle errors as
//     follows, unless otherwise specified: (3.1) If a call by the implementation to an
//     operating system or other underlying API results in an error that prevents the function
//     from meeting its specifications, the error_code& argument is set as appropriate for the
//     specific operating system dependent error. Otherwise, clear() is called on the
//     error_code& argument." The draft says nothing more about allocation failure in these
//     overloads (unlike /2.2 for the overloads without error_code&). These functions are
//     potentially-throwing ([fs.filesystem.syn]), so the test accepts both ways of reporting
//     the failure: the bad_alloc propagates ([res.on.exception.handling]/4 and footnote 147),
//     or the error is reported through ec. What it checks is what the draft does state:
//   - a call that returns normally either sets ec and returns the documented error value, or
//     clears ec (it was set beforehand) and meets its specification:
//       absolute, canonical, read_symlink, current_path(ec), temp_directory_path(ec):
//         "The signature with argument ec returns path() if an error occurs" ([fs.op.absolute]/2,
//         [fs.op.canonical]/2, [fs.op.read.symlink]/1, [fs.op.current.path]/1,
//         [fs.op.temp.dir.path]/1); weakly_canonical: "path() is returned at the first error
//         occurrence" ([fs.op.weakly.canonical]/1); relative/proximate: "or path() at the first
//         error occurrence" ([fs.op.relative]/3, [fs.op.proximate]/3);
//       copy_file: "The signature with argument ec returns false if an error occurs"
//         ([fs.op.copy.file]/5); is_empty: "return false if an error occurred"
//         ([fs.op.is.empty]/1); remove_all: "returns static_cast<uintmax_t>(-1) if an error
//         occurs" ([fs.op.remove.all]/3);
//       directory_entry(p, ec): "otherwise path() == filesystem::path()" ([fs.dir.entry.cons]/2);
//       [fs.class.directory.iterator.general]/3 (and [fs.class.rec.dir.itr.general]/3: "the same
//         as a directory_iterator unless otherwise specified"): "If an
//         iterator of type directory_iterator reports an error or is advanced past the last
//         directory element, that iterator shall become equal to the end iterator value";
//   - no operator new block and no file descriptor (an open directory stream) is leaked
//     whichever way the call ends ([res.on.exception.handling]: an exception leaves no
//     resources behind; the iterators release their directory when destroyed);
//   - nothing is left in a degraded state: the same call made again once memory is available
//     succeeds and returns the right result.
// REQUIRES: exceptions
#include <filesystem>
#include <string>
#include <system_error>
#include "exc_new.hpp"
#include "fs_tmpdir.hpp"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace exh;
namespace fs = std::filesystem;

static long errors_reported = 0, exceptions_seen = 0;

static int open_fds() {
  int n = 0;
  for (int fd = 0; fd < 1024; ++fd) n += fcntl(fd, F_GETFD) != -1;
  return n;
}

// reset() restores the files the operation works on; run(ec) makes the call and returns its
// result; good(r) checks a successful result; bad(r) the documented error result.
template <class Reset, class Run, class Good, class Bad>
void sw(const char* name, Reset reset, Run run, Good good, Bad bad) {
  sweep_new(name, [&] {
    reset();
    const int fds = open_fds();
    std::error_code ec = std::make_error_code(std::errc::io_error);
    decltype(run(ec)) r{};
    const bool threw = attempt([&] { r = run(ec); });
    const bool fired = st.fired;
    disarm();
    EXH_EXPECT(open_fds() == fds, "a file descriptor leaked");
    if (threw) {
      ++exceptions_seen;
    } else if (ec) {
      ++errors_reported;
      EXH_EXPECT(fired, "an error reported with no allocation failure");
      EXH_EXPECT(bad(r), "the documented error result");
    } else {
      EXH_EXPECT(good(r), "the result of a call that cleared ec");
    }
    if (fired) {
      // The same call once memory is available.
      reset();
      std::error_code ec2 = std::make_error_code(std::errc::io_error);
      auto r2 = run(ec2);
      EXH_EXPECT(!ec2, "the call fails after an earlier allocation failure");
      EXH_EXPECT(good(r2), "the result after an earlier allocation failure");
    }
    return fired;
  }, options{true, 4000});
}

static void nothing() {}

static bool exists_posix(const std::string& p) {
  struct stat sb;
  return lstat(p.c_str(), &sb) == 0;
}

static int rm_tree(const char* p, const struct stat*, int, struct FTW*) {
  ::remove(p);
  return 0;
}
static void remove_tree(const std::string& p) { nftw(p.c_str(), rm_tree, 16, FTW_DEPTH | FTW_PHYS); }

// Directory listing results, gathered without allocating in the test.
struct Listing {
  int count = -1;
  long name_bytes = 0;
  int max_depth = 0;
  bool error_at_end = true;  // after an error the iterator was equal to the end iterator
};

static long basename_size(const fs::path& p) {
  const std::string& s = p.native();
  const auto slash = s.rfind('/');
  return long(slash == std::string::npos ? s.size() : s.size() - slash - 1);
}

int main() {
  TmpDir tmp;
  const std::string base = tmp / "a-directory-name-longer-than-any-small-string-buffer";
  if (mkdir(base.c_str(), 0700) != 0) abort();
  const std::string sub = base + "/sub-directory-with-a-long-enough-name";
  if (mkdir(sub.c_str(), 0700) != 0) abort();
  const std::string file_s = base + "/file-with-a-long-enough-name.txt";
  write_file(file_s, "hello");
  write_file(sub + "/nested-file-with-a-long-enough-name.txt", "nested");
  write_file(sub + "/second-nested-file-with-a-long-name.txt", "");
  const std::string link_s = base + "/symbolic-link-with-a-long-enough-name";
  const std::string target = "sub-directory-with-a-long-enough-name/nested-file-with-a-long-enough-name.txt";
  if (symlink(target.c_str(), link_s.c_str()) != 0) abort();
  const std::string empty_s = base + "/empty-directory-with-a-long-enough-name";
  if (mkdir(empty_s.c_str(), 0700) != 0) abort();

  char realbuf[4096];
  if (!realpath(base.c_str(), realbuf)) abort();
  const fs::path real_base = realbuf;
  const fs::path file = file_s, link = link_s, dir = base, subdir = sub;
  const fs::path real_nested = real_base / "sub-directory-with-a-long-enough-name/nested-file-with-a-long-enough-name.txt";
  const fs::path missing_tail = link / "not/existing/component/names";

  // --- operations returning a path --------------------------------------------------------
  auto empty_path = [](const fs::path& r) { return r.empty(); };
  sw("absolute", nothing, [&](std::error_code& ec) { return fs::absolute(fs::path(target), ec); },
     [&](const fs::path& r) {
       char cwd[4096];
       return getcwd(cwd, sizeof cwd) && r.is_absolute() && r == fs::path(cwd) / target;
     },
     empty_path);
  sw("canonical", nothing, [&](std::error_code& ec) { return fs::canonical(subdir / "../file-with-a-long-enough-name.txt", ec); },
     [&](const fs::path& r) { return r == real_base / "file-with-a-long-enough-name.txt"; },
     empty_path);
  sw("canonical through a link", nothing, [&](std::error_code& ec) { return fs::canonical(link, ec); },
     [&](const fs::path& r) { return r == real_nested; }, empty_path);
  sw("weakly_canonical", nothing, [&](std::error_code& ec) { return fs::weakly_canonical(fs::path(base) / "x/../sub-directory-with-a-long-enough-name/./not-there/file", ec); },
     [&](const fs::path& r) { return r == real_base / "sub-directory-with-a-long-enough-name/not-there/file"; },
     empty_path);
  sw("read_symlink", nothing, [&](std::error_code& ec) { return fs::read_symlink(link, ec); },
     [&](const fs::path& r) { return r == fs::path(target); }, empty_path);
  sw("relative", nothing, [&](std::error_code& ec) { return fs::relative(link, subdir, ec); },
     [&](const fs::path& r) { return r == fs::path("nested-file-with-a-long-enough-name.txt"); }, empty_path);
  sw("proximate", nothing, [&](std::error_code& ec) { return fs::proximate(file, subdir, ec); },
     [&](const fs::path& r) { return r == fs::path("../file-with-a-long-enough-name.txt"); }, empty_path);
  {
    char cwd[4096];
    if (!getcwd(cwd, sizeof cwd)) abort();
    if (chdir(sub.c_str()) != 0) abort();
    sw("current_path", nothing, [&](std::error_code& ec) { return fs::current_path(ec); },
       [&](const fs::path& r) { return r == real_base / "sub-directory-with-a-long-enough-name"; }, empty_path);
    sw("relative to the current directory", nothing, [&](std::error_code& ec) { return fs::relative(file, ec); },
       [&](const fs::path& r) { return r == fs::path("../file-with-a-long-enough-name.txt"); }, empty_path);
    if (chdir(cwd) != 0) abort();
  }
  // [fs.op.temp.dir.path]/1: "An unspecified directory path suitable for temporary files"; with
  // TMPDIR naming an existing directory, POSIX implementations use it ([fs.op.temp.dir.path]
  // Example 1), so only a non-empty directory path is required here.
  setenv("TMPDIR", base.c_str(), 1);
  sw("temp_directory_path", nothing, [&](std::error_code& ec) { return fs::temp_directory_path(ec); },
     [&](const fs::path& r) {
       struct stat sb;
       return !r.empty() && stat(r.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
     },
     empty_path);

  // --- operations with other results -------------------------------------------------------
  const std::string copy_s = base + "/copied-file-with-a-long-enough-name.txt";
  sw("copy_file", [&] { ::remove(copy_s.c_str()); },
     [&](std::error_code& ec) { return fs::copy_file(file, copy_s, ec); },
     [&](bool r) { return r && read_file(copy_s) == "hello"; }, [](bool r) { return !r; });
  sw("copy_file overwrite_existing", [&] { write_file(copy_s, "old contents"); },
     [&](std::error_code& ec) { return fs::copy_file(file, copy_s, fs::copy_options::overwrite_existing, ec); },
     [&](bool r) { return r && read_file(copy_s) == "hello"; }, [](bool r) { return !r; });
  sw("is_empty of a file", nothing, [&](std::error_code& ec) { return fs::is_empty(file, ec); },
     [](bool r) { return !r; }, [](bool r) { return !r; });
  sw("is_empty of an empty directory", nothing, [&](std::error_code& ec) { return fs::is_empty(empty_s, ec); },
     [](bool r) { return r; }, [](bool r) { return !r; });
  sw("is_empty of a directory", nothing, [&](std::error_code& ec) { return fs::is_empty(subdir, ec); },
     [](bool r) { return !r; }, [](bool r) { return !r; });

  const std::string tree_s = base + "/tree-root-with-a-long-enough-name";
  const std::string deep_s = tree_s + "/first-level-directory/second-level-directory/third-level";
  sw("create_directories", [&] { remove_tree(tree_s); },
     [&](std::error_code& ec) { return fs::create_directories(deep_s, ec); },
     [&](bool r) {
       struct stat sb;
       return r && stat(deep_s.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
     },
     [](bool) { return true; });
  auto make_tree = [&] {
    remove_tree(tree_s);
    std::string d = tree_s;
    for (const char* c : {"/first-level-directory", "/second-level-directory", "/third-level"}) {
      mkdir(d.c_str(), 0700);
      write_file(d + "/a-file-at-this-level-with-a-long-name", "x");
      d += c;
    }
    mkdir(d.c_str(), 0700);
  };
  sw("remove_all", make_tree, [&](std::error_code& ec) { return fs::remove_all(tree_s, ec); },
     [&](std::uintmax_t r) { return r == 7 && !exists_posix(tree_s); },
     [](std::uintmax_t r) { return r == static_cast<std::uintmax_t>(-1); });
  const std::string copy_tree_s = base + "/copied-tree-with-a-long-enough-name";
  sw("copy recursive", [&] {
       make_tree();
       remove_tree(copy_tree_s);
     },
     [&](std::error_code& ec) {
       fs::copy(tree_s, copy_tree_s, fs::copy_options::recursive, ec);
       return 0;
     },
     [&](int) {
       return read_file(copy_tree_s + "/first-level-directory/second-level-directory/a-file-at-this-level-with-a-long-name") == "x" &&
              exists_posix(copy_tree_s + "/first-level-directory/second-level-directory/third-level");
     },
     [](int) { return true; });
  sw("permissions with options", nothing,
     [&](std::error_code& ec) {
       fs::permissions(file, fs::perms::owner_write, fs::perm_options::remove, ec);
       if (!ec) fs::permissions(file, fs::perms::owner_write, fs::perm_options::add, ec);
       return 0;
     },
     [&](int) {
       struct stat sb;
       return stat(file_s.c_str(), &sb) == 0 && (sb.st_mode & S_IWUSR);
     },
     [](int) { return true; });
  chmod(file_s.c_str(), 0600);

  // --- iterators -----------------------------------------------------------------------------
  // The base directory holds: file, copied file, link, sub, empty dir, tree (left by copy),
  // copied tree. Count what is there with POSIX first.
  int expected = 0;
  long expected_bytes = 0;
  {
    fs::directory_iterator probe(dir);  // (not under failure)
    for (; probe != fs::directory_iterator(); ++probe) {
      ++expected;
      expected_bytes += basename_size(probe->path());
    }
  }
  auto good_listing = [&](const Listing& l) { return l.count == expected && l.name_bytes == expected_bytes; };
  auto bad_listing = [](const Listing& l) { return l.error_at_end; };
  sw("directory_iterator", nothing,
     [&](std::error_code& ec) {
       Listing l;
       fs::directory_iterator it(dir, ec);
       if (ec) {
         l.error_at_end = it == fs::directory_iterator();
         return l;
       }
       l.count = 0;
       while (it != fs::directory_iterator()) {
         ++l.count;
         l.name_bytes += basename_size(it->path());
         it.increment(ec);
         if (ec) {
           l.error_at_end = it == fs::directory_iterator();
           return l;
         }
       }
       return l;
     },
     good_listing, bad_listing);
  // A recursive walk of the tree: 4 directories below the root... counted with POSIX nftw.
  static int rcount;
  rcount = -1;  // nftw counts the root itself
  nftw(tree_s.c_str(), [](const char*, const struct stat*, int, struct FTW*) { return ++rcount, 0; }, 16, FTW_PHYS);
  sw("recursive_directory_iterator", nothing,
     [&](std::error_code& ec) {
       Listing l;
       fs::recursive_directory_iterator it(tree_s, ec);
       if (ec) {
         l.error_at_end = it == fs::recursive_directory_iterator();
         return l;
       }
       l.count = 0;
       while (it != fs::recursive_directory_iterator()) {
         ++l.count;
         if (it.depth() > l.max_depth) l.max_depth = it.depth();
         it.increment(ec);
         if (ec) {
           l.error_at_end = it == fs::recursive_directory_iterator();
           return l;
         }
       }
       return l;
     },
     [&](const Listing& l) { return l.count == rcount && l.max_depth == 2; }, bad_listing);
  sw("recursive_directory_iterator pop", nothing,
     [&](std::error_code& ec) {
       Listing l;
       fs::recursive_directory_iterator it(tree_s, ec);
       if (ec) {
         l.error_at_end = it == fs::recursive_directory_iterator();
         return l;
       }
       // Descend to depth 2, then pop back up twice ([fs.rec.dir.itr.members] pop: "If depth()
       // == 0, set *this to recursive_directory_iterator(). Otherwise, cease iteration of the
       // directory currently being iterated over, and continue iteration over the parent
       // directory").
       l.count = 0;
       while (it != fs::recursive_directory_iterator() && it.depth() < 2) {
         it.increment(ec);
         if (ec) return l.error_at_end = it == fs::recursive_directory_iterator(), l;
       }
       l.max_depth = it == fs::recursive_directory_iterator() ? -1 : it.depth();
       while (it != fs::recursive_directory_iterator()) {
         it.pop(ec);
         if (ec) return l.error_at_end = it == fs::recursive_directory_iterator(), l;
         ++l.count;
       }
       return l;
     },
     [&](const Listing& l) { return l.max_depth == 2 && l.count >= 1 && l.count <= 3; }, bad_listing);

  // --- directory_entry -----------------------------------------------------------------------
  struct Entry {
    fs::directory_entry e;
    bool status_ok = false;
  };
  auto good_entry = [&](const fs::path& p, fs::file_type t) {
    return [p, t](const Entry& r) { return r.e.path() == p && r.status_ok && r.e.status().type() == t; };
  };
  sw("directory_entry(p, ec)", nothing,
     [&](std::error_code& ec) {
       Entry r{fs::directory_entry(link, ec)};
       if (!ec) r.status_ok = true;
       return r;
     },
     // [fs.dir.entry.cons]/2: "path() == p if no error occurs, otherwise path() ==
     // filesystem::path()".
     good_entry(link, fs::file_type::regular), [](const Entry& r) { return r.e.path().empty(); });
  sw("directory_entry::assign", nothing,
     [&](std::error_code& ec) {
       Entry r;
       r.e.assign(subdir, ec);
       r.status_ok = !ec;
       return r;
     },
     good_entry(subdir, fs::file_type::directory), [](const Entry&) { return true; });
  sw("directory_entry::replace_filename", nothing,
     [&](std::error_code& ec) {
       Entry r{fs::directory_entry(file)};
       r.e.replace_filename("empty-directory-with-a-long-enough-name", ec);
       r.status_ok = !ec;
       return r;
     },
     good_entry(empty_s, fs::file_type::directory),
     [](const Entry&) { return true; });

  dprintf(1, "%ld failures reported through error_code, %ld as exceptions\n", errors_reported, exceptions_seen);
  return finish();
}
