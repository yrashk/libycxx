// Filesystem operations and file streams that fail with an operating system error part-way
// (after opening one file or directory, before finishing) leave no file descriptor open:
//   [fs.op.copy.file]/4: copy_file reports an error "if ... !is_regular_file(from)", if to
//     exists and options give no way to proceed, or if the copy fails; [fs.err.report]/3.1:
//     the error goes to ec; [fs.dir.itr.members]/2-3, [fs.rec.dir.itr.members]: an iterator
//     for a path that is not a directory reports an error (and is the end iterator,
//     [fs.class.directory.iterator.general]/3); [fs.op.copy] recursive copy into a
//     destination that cannot be created; [fs.op.is.empty]; [filebuf.members]/? open returns a
//     null pointer when the file cannot be opened; [ifstream.cons].
// Nothing in the draft lets a failed operation keep a resource it acquired; each case runs a
// hundred times and the process's open descriptors are counted before and after.
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

static int open_fds() {
  int n = 0;
  for (int fd = 0; fd < 1024; ++fd) n += fcntl(fd, F_GETFD) != -1;
  return n;
}

template <class F>
static void no_leak(const char* name, F f) {
  for (int warm = 0; warm < 2; ++warm) f();  // (anything an implementation caches is made now)
  const int before = open_fds();
  for (int i = 0; i < 100; ++i) f();
  const int after = open_fds();
  if (after != before) dprintf(2, "%s: %d descriptor(s) left open by 100 failing calls\n", name, after - before);
  CHECK(after == before);
}

int main() {
  TmpDir tmp;
  const std::string base = tmp / "a-directory-name-longer-than-any-small-string-buffer";
  CHECK(mkdir(base.c_str(), 0700) == 0);
  const std::string file = base + "/file-with-a-long-enough-name.txt";
  write_file(file, "contents of the file");
  const std::string other = base + "/other-file-with-a-long-enough-name.txt";
  write_file(other, "other contents");
  const std::string dir = base + "/a-subdirectory";
  CHECK(mkdir(dir.c_str(), 0700) == 0);
  write_file(dir + "/inside", "x");
  const std::string missing_dir = base + "/no-such-directory/target-file";

  no_leak("copy_file into a missing directory", [&] {
    std::error_code ec;
    CHECK(!fs::copy_file(file, missing_dir, ec) && ec);
  });
  no_leak("copy_file onto an existing file without options", [&] {
    std::error_code ec;
    CHECK(!fs::copy_file(file, other, ec) && ec);
    CHECK(read_file(other) == "other contents");
  });
  no_leak("copy_file of a directory", [&] {
    std::error_code ec;
    CHECK(!fs::copy_file(dir, base + "/copy-of-a-directory", ec) && ec);
  });
  no_leak("copy_file onto a directory", [&] {
    std::error_code ec;
    CHECK(!fs::copy_file(file, dir, fs::copy_options::overwrite_existing, ec) && ec);
  });
  no_leak("copy_file throwing", [&] {
    try {
      fs::copy_file(file, missing_dir);
      CHECK(false);
    } catch (const fs::filesystem_error&) {
    }
  });
  no_leak("recursive copy into a regular file", [&] {
    std::error_code ec;
    fs::copy(dir, file + "/below-a-file", fs::copy_options::recursive, ec);
    CHECK(bool(ec));
  });
  no_leak("directory_iterator of a regular file", [&] {
    std::error_code ec;
    fs::directory_iterator it(file, ec);
    CHECK(ec && it == fs::directory_iterator());
  });
  no_leak("recursive_directory_iterator of a regular file", [&] {
    std::error_code ec;
    fs::recursive_directory_iterator it(file, ec);
    CHECK(ec && it == fs::recursive_directory_iterator());
  });
  no_leak("directory_iterator throwing", [&] {
    try {
      fs::directory_iterator it(missing_dir);
      CHECK(false);
    } catch (const fs::filesystem_error&) {
    }
  });
  no_leak("is_empty of a missing path", [&] {
    std::error_code ec;
    CHECK(!fs::is_empty(missing_dir, ec) && ec);
  });
  no_leak("ifstream of a missing file", [&] {
    std::ifstream in(missing_dir);
    CHECK(!in.is_open() && in.fail());
  });
  no_leak("filebuf::open of a directory for writing", [&] {
    std::filebuf fb;
    CHECK(fb.open(dir, std::ios::out) == nullptr && !fb.is_open());
  });
  no_leak("ofstream into a missing directory", [&] {
    std::ofstream out(missing_dir);
    CHECK(!out.is_open());
  });
  no_leak("remove_all of a missing path", [&] {
    std::error_code ec;
    CHECK(fs::remove_all(missing_dir, ec) == 0 && !ec);
  });
  return 0;
}
