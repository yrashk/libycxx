// The noexcept error_code overloads of the filesystem operations on their ERROR paths (the
// underlying API reports an error: a missing file, a file where a directory is needed, an
// existing destination, a non-empty directory, a symbolic link loop) and the noexcept ones not
// covered elsewhere (create_directory_symlink, the file type queries other than
// is_regular_file / is_symlink, and the corresponding directory_entry observers), while
// operator new fails at its k-th call, for every k until the call makes no allocation that
// fails.
//   [except.spec]/5: an exception escaping a noexcept function calls std::terminate (this test
//     then dies). [fs.err.report]/3.1: "If a call by the implementation to an operating system
//     or other underlying API results in an error that prevents the function from meeting its
//     specifications, the error_code& argument is set as appropriate for the specific
//     operating system dependent error. Otherwise, clear() is called on the error_code&
//     argument." The draft has no allocation-failure rule for these overloads, so when an
//     allocation fails the test accepts any non-zero ec with the documented error result;
//     when none fails, the exact result:
//   [fs.op.status]/4, /6.1.1: status(p, ec) for a p with an element that does not exist sets
//     ec to the error reported (ENOENT: errc::no_such_file_or_directory) and returns
//     file_status(file_type::not_found); [fs.op.symlink.status]/3 likewise;
//   [fs.op.exists]/3-4: "calls ec.clear() if status_known(s)"; exists(missing) is false with
//     ec clear;
//   [fs.op.is.directory]/2 (and the other is_* queries): "returns false if an error occurs";
//   [fs.op.file.size]/1-2: "If exists(p) is false, an error is reported", the ec form
//     "returns static_cast<uintmax_t>(-1) if an error occurs"; [fs.op.hard.lk.ct]/1 (-1);
//     [fs.op.last.write.time]/1 (file_time_type::min()); [fs.op.space]/1 (every member -1);
//   [fs.op.equivalent]: an error if neither path resolves to an existing file;
//   [fs.op.create.directory]/1: "If mkdir fails because p resolves to an existing directory,
//     no error is reported. Otherwise on failure an error is reported"; /2 false when nothing
//     was created;
//   [fs.op.remove]/1-3: "Absence of a file p is not an error"; removing a non-empty directory
//     (POSIX remove fails) is an error, false;
//   [fs.op.create.dir.symlk], [fs.op.create.symlink], [fs.op.create.hard.lk], [fs.op.rename],
//     [fs.op.resize.file], [fs.op.permissions], [fs.op.current.path], [fs.op.copy.symlink]:
//     an existing destination or a missing source is an error reported through ec.
// A failed call is then repeated with no allocation failure, and must give the exact result.
// Each run's operator new blocks are balanced (no leak).
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include "exc_new.hpp"
#include "fs_tmpdir.hpp"
#include <sys/stat.h>
#include <unistd.h>

using namespace exh;
namespace fs = std::filesystem;

// run(ec) makes the call; check(r, ec, fired) checks the outcome of a call that returned.
template <class Run, class Check>
void sw(const char* name, Run run, Check check) {
  sweep_new(name, [&] {
    std::error_code ec = std::make_error_code(std::errc::io_error);
    decltype(run(ec)) r{};
    (void)attempt([&] { r = run(ec); });  // an escaping exception would have terminated
    const bool fired = st.fired;
    disarm();
    EXH_EXPECT(check(r, ec, fired), "the outcome");
    if (fired) {
      std::error_code ec2 = std::make_error_code(std::errc::io_error);
      auto r2 = run(ec2);
      EXH_EXPECT(check(r2, ec2, false), "the outcome after an earlier allocation failure");
    }
    return fired;
  }, options{true, 4000});
}

constexpr auto minus1 = static_cast<std::uintmax_t>(-1);

int main() {
  TmpDir tmp;
  const std::string base = tmp / "a-directory-name-longer-than-any-small-string-buffer";
  if (mkdir(base.c_str(), 0700) != 0) abort();
  const fs::path dir = base;
  const fs::path file = base + "/file-with-a-long-enough-name.txt";
  write_file(file.native(), "hello");
  const fs::path missing = base + "/missing-directory-with-a-long-name/missing-file-with-a-long-name";
  const fs::path through_file = file / "a-component-below-a-regular-file";
  const fs::path loop_a = base + "/symbolic-link-loop-first-with-a-long-name";
  const fs::path loop_b = base + "/symbolic-link-loop-second-with-a-long-name";
  if (symlink(loop_b.c_str(), loop_a.c_str()) != 0 || symlink(loop_a.c_str(), loop_b.c_str()) != 0) abort();
  const fs::path nonempty = base + "/non-empty-directory-with-a-long-enough-name";
  if (mkdir(nonempty.c_str(), 0700) != 0) abort();
  write_file(nonempty.native() + "/content", "x");
  const fs::path fifo = base + "/named-pipe-with-a-long-enough-name";
  if (mkfifo(fifo.c_str(), 0600) != 0) abort();
  const fs::path dirlink = base + "/directory-symbolic-link-with-a-long-name";
  const fs::path dev_null = "/dev/null";  // POSIX: a character special file

  // --- status family on error paths ----------------------------------------------------------
  sw("status of a missing file", [&](std::error_code& ec) { return fs::status(missing, ec); },
     [&](const fs::file_status& s, const std::error_code& ec, bool fired) {
       if (fired) return bool(ec) && (s.type() == fs::file_type::not_found || s.type() == fs::file_type::none);
       return ec == std::errc::no_such_file_or_directory && s.type() == fs::file_type::not_found;
     });
  sw("symlink_status of a missing file", [&](std::error_code& ec) { return fs::symlink_status(missing, ec); },
     [&](const fs::file_status& s, const std::error_code& ec, bool fired) {
       if (fired) return bool(ec) && (s.type() == fs::file_type::not_found || s.type() == fs::file_type::none);
       return ec == std::errc::no_such_file_or_directory && s.type() == fs::file_type::not_found;
     });
  sw("status through a link loop", [&](std::error_code& ec) { return fs::status(loop_a, ec); },
     [&](const fs::file_status& s, const std::error_code& ec, bool) {
       return bool(ec) && s.type() != fs::file_type::regular && s.type() != fs::file_type::directory &&
              s.type() != fs::file_type::symlink;
     });
  sw("status below a regular file", [&](std::error_code& ec) { return fs::status(through_file, ec); },
     [&](const fs::file_status& s, const std::error_code& ec, bool) {
       return bool(ec) && s.type() != fs::file_type::regular && s.type() != fs::file_type::directory;
     });
  sw("exists of a missing file", [&](std::error_code& ec) { return fs::exists(missing, ec); },
     [&](bool r, const std::error_code& ec, bool fired) { return !r && (fired || !ec); });
  for (const fs::path* p : {&missing, &file, &dev_null, &fifo, &dir}) {
    sw("file type queries", [&](std::error_code& ec) {
         int bits = 0, errs = 0;
         auto q = [&](bool b, int bit) {
           bits |= b ? bit : 0;
           errs += bool(ec);
         };
         q(fs::is_block_file(*p, ec), 1);
         q(fs::is_character_file(*p, ec), 2);
         q(fs::is_directory(*p, ec), 4);
         q(fs::is_fifo(*p, ec), 8);
         q(fs::is_other(*p, ec), 16);
         q(fs::is_socket(*p, ec), 32);
         q(fs::is_regular_file(*p, ec), 64);
         q(fs::is_symlink(*p, ec), 128);
         return bits * 100 + errs;
       },
       [&](int r, const std::error_code&, bool fired) {
         const int bits = r / 100, errs = r % 100;
         const int want = p == &missing ? 0 : p == &file ? 64 : p == &dev_null ? 2 + 16 : p == &fifo ? 8 + 16 : 4;
         // [fs.op.is.other]: is_other(s) is exists(s) && !is_regular_file(s) &&
         // !is_directory(s) && !is_symlink(s), so a fifo and a character file are "other" too.
         if (fired) return (bits & ~want) == 0;  // a query that failed returned false
         // A missing file: is_symlink uses symlink_status, which reports ENOENT like status.
         return bits == want && errs == (p == &missing ? 8 : 0);
       });
  }

  // --- other queries -----------------------------------------------------------------------
  sw("file_size of a missing file", [&](std::error_code& ec) { return fs::file_size(missing, ec); },
     [](std::uintmax_t r, const std::error_code& ec, bool) { return ec && r == minus1; });
  sw("hard_link_count of a missing file", [&](std::error_code& ec) { return fs::hard_link_count(missing, ec); },
     [](std::uintmax_t r, const std::error_code& ec, bool) { return ec && r == minus1; });
  sw("last_write_time of a missing file", [&](std::error_code& ec) { return fs::last_write_time(missing, ec); },
     [](fs::file_time_type r, const std::error_code& ec, bool) { return ec && r == fs::file_time_type::min(); });
  sw("equivalent of missing files", [&](std::error_code& ec) { return fs::equivalent(missing, through_file, ec); },
     [](bool r, const std::error_code& ec, bool) { return ec && !r; });
  sw("space of a missing file", [&](std::error_code& ec) { return fs::space(missing, ec); },
     [](const fs::space_info& r, const std::error_code& ec, bool) {
       return ec && r.capacity == minus1 && r.free == minus1 && r.available == minus1;
     });

  // --- modifying operations on error paths ---------------------------------------------------
  sw("create_directory of an existing directory", [&](std::error_code& ec) { return fs::create_directory(dir, ec); },
     [](bool r, const std::error_code& ec, bool fired) { return !r && (fired || !ec); });
  sw("create_directory over a regular file", [&](std::error_code& ec) { return fs::create_directory(file, ec); },
     [](bool r, const std::error_code& ec, bool) { return !r && ec; });
  sw("create_directory below a missing directory", [&](std::error_code& ec) { return fs::create_directory(missing, dir, ec); },
     [](bool r, const std::error_code& ec, bool) { return !r && ec; });
  sw("remove of a missing file", [&](std::error_code& ec) { return fs::remove(missing, ec); },
     [](bool r, const std::error_code& ec, bool fired) { return !r && (fired || !ec); });
  sw("remove of a non-empty directory", [&](std::error_code& ec) { return fs::remove(nonempty, ec); },
     [&](bool r, const std::error_code& ec, bool) {
       struct stat sb;
       return !r && ec && stat(nonempty.c_str(), &sb) == 0;
     });
  sw("create_symlink over an existing file", [&](std::error_code& ec) {
       fs::create_symlink("target-name-of-a-symbolic-link", file, ec);
       return 0;
     },
     [&](int, const std::error_code& ec, bool) { return ec && read_file(file.native()) == "hello"; });
  sw("create_directory_symlink", [&](std::error_code& ec) {
       fs::create_directory_symlink(dir, dirlink, ec);
       char buf[4096];
       const ssize_t n = readlink(dirlink.c_str(), buf, sizeof buf);
       if (n >= 0) unlink(dirlink.c_str());
       return n == ssize_t(dir.native().size()) && dir.native().compare(0, std::string::npos, buf, std::size_t(n)) == 0;
     },
     [](bool made, const std::error_code& ec, bool) { return ec ? !made : made; });
  sw("create_directory_symlink over an existing file", [&](std::error_code& ec) {
       fs::create_directory_symlink(dir, file, ec);
       return 0;
     },
     [&](int, const std::error_code& ec, bool) { return ec && read_file(file.native()) == "hello"; });
  sw("create_hard_link of a missing file", [&](std::error_code& ec) {
       fs::create_hard_link(missing, base + "/hard-link-that-cannot-be-created", ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("rename of a missing file", [&](std::error_code& ec) {
       fs::rename(missing, base + "/rename-destination-with-a-long-name", ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("rename of a directory over a regular file", [&](std::error_code& ec) {
       fs::rename(nonempty, file, ec);
       return 0;
     },
     [&](int, const std::error_code& ec, bool) { return ec && read_file(file.native()) == "hello"; });
  sw("resize_file of a missing file", [&](std::error_code& ec) {
       fs::resize_file(missing, 10, ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("permissions of a missing file", [&](std::error_code& ec) {
       fs::permissions(missing, fs::perms::owner_all, ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("last_write_time of a missing file (set)", [&](std::error_code& ec) {
       fs::last_write_time(missing, fs::file_time_type::clock::now(), ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("current_path to a missing directory", [&](std::error_code& ec) {
       fs::current_path(missing, ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });

  // --- directory_entry observers -----------------------------------------------------------
  for (const fs::path* p : {&missing, &fifo, &dev_null, &dir}) {
    fs::directory_entry e(*p);  // a missing file is not an error for refresh ([fs.dir.entry.mods])
    sw("directory_entry file type observers", [&](std::error_code& ec) {
         int bits = 0;
         auto q = [&](bool b, int bit) { bits |= b && !ec ? bit : 0; };
         q(e.is_block_file(ec), 1);
         q(e.is_character_file(ec), 2);
         q(e.is_directory(ec), 4);
         q(e.is_fifo(ec), 8);
         q(e.is_other(ec), 16);
         q(e.is_socket(ec), 32);
         q(e.is_regular_file(ec), 64);
         q(e.is_symlink(ec), 128);
         q(e.exists(ec), 256);
         return bits;
       },
       [&](int bits, const std::error_code&, bool fired) {
         const int want = p == &missing ? 0 : p == &fifo ? 8 + 16 + 256 : p == &dev_null ? 2 + 16 + 256 : 4 + 256;
         return fired ? (bits & ~want) == 0 : bits == want;
       });
  }

  // Last: copy_symlink's allocation failure on its success path terminates in some
  // implementations; here its error path (the source is not a symbolic link, [fs.op.copy.symlink]
  // Effects: read_symlink(existing_symlink, ec) reports the error).
  sw("copy_symlink of a regular file", [&](std::error_code& ec) {
       fs::copy_symlink(file, base + "/copy-of-a-symbolic-link-that-is-a-file", ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  sw("copy_symlink of a missing file", [&](std::error_code& ec) {
       fs::copy_symlink(missing, base + "/copy-of-a-symbolic-link-that-is-missing", ec);
       return 0;
     },
     [](int, const std::error_code& ec, bool) { return bool(ec); });
  return finish();
}
