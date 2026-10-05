// The noexcept error_code overloads of the filesystem operations while operator new fails at
// its k-th call, for every k until the operation makes no allocation that fails.
//   [fs.filesystem.syn]: copy_symlink, create_directory, create_hard_link, create_symlink,
//     current_path(p, ec), equivalent, exists, file_size, hard_link_count, is_*,
//     last_write_time, permissions(p, prms, ec), remove, rename, resize_file, space, status,
//     symlink_status taking error_code& are noexcept: an allocation failure inside them cannot
//     propagate ([except.spec]/5: it would call std::terminate), so they must either meet their
//     specification or report the failure through ec ([fs.err.report]/3.1: an error "that
//     prevents the function from meeting its specifications" sets ec; otherwise ec.clear()).
//     copy_symlink in particular reads the link's target (a path) before creating the copy
//     ([fs.op.copy.symlink]/1: "Effects: Equivalent to function(read_symlink(existing_symlink),
//     new_symlink) or function(read_symlink(existing_symlink, ec), new_symlink, ec)").
//   When ec is set, the documented error results: exists false ([fs.op.exists]/3: "Returns:
//     exists(status(p, ec))", status of type none: [fs.op.status]/? "if ... error, ec set and
//     returns file_status(file_type::none)"), file_size and hard_link_count
//     static_cast<uintmax_t>(-1) ([fs.op.file.size]/?, [fs.op.hard.lk.ct]/?),
//     last_write_time file_time_type::min(), equivalent false, create_directory false,
//     remove false, space with every member static_cast<uintmax_t>(-1) ([fs.op.space]/?).
// Each run's operator new blocks are balanced (no leak).
// REQUIRES: exceptions
#include <filesystem>
#include <string>
#include <system_error>
#include "exc_new.hpp"
#include "fs_tmpdir.hpp"
#include <sys/stat.h>
#include <unistd.h>

using namespace exh;
namespace fs = std::filesystem;

template <class F>
void sw(const char* name, F op) {
  sweep_new(name, [&] {
    std::error_code ec = std::make_error_code(std::errc::io_error);  // must be cleared on success
    (void)attempt([&] { op(ec, false); });
    bool failed = st.fired;
    disarm();
    op(ec, true);  // the checks (no allocation is armed)
    return failed;
  }, options{true, 4000});
}

int main() {
  TmpDir tmp;
  const std::string base = tmp / "a-directory-name-longer-than-any-small-string-buffer";
  if (mkdir(base.c_str(), 0700) != 0) abort();
  write_file(base + "/file-with-a-long-enough-name.txt", "hello");
  const fs::path file = base + "/file-with-a-long-enough-name.txt";
  const fs::path link = base + "/symbolic-link-with-a-long-enough-name";
  const fs::path link_copy = base + "/copied-symbolic-link-with-a-long-enough-name";
  const fs::path hard = base + "/hard-link-with-a-long-enough-name";
  const fs::path newdir = base + "/new-directory-with-a-long-enough-name";
  const fs::path renamed = base + "/renamed-file-with-a-long-enough-name.txt";
  const fs::path dir = base;
  const std::string target = "file-with-a-long-enough-name.txt";
  if (symlink(target.c_str(), link.c_str()) != 0) abort();

  bool bres = false;
  std::uintmax_t ures = 0;
  fs::file_status sres;
  fs::file_time_type tres;
  fs::space_info spres;

  sw("exists", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::exists(file, ec));
    EXH_EXPECT(ec ? !bres : bres, "exists");
  });
  sw("status", [&](std::error_code& ec, bool check) {
    if (!check) return void(sres = fs::status(link, ec));
    EXH_EXPECT(ec ? sres.type() == fs::file_type::none : sres.type() == fs::file_type::regular, "status");
  });
  sw("symlink_status", [&](std::error_code& ec, bool check) {
    if (!check) return void(sres = fs::symlink_status(link, ec));
    EXH_EXPECT(ec ? sres.type() == fs::file_type::none : sres.type() == fs::file_type::symlink, "symlink_status");
  });
  sw("is_regular_file", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::is_regular_file(link, ec) && !fs::is_directory(file, ec));
    EXH_EXPECT(ec ? !bres : bres, "is_regular_file");
  });
  sw("is_symlink", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::is_symlink(link, ec));
    EXH_EXPECT(ec ? !bres : bres, "is_symlink");
  });
  sw("file_size", [&](std::error_code& ec, bool check) {
    if (!check) return void(ures = fs::file_size(file, ec));
    EXH_EXPECT(ec ? ures == static_cast<std::uintmax_t>(-1) : ures == 5, "file_size");
  });
  sw("hard_link_count", [&](std::error_code& ec, bool check) {
    if (!check) return void(ures = fs::hard_link_count(file, ec));
    EXH_EXPECT(ec ? ures == static_cast<std::uintmax_t>(-1) : ures == 1, "hard_link_count");
  });
  sw("equivalent", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::equivalent(file, link, ec));
    EXH_EXPECT(ec ? !bres : bres, "equivalent");
  });
  sw("last_write_time", [&](std::error_code& ec, bool check) {
    if (!check) return void(tres = fs::last_write_time(file, ec));
    EXH_EXPECT(ec ? tres == fs::file_time_type::min() : tres != fs::file_time_type::min(), "last_write_time");
  });
  sw("last_write_time set", [&](std::error_code& ec, bool check) {
    if (!check) return fs::last_write_time(file, fs::file_time_type::clock::now(), ec);
    EXH_EXPECT(ec != std::errc::io_error, "last_write_time set did not clear ec");
  });
  sw("permissions", [&](std::error_code& ec, bool check) {
    if (!check) return fs::permissions(file, fs::perms::owner_read | fs::perms::owner_write, ec);
    EXH_EXPECT(ec != std::errc::io_error, "permissions did not clear ec");
  });
  sw("resize_file", [&](std::error_code& ec, bool check) {
    if (!check) return fs::resize_file(file, 5, ec);
    EXH_EXPECT(ec != std::errc::io_error, "resize_file did not clear ec");
  });
  sw("space", [&](std::error_code& ec, bool check) {
    if (!check) return void(spres = fs::space(dir, ec));
    if (ec)
      EXH_EXPECT(spres.capacity == static_cast<std::uintmax_t>(-1) && spres.free == static_cast<std::uintmax_t>(-1) &&
                     spres.available == static_cast<std::uintmax_t>(-1),
                 "space error result");
    else
      EXH_EXPECT(spres.capacity != static_cast<std::uintmax_t>(-1) && spres.capacity > 0, "space");
  });
  sw("create_directory", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::create_directory(newdir, ec));
    struct stat sb;
    const bool made = stat(newdir.c_str(), &sb) == 0;
    EXH_EXPECT(ec ? !bres : bres && made, "create_directory");
    if (made) rmdir(newdir.c_str());
  });
  sw("create_directory with attributes", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::create_directory(newdir, dir, ec));
    struct stat sb;
    const bool made = stat(newdir.c_str(), &sb) == 0;
    EXH_EXPECT(ec ? !bres : bres && made, "create_directory(p, attributes)");
    if (made) rmdir(newdir.c_str());
  });
  sw("create_symlink", [&](std::error_code& ec, bool check) {
    if (!check) return fs::create_symlink(target, link_copy, ec);
    char buf[512];
    ssize_t n = readlink(link_copy.c_str(), buf, sizeof buf);
    EXH_EXPECT(ec || n == ssize_t(target.size()), "create_symlink");
    if (n >= 0) unlink(link_copy.c_str());
  });
  sw("create_hard_link", [&](std::error_code& ec, bool check) {
    if (!check) return fs::create_hard_link(file, hard, ec);
    struct stat sb;
    const bool made = stat(hard.c_str(), &sb) == 0;
    EXH_EXPECT(ec || made, "create_hard_link");
    if (made) unlink(hard.c_str());
  });
  sw("rename", [&](std::error_code& ec, bool check) {
    if (!check) return fs::rename(file, renamed, ec);
    struct stat sb;
    const bool moved = stat(renamed.c_str(), &sb) == 0;
    EXH_EXPECT(ec || moved, "rename");
    if (moved) ::rename(renamed.c_str(), file.c_str());
  });
  sw("remove", [&](std::error_code& ec, bool check) {
    if (!check) return void(bres = fs::remove(link_copy, ec));
    EXH_EXPECT(!ec && !bres, "remove of a missing file: false, no error");
  });
  sw("remove existing", [&](std::error_code& ec, bool check) {
    if (!check) {
      if (symlink(target.c_str(), link_copy.c_str()) != 0) abort();
      return void(bres = fs::remove(link_copy, ec));
    }
    struct stat sb;
    const bool still = lstat(link_copy.c_str(), &sb) == 0;
    EXH_EXPECT(ec ? !bres : bres && !still, "remove");
    if (still) unlink(link_copy.c_str());
  });
  char cwd[4096];
  if (!getcwd(cwd, sizeof cwd)) abort();
  sw("current_path set", [&](std::error_code& ec, bool check) {
    if (!check) return fs::current_path(dir, ec);
    char now[4096];
    if (!getcwd(now, sizeof now)) abort();
    EXH_EXPECT(ec || fs::path(now) == fs::canonical(dir), "current_path(p, ec)");
    if (chdir(cwd) != 0) abort();
  });
  // directory_entry's noexcept observers and refresh ([fs.dir.entry.mods], [fs.dir.entry.obs]),
  // on an entry made directly and on one obtained from a directory_iterator.
  fs::directory_entry made(link);
  fs::directory_entry iterated;
  for (const fs::directory_entry& e : fs::directory_iterator(dir))
    if (e.path().filename() == file.filename()) iterated = e;
  for (fs::directory_entry* de : {&made, &iterated}) {
    sw("directory_entry", [&](std::error_code& ec, bool check) {
      if (!check) {
        bool ok = true;
        de->refresh(ec);
        ok = ok && !ec;
        ok = de->exists(ec) && !ec && ok;
        ok = de->is_regular_file(ec) && !ec && ok;
        ures = de->file_size(ec);
        ok = !ec && ok;
        ok = de->hard_link_count(ec) >= 1 && !ec && ok;
        ok = de->last_write_time(ec) != fs::file_time_type::min() && !ec && ok;
        ok = de->status(ec).type() == fs::file_type::regular && !ec && ok;
        const fs::file_type lt = de == &made ? fs::file_type::symlink : fs::file_type::regular;
        ok = de->symlink_status(ec).type() == lt && !ec && ok;
        bres = ok && *de == *de && (*de <=> *de) == 0;
        return;
      }
      // Everything succeeded, unless an allocation failed and some call reported it through ec.
      EXH_EXPECT((bres && ures == 5) || st.fired, "directory_entry observers");
    });
  }
  // Last: it terminates if the allocation failure escapes.
  sw("copy_symlink", [&](std::error_code& ec, bool check) {
    if (!check) return fs::copy_symlink(link, link_copy, ec);
    char buf[512];
    ssize_t n = readlink(link_copy.c_str(), buf, sizeof buf);
    EXH_EXPECT(ec || (n == ssize_t(target.size()) && std::string(buf, std::size_t(n)) == target), "copy_symlink");
    if (n >= 0) unlink(link_copy.c_str());
  });
  return finish();
}
