// [fs.op.status]/4-6: status(p, ec) for a missing file sets ec and returns
// file_status(file_type::not_found); the throwing status(p) does not throw for not_found
// (Note 1); regular files and directories report their type and permissions (st_mode & mask).
// [fs.op.exists]/3: exists(p, ec) clears ec when the status is known. [fs.op.file.size]: the
// size of a regular file; "If exists(p) is false, an error is reported", the ec overload
// returning static_cast<uintmax_t>(-1). [fs.op.permissions]: replace / add / remove.
// [fs.op.resize.file]. [fs.class.file.status]: type() / permissions().
// REQUIRES: exceptions
#include <filesystem>
#include <cstdint>
#include <system_error>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;
using fs::perms;

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  const fs::path f = base / "file";
  write_file(f.string(), "12345");

  std::error_code ec;
  fs::file_status st = fs::status(base / "missing", ec);
  CHECK(st.type() == fs::file_type::not_found);
  CHECK(bool(ec));
  CHECK(fs::status(base / "missing").type() == fs::file_type::not_found);  // no throw
  CHECK(!fs::exists(base / "missing", ec));
  CHECK(!ec);  // status known: cleared
  CHECK(!fs::exists(st));
  CHECK(fs::status_known(st));
  CHECK(!fs::status_known(fs::file_status()));  // default: file_type::none
  CHECK(fs::file_status().type() == fs::file_type::none);

  CHECK(fs::status(f).type() == fs::file_type::regular);
  CHECK(fs::status(base).type() == fs::file_type::directory);
  CHECK(fs::is_regular_file(fs::status(f)));
  CHECK(fs::exists(f, ec) && !ec);

  CHECK(fs::file_size(f) == 5);
  CHECK(fs::file_size(base / "missing", ec) == static_cast<std::uintmax_t>(-1));
  CHECK(bool(ec));
  bool threw = false;
  try {
    fs::file_size(base / "missing");
  } catch (const fs::filesystem_error& e) {
    threw = e.path1() == base / "missing" && e.code() == std::errc::no_such_file_or_directory;
  }
  CHECK(threw);

  fs::resize_file(f, 2);
  CHECK(fs::file_size(f) == 2);
  fs::resize_file(f, 10);
  CHECK(fs::file_size(f) == 10);

  fs::permissions(f, perms::owner_read | perms::owner_write);
  CHECK((fs::status(f).permissions() & perms::mask) == (perms::owner_read | perms::owner_write));
  fs::permissions(f, perms::group_read | perms::others_read, fs::perm_options::add);
  CHECK(fs::status(f).permissions() ==
        (perms::owner_read | perms::owner_write | perms::group_read | perms::others_read));
  fs::permissions(f, perms::owner_write, fs::perm_options::remove);
  CHECK(fs::status(f).permissions() == (perms::owner_read | perms::group_read | perms::others_read));
  fs::permissions(f, perms::owner_all, ec);  // replace, error_code overload
  CHECK(!ec);
  CHECK(fs::status(f).permissions() == perms::owner_all);
  fs::permissions(base / "missing", perms::owner_all, ec);
  CHECK(bool(ec));

  // perms values ([fs.enum.perms])
  static_assert(static_cast<int>(perms::owner_all) == 0700);
  static_assert(static_cast<int>(perms::group_all) == 070);
  static_assert(static_cast<int>(perms::others_all) == 07);
  static_assert(static_cast<int>(perms::all) == 0777);
  static_assert(static_cast<int>(perms::set_uid) == 04000);
  static_assert(static_cast<int>(perms::sticky_bit) == 01000);
  static_assert(static_cast<int>(perms::mask) == 07777);
  static_assert(static_cast<int>(perms::unknown) == 0xFFFF);
  return 0;
}
