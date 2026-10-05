// File types other than regular files, directories and symlinks, space(), last_write_time():
//   [fs.op.is.fifo] is_fifo(s): s.type() == file_type::fifo; [fs.op.is.socket], [fs.op.is.char.
//   file] (file_type::character), [fs.op.is.block.file] (file_type::block); /2 of each: the path
//   forms are is_X(status(p)) or is_X(status(p, ec)), "The signature with argument ec returns
//   false if an error occurs"; [fs.op.is.other] is_other(s): "exists(s) && !is_regular_file(s)
//   && !is_directory(s) && !is_symlink(s)"; [fs.op.status]: status(p) of a FIFO / socket /
//   character device has type fifo / socket / character ([fs.op.status]/4.3: "Otherwise, if
//   [the POSIX type] indicates a block special file ... a character special file ... a fifo ...
//   a socket, returns file_status(file_type::block / character / fifo / socket, prms)").
//   [fs.op.space]: capacity, free and available are statvfs's f_blocks, f_bfree and f_bavail
//   times f_frsize; with ec, every member is static_cast<uintmax_t>(-1) on error.
//   [fs.op.last.write.time]: the time of last data modification as if by stat's st_mtime; with
//   ec, file_time_type::min() on error; the setter "as if by POSIX futimens".
//   [fs.op.hard.lk.ct] hard_link_count, [fs.op.create.hard.lk] create_hard_link: the count
//   grows with each link.
// The platform properties come from POSIX calls made by the test itself (mkfifo, socket/bind,
// statvfs, stat), not assumptions.
// COUNTERPART: libcxx:input.output/filesystems/fs.op.funcs/fs.op.last_write_time/last_write_time.pass.cpp
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <string>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/un.h>
#include <unistd.h>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

void check_other(const fs::path& p, fs::file_type t) {
  const fs::file_status s = fs::status(p);
  CHECK(s.type() == t);
  CHECK(fs::is_other(s) && fs::is_other(p) && fs::exists(p));
  CHECK(!fs::is_regular_file(p) && !fs::is_directory(p) && !fs::is_symlink(p));
  CHECK(fs::is_fifo(s) == (t == fs::file_type::fifo) && fs::is_fifo(p) == (t == fs::file_type::fifo));
  CHECK(fs::is_socket(s) == (t == fs::file_type::socket) && fs::is_socket(p) == (t == fs::file_type::socket));
  CHECK(fs::is_character_file(s) == (t == fs::file_type::character));
  CHECK(fs::is_block_file(s) == (t == fs::file_type::block));
  std::error_code ec = std::make_error_code(std::errc::io_error);
  CHECK(fs::is_other(p, ec) && !ec);
  // A symlink to it: status follows the link, symlink_status does not.
  const fs::path l = p.string() + ".lnk";
  fs::create_symlink(p, l);
  CHECK(fs::status(l).type() == t && fs::is_symlink(l) && fs::is_other(l));
  CHECK(fs::symlink_status(l).type() == fs::file_type::symlink && !fs::is_other(fs::symlink_status(l)));
}

int main() {
  TmpDir tmp;
  const fs::path dir = tmp.str();

  // FIFO.
  const fs::path fifo = dir / "fifo";
  CHECK(::mkfifo(fifo.c_str(), 0600) == 0);
  check_other(fifo, fs::file_type::fifo);

  // Socket (a bound AF_UNIX socket creates the file).
  const fs::path sock = dir / "s";
  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  CHECK(fd >= 0);
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  const std::string sp = sock.string();
  CHECK(sp.size() < sizeof addr.sun_path);
  sp.copy(addr.sun_path, sp.size());
  CHECK(::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) == 0);
  check_other(sock, fs::file_type::socket);
  ::close(fd);

  // A character device: /dev/null if the system has one as such.
  struct stat st{};
  if (::stat("/dev/null", &st) == 0 && S_ISCHR(st.st_mode)) {
    CHECK(fs::is_character_file("/dev/null") && fs::is_other("/dev/null"));
    CHECK(fs::status("/dev/null").type() == fs::file_type::character);
  }

  // The predicates on a missing path: false, and the ec forms report the error.
  const fs::path missing = dir / "missing";
  for (auto f : {+[](const fs::path& p, std::error_code& e) { return fs::is_fifo(p, e); },
                 +[](const fs::path& p, std::error_code& e) { return fs::is_socket(p, e); },
                 +[](const fs::path& p, std::error_code& e) { return fs::is_block_file(p, e); },
                 +[](const fs::path& p, std::error_code& e) { return fs::is_character_file(p, e); },
                 +[](const fs::path& p, std::error_code& e) { return fs::is_other(p, e); }}) {
    std::error_code ec;
    CHECK(!f(missing, ec));
    CHECK(ec);  // status(p, ec) reports not_found as an error ([fs.op.status]/3)
  }
  CHECK(!fs::is_fifo(missing) && !fs::is_other(missing));  // status(p) does not throw for not_found

  // space(): the statvfs values, multiplied by f_frsize.
  struct statvfs vfs{};
  CHECK(::statvfs(dir.c_str(), &vfs) == 0);
  const fs::space_info si = fs::space(dir);
  CHECK(si.capacity == static_cast<std::uintmax_t>(vfs.f_blocks) * vfs.f_frsize);
  CHECK(si.free <= si.capacity && si.available <= si.capacity);
  std::error_code ec;
  const fs::space_info bad = fs::space(missing / "x", ec);
  CHECK(ec);
  constexpr auto all = static_cast<std::uintmax_t>(-1);
  CHECK(bad.capacity == all && bad.free == all && bad.available == all);

  // last_write_time.
  const fs::path file = dir / "f";
  ::close(::open(file.c_str(), O_CREAT | O_WRONLY, 0600));
  const fs::file_time_type set = fs::last_write_time(file) - std::chrono::hours(48);
  fs::last_write_time(file, set);
  const fs::file_time_type got = fs::last_write_time(file);
  CHECK(got <= set + std::chrono::seconds(2) && got + std::chrono::seconds(2) >= set);
  struct stat fst{};
  CHECK(::stat(file.c_str(), &fst) == 0);
  const fs::file_time_type now = fs::file_time_type::clock::now();
  CHECK(got < now - std::chrono::hours(47));
  ec.clear();
  CHECK(fs::last_write_time(missing, ec) == fs::file_time_type::min() && ec);
  ec.clear();
  fs::last_write_time(missing, set, ec);
  CHECK(ec);

  // Hard links.
  CHECK(fs::hard_link_count(file) == 1);
  fs::create_hard_link(file, dir / "h1");
  fs::create_hard_link(file, dir / "h2");
  CHECK(fs::hard_link_count(file) == 3 && fs::hard_link_count(dir / "h2") == 3);
  CHECK(fs::equivalent(file, dir / "h1"));
  ec.clear();
  CHECK(fs::hard_link_count(missing, ec) == static_cast<std::uintmax_t>(-1) && ec);
  return 0;
}
