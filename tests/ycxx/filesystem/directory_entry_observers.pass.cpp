// [fs.dir.entry.obs]/2: path() and the conversion to const path& return the stored path,
// noexcept. /3-20: exists() and is_X() are the namespace-scope functions applied to status()
// (is_symlink() to symlink_status()), with the same results for the error_code forms, which are
// noexcept. /21-30: file_size(), hard_link_count(), last_write_time(), status() and
// symlink_status() return the attribute (cached or not) the namespace-scope function would.
// /31-32: == and <=> compare the paths (strong_ordering, noexcept).
// [fs.dir.entry.cons]/1-2: the constructors call refresh(); path() == p when there is no error.
// [fs.dir.entry.mods]/1-6: assign() and replace_filename() change the path and refresh; refresh()
// stores the current attributes, so a refreshed entry sees a changed file.
#include <filesystem>
#include <compare>
#include <cstdint>
#include <system_error>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;
using DE = fs::directory_entry;

static_assert(noexcept(std::declval<const DE&>().path()));
static_assert(std::is_same_v<decltype(std::declval<const DE&>().path()), const fs::path&>);
static_assert(std::is_nothrow_convertible_v<const DE&, const fs::path&>);
static_assert(noexcept(std::declval<const DE&>().exists(std::declval<std::error_code&>())));
static_assert(noexcept(std::declval<const DE&>().is_symlink(std::declval<std::error_code&>())));
static_assert(noexcept(std::declval<const DE&>().file_size(std::declval<std::error_code&>())));
static_assert(noexcept(std::declval<const DE&>().status(std::declval<std::error_code&>())));
static_assert(noexcept(std::declval<const DE&>() == std::declval<const DE&>()));
static_assert(std::is_same_v<decltype(std::declval<const DE&>() <=> std::declval<const DE&>()),
                             std::strong_ordering>);
static_assert(!std::is_convertible_v<const fs::path&, DE>); // explicit

void same_as_free_functions(const DE& d) {
  const fs::path& p = d.path();
  std::error_code ec1, ec2;
  CHECK(d.exists() == fs::exists(fs::status(p)));
  CHECK(d.is_regular_file() == fs::is_regular_file(p));
  CHECK(d.is_directory() == fs::is_directory(p));
  CHECK(d.is_symlink() == fs::is_symlink(fs::symlink_status(p)));
  CHECK(d.is_other() == fs::is_other(fs::status(p)));
  CHECK(d.is_fifo() == fs::is_fifo(fs::status(p)) && d.is_socket() == fs::is_socket(fs::status(p)));
  CHECK(d.is_block_file() == fs::is_block_file(fs::status(p)));
  CHECK(d.is_character_file() == fs::is_character_file(fs::status(p)));
  CHECK(d.status().type() == fs::status(p).type());
  CHECK(d.status().permissions() == fs::status(p).permissions());
  CHECK(d.symlink_status().type() == fs::symlink_status(p).type());
  CHECK(d.exists(ec1) == d.exists() && d.is_directory(ec2) == d.is_directory());
  CHECK(d.is_symlink(ec1) == d.is_symlink() && d.is_regular_file(ec2) == d.is_regular_file());
}

int main() {
  TmpDir tmp;
  const fs::path base = tmp.str();
  const fs::path file = base / "file";
  const fs::path dir = base / "dir";
  write_file(file.string(), "12345");
  fs::create_directory(dir);
  fs::create_symlink(file, base / "link");
  fs::create_symlink(base / "nothing", base / "dangling");

  DE f(file);
  CHECK(f.path() == file && static_cast<const fs::path&>(f) == file);
  same_as_free_functions(f);
  CHECK(f.exists() && f.is_regular_file() && !f.is_directory() && !f.is_symlink() && !f.is_other());
  CHECK(f.file_size() == 5 && f.hard_link_count() == 1);
  CHECK(f.last_write_time() == fs::last_write_time(file));
  std::error_code ec;
  CHECK(f.file_size(ec) == 5 && !ec);

  DE d(dir);
  same_as_free_functions(d);
  CHECK(d.is_directory() && d.exists() && !d.is_regular_file());

  // A symlink to a file: is_regular_file() follows it, is_symlink() does not.
  DE l(base / "link");
  same_as_free_functions(l);
  CHECK(l.is_symlink() && l.is_regular_file() && l.exists() && l.file_size() == 5);
  CHECK(l.status().type() == fs::file_type::regular && l.symlink_status().type() == fs::file_type::symlink);

  // A dangling symlink: it does not exist, but is a symlink.
  DE dl(base / "dangling");
  same_as_free_functions(dl);
  CHECK(!dl.exists() && dl.is_symlink() && !dl.is_regular_file());
  CHECK(dl.status().type() == fs::file_type::not_found);

  // file_size of a dangling link is an error: (uintmax_t)-1 with ec.
  CHECK(dl.file_size(ec) == static_cast<std::uintmax_t>(-1) && ec);
  ec.clear();
  CHECK(dl.hard_link_count(ec) == static_cast<std::uintmax_t>(-1) && ec);
  ec.clear();
  CHECK(dl.last_write_time(ec) == fs::file_time_type::min() && ec);

  // refresh() stores the current attributes.
  write_file(file.string(), "123456789");
  f.refresh();
  CHECK(f.file_size() == 9);
  fs::create_hard_link(file, base / "hard");
  f.refresh(ec);
  CHECK(!ec && f.hard_link_count() == 2);
  fs::remove(file);
  f.refresh(ec);
  CHECK(!f.exists(ec) && !f.is_regular_file(ec));

  // assign / replace_filename change the path and refresh.
  DE m(dir);
  m.assign(base / "hard");
  CHECK(m.path() == base / "hard" && m.is_regular_file() && m.file_size() == 9);
  m.replace_filename("link");
  CHECK(m.path() == base / "link" && m.is_symlink() && !m.exists()); // link's target is gone
  m.replace_filename("dir", ec);
  CHECK(!ec && m.path() == dir && m.is_directory());

  // Comparisons are path comparisons.
  DE a(dir), b(base / "hard");
  DE a2(dir);
  CHECK(a == a2 && a != b && a < b && (b <=> a) == std::strong_ordering::greater);
  CHECK((a <=> a2) == std::strong_ordering::equal);
  CHECK((DE(dir) < DE(base / "link")) == (dir < base / "link"));
  DE def;
  CHECK(def.path().empty() && def < a);
  return 0;
}
