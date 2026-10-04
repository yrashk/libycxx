// libycxx hosted runtime: the file system library ([filesystems]) on POSIX (Linux, macOS).
//
// Like the other hosted sources that work on files (fstream.cpp works on C stdio), this file
// calls the POSIX API directly rather than through the PAL: the operations are specified "as if
// by" POSIX calls (stat, lstat, mkdir, link, symlink, rename, truncate, statvfs, fchmodat,
// utimensat, ...), so a PAL layer would restate the POSIX interface function by function, and a
// non-POSIX port replaces this file as a whole (DECISIONS §8).
//
// Errors are reported in the generic category: the POSIX error numbers are the errc values.
// remove_all and recursive_directory_iterator walk directories through descriptors (openat,
// fdopendir, unlinkat) and open subdirectories with O_NOFOLLOW, so a directory replaced by a
// symbolic link during the walk is not followed out of the tree.
#include <filesystem>
#include <locale>
#include <vector>
#include <ycxx/core/hash.hpp>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

namespace fs = std::filesystem;
using std::error_code;
using std::size_t;

namespace {

// ---- lexical structure of a pathname ([fs.path.generic]) ----
// No root-names: a pathname is an optional root-directory (a leading run of separators) and a
// relative-path. An element is identified by its offset (see path::iterator in the header).
using sv = std::string_view;
constexpr size_t npos = sv::npos;

bool has_root(sv s) noexcept { return !s.empty() && s[0] == '/'; }

// The offset of the first character after the root-directory (0 without one).
size_t root_end(sv s) noexcept {
  if (!has_root(s))
    return 0;
  size_t f = s.find_first_not_of('/');
  return f == npos ? s.size() : f;
}

// The element after the one at pos (pos < size).
size_t next_elem(sv s, size_t pos) noexcept {
  const size_t n = s.size();
  if (s[pos] == '/') // the root-directory (pos 0) or the trailing empty element (the last one)
    return pos == 0 ? root_end(s) : n;
  size_t e = s.find('/', pos);
  if (e == npos)
    return n;
  size_t f = s.find_first_not_of('/', e);
  return f == npos ? n - 1 : f; // a trailing separator: the empty element, at the last separator
}

// The element before the one at pos (pos > the first element).
size_t prev_elem(sv s, size_t pos) noexcept {
  const size_t n = s.size();
  size_t last_char; // the last character of the previous filename
  if (pos == n) {
    if (s[n - 1] == '/')
      return s.find_first_not_of('/') == npos ? 0 : n - 1;
    last_char = n - 1;
  } else {
    last_char = s.find_last_not_of('/', pos - 1);
    if (last_char == npos)
      return 0; // only the root-directory precedes
  }
  size_t k = s.rfind('/', last_char);
  return k == npos ? 0 : k + 1;
}

// The text of the element at pos.
sv elem_at(sv s, size_t pos) noexcept {
  if (pos >= s.size())
    return sv();
  if (s[pos] == '/')
    return pos == 0 ? sv("/") : sv();
  size_t e = s.find('/', pos);
  return s.substr(pos, e == npos ? npos : e - pos);
}

sv filename_of(sv s) noexcept {
  if (root_end(s) == s.size() || s.back() == '/')
    return sv();
  size_t k = s.rfind('/');
  return k == npos ? s : s.substr(k + 1);
}

// The offset where the extension of filename f starts (f.size() without one).
size_t extension_start(sv f) noexcept {
  if (f == "." || f == "..")
    return f.size();
  size_t dot = f.rfind('.');
  return dot == npos || dot == 0 ? f.size() : dot;
}

int compare_paths(sv a, sv b) noexcept {
  bool ra = has_root(a), rb = has_root(b);
  if (ra != rb)
    return ra ? 1 : -1;
  size_t i = root_end(a), j = root_end(b);
  while (i < a.size() && j < b.size()) {
    int c = elem_at(a, i).compare(elem_at(b, j));
    if (c != 0)
      return c < 0 ? -1 : 1;
    i = next_elem(a, i);
    j = next_elem(b, j);
  }
  if (i < a.size())
    return 1;
  if (j < b.size())
    return -1;
  return 0;
}

// ---- POSIX helpers ----
error_code errno_code(int e) noexcept { return error_code(e, std::generic_category()); }
error_code last_error() noexcept { return errno_code(errno); }

fs::file_type type_of_mode(mode_t m) noexcept {
  if (S_ISREG(m))
    return fs::file_type::regular;
  if (S_ISDIR(m))
    return fs::file_type::directory;
  if (S_ISLNK(m))
    return fs::file_type::symlink;
  if (S_ISBLK(m))
    return fs::file_type::block;
  if (S_ISCHR(m))
    return fs::file_type::character;
  if (S_ISFIFO(m))
    return fs::file_type::fifo;
  if (S_ISSOCK(m))
    return fs::file_type::socket;
  return fs::file_type::unknown;
}

fs::perms perms_of_mode(mode_t m) noexcept { return static_cast<fs::perms>(m & 07777); }

// [fs.op.status]/6.1: the status that an error of stat/lstat stands for.
fs::file_status status_for_error(int e) noexcept {
  if (e == ENOENT || e == ENOTDIR)
    return fs::file_status(fs::file_type::not_found);
  if (e == EOVERFLOW)
    return fs::file_status(fs::file_type::unknown);
  return fs::file_status();
}

fs::file_status status_from(int r, const struct stat& st, error_code& ec) noexcept {
  if (r != 0) {
    int e = errno;
    ec = errno_code(e);
    return status_for_error(e);
  }
  ec.clear();
  return fs::file_status(type_of_mode(st.st_mode), perms_of_mode(st.st_mode));
}

// The modification time of a struct stat in nanoseconds since the Unix epoch; false if it does
// not fit file_time_type. (st_mtim is POSIX; Darwin names it st_mtimespec.)
template <class Stat>
bool mtime_of(const Stat& st, long long& out) noexcept {
  long long sec, nsec;
  if constexpr (requires { st.st_mtim; }) {
    sec = static_cast<long long>(st.st_mtim.tv_sec);
    nsec = static_cast<long long>(st.st_mtim.tv_nsec);
  } else {
    sec = static_cast<long long>(st.st_mtimespec.tv_sec);
    nsec = static_cast<long long>(st.st_mtimespec.tv_nsec);
  }
  long long ns;
  if (__builtin_mul_overflow(sec, 1'000'000'000LL, &ns) || __builtin_add_overflow(ns, nsec, &ns))
    return false;
  out = ns;
  return true;
}

// Opens a directory stream on a descriptor; closes fd on failure.
DIR* open_dir_fd(int fd) noexcept {
  DIR* d = ::fdopendir(fd);
  if (d == nullptr) {
    int e = errno;
    ::close(fd);
    errno = e;
  }
  return d;
}

} // namespace

// ---- [fs.class.path] ----
namespace std::filesystem {

path& path::operator/=(const path& p) {
  if (p.is_absolute()) { // [fs.path.append]/2
    if (this != __builtin_addressof(p))
      s_ = p.s_;
    return *this;
  }
  if (this == __builtin_addressof(p)) {
    path copy(p);
    return *this /= copy;
  }
  if (has_filename()) // [fs.path.append]/3.1 (no root-names: the absolute case has a root-directory)
    s_.push_back('/');
  s_.append(p.s_);
  return *this;
}

path& path::remove_filename() {
  s_.erase(s_.size() - filename_of(s_).size());
  return *this;
}

path& path::replace_filename(const path& replacement) {
  path r(replacement); // replacement may be *this
  remove_filename();
  return *this /= r;
}

path& path::replace_extension(const path& replacement) {
  path r(replacement);
  sv f = filename_of(s_);
  s_.erase(s_.size() - (f.size() - extension_start(f)));
  if (!r.empty() && r.s_[0] != '.')
    s_.push_back('.');
  s_.append(r.s_);
  return *this;
}

int path::compare(const path& p) const noexcept { return compare_paths(s_, p.s_); }
int path::compare(basic_string_view<value_type> s) const { return compare_paths(s_, s); }

path path::root_name() const { return path(); }
path path::root_directory() const { return has_root(s_) ? path("/") : path(); }
path path::root_path() const { return root_directory(); }
path path::relative_path() const { return path(string_type(sv(s_).substr(root_end(s_)))); }

path path::parent_path() const {
  const size_t n = s_.size();
  if (root_end(s_) == n) // !has_relative_path()
    return *this;
  size_t last = prev_elem(s_, n);
  if (s_[last] == '/') // the trailing empty element: drop the separators
    return path(s_.substr(0, s_.find_last_not_of('/', last) + 1));
  size_t j = s_.find_last_not_of('/', last == 0 ? 0 : last - 1);
  if (last == 0 || j == npos) // the root-directory (or nothing) precedes the last filename
    return path(s_.substr(0, last));
  return path(s_.substr(0, j + 1));
}

path path::filename() const { return path(string_type(filename_of(s_))); }

path path::stem() const {
  sv f = filename_of(s_);
  return path(string_type(f.substr(0, extension_start(f))));
}

path path::extension() const {
  sv f = filename_of(s_);
  return path(string_type(f.substr(extension_start(f))));
}

bool path::has_relative_path() const { return root_end(s_) < s_.size(); }
bool path::has_parent_path() const { return !parent_path().empty(); }
bool path::has_filename() const { return !filename_of(s_).empty(); }
bool path::has_stem() const { return extension_start(filename_of(s_)) != 0; }
bool path::has_extension() const {
  sv f = filename_of(s_);
  return extension_start(f) != f.size();
}

// [fs.path.generic]/6: the normal form, as a stack of filenames.
path path::lexically_normal() const {
  if (s_.empty())
    return path();
  const sv s = s_;
  const bool root = has_root(s);
  std::vector<sv> names;
  bool trailing = false; // the normal form ends in a separator
  for (size_t pos = root_end(s); pos < s.size(); pos = next_elem(s, pos)) {
    sv e = elem_at(s, pos);
    if (e.empty() || e == ".") { // steps 3-4
      trailing = true;
    } else if (e == "..") {
      if (!names.empty() && names.back() != "..") { // step 5
        names.pop_back();
        trailing = true;
      } else if (root) { // step 6
        trailing = true;
      } else {
        names.push_back(e);
        trailing = false;
      }
    } else {
      names.push_back(e);
      trailing = false;
    }
  }
  string_type r = root ? "/" : "";
  if (names.empty())
    return path(root ? r : string_type(".")); // step 8
  for (size_t i = 0; i < names.size(); ++i) {
    if (i != 0)
      r.push_back('/');
    r.append(names[i]);
  }
  if (trailing && names.back() != "..") // step 7
    r.push_back('/');
  return path(static_cast<string_type&&>(r));
}

// [fs.path.gen]/3
path path::lexically_relative(const path& base) const {
  if (is_absolute() != base.is_absolute())
    return path();
  const sv a = s_, b = base.s_;
  size_t i = 0, j = 0; // mismatch(begin(), end(), base.begin(), base.end())
  while (i < a.size() && j < b.size() && elem_at(a, i) == elem_at(b, j)) {
    i = next_elem(a, i);
    j = next_elem(b, j);
  }
  if (i == a.size() && j == b.size())
    return path(".");
  long n = 0;
  for (; j < b.size(); j = next_elem(b, j)) {
    sv e = elem_at(b, j);
    if (e == "..")
      --n;
    else if (!e.empty() && e != ".")
      ++n;
  }
  if (n < 0)
    return path();
  if (n == 0 && (i == a.size() || elem_at(a, i).empty()))
    return path(".");
  path r;
  for (; n > 0; --n)
    r /= path("..");
  for (; i < a.size(); i = next_elem(a, i))
    r /= path(string_type(elem_at(a, i)));
  return r;
}

path path::lexically_proximate(const path& base) const {
  path r = lexically_relative(base);
  return r.empty() ? *this : r;
}

path::iterator path::begin() const {
  iterator it;
  it.p_ = this;
  it.pos_ = 0;
  it.load();
  return it;
}

path::iterator path::end() const {
  iterator it;
  it.p_ = this;
  it.pos_ = s_.size();
  return it;
}

void path::iterator::load() { elem_ = path(string_type(elem_at(p_->s_, pos_))); }

path::iterator& path::iterator::operator++() {
  pos_ = next_elem(p_->s_, pos_);
  load();
  return *this;
}

path::iterator& path::iterator::operator--() {
  pos_ = prev_elem(p_->s_, pos_);
  load();
  return *this;
}

// Equal paths have equal elements ([fs.path.compare]), so the hash combines the elements.
size_t hash_value(const path& p) noexcept {
  const sv s = p.native();
  std::uint64_t h = has_root(s) ? 0x2f : 0;
  for (size_t pos = root_end(s); pos < s.size(); pos = next_elem(s, pos)) {
    sv e = elem_at(s, pos);
    h = ycxx::detail::mum(h ^ ycxx::detail::hash_chars(e.data(), e.size()), ycxx::detail::hash_k1);
  }
  return static_cast<size_t>(h);
}

// ---- [fs.class.filesystem.error] ----
namespace {
shared_ptr<const ycxx::detail::fs_error_data> error_data(const char* base_what, const path* p1, const path* p2) {
  auto d = make_shared<ycxx::detail::fs_error_data>();
  d->what = "filesystem error: ";
  d->what += base_what;
  if (p1 != nullptr) {
    d->p1 = *p1;
    d->what += " [";
    d->what += p1->native();
    d->what += "]";
  }
  if (p2 != nullptr) {
    d->p2 = *p2;
    d->what += " [";
    d->what += p2->native();
    d->what += "]";
  }
  return d;
}
} // namespace

filesystem_error::filesystem_error(const string& what_arg, error_code ec)
    : system_error(ec, what_arg), data_(error_data(system_error::what(), nullptr, nullptr)) {}
filesystem_error::filesystem_error(const string& what_arg, const path& p1, error_code ec)
    : system_error(ec, what_arg), data_(error_data(system_error::what(), __builtin_addressof(p1), nullptr)) {}
filesystem_error::filesystem_error(const string& what_arg, const path& p1, const path& p2, error_code ec)
    : system_error(ec, what_arg),
      data_(error_data(system_error::what(), __builtin_addressof(p1), __builtin_addressof(p2))) {}
filesystem_error::~filesystem_error() = default;

} // namespace std::filesystem

// [fs.path.construct]/6
std::string ycxx::detail::fs_native_through_locale(const char* first, const char* last, const std::locale& loc) {
  using cvt_t = std::codecvt<wchar_t, char, std::mbstate_t>;
  const cvt_t& cvt = std::use_facet<cvt_t>(loc);
  std::wstring wide;
  if (cvt.always_noconv()) {
    for (; first != last; ++first)
      wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*first)));
  } else {
    std::mbstate_t state{};
    wchar_t buf[64];
    while (first != last) {
      const char* from_next = first;
      wchar_t* to_next = buf;
      auto res = cvt.in(state, first, last, from_next, buf, buf + 64, to_next);
      wide.append(buf, to_next);
      if (res == std::codecvt_base::error || (res == std::codecvt_base::partial && from_next == first)) {
        // [fs.path.type.cvt]/3: an unrepresentable character converts to something unspecified
        wide.push_back(L'�');
        from_next = first + 1;
        state = std::mbstate_t{};
      } else if (res == std::codecvt_base::noconv) {
        for (; first != last; ++first)
          wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*first)));
        break;
      }
      first = from_next;
    }
  }
  std::string out;
  ::ycxx::detail::utf_append<wchar_t>(out, wide.data(), wide.data() + wide.size());
  return out;
}

// ---- directory iteration ----
namespace ycxx::detail {

struct fs_dir_state {
  DIR* dir = nullptr;
  fs::path root;
  fs::directory_entry entry;

  fs_dir_state() = default;
  fs_dir_state(const fs_dir_state&) = delete;
  fs_dir_state& operator=(const fs_dir_state&) = delete;
  ~fs_dir_state() {
    if (dir != nullptr)
      ::closedir(dir);
  }

  // Reads the next entry of d (not "." or "..") into e, as dir / name with the file type of the
  // listing cached. Returns 1, 0 at the end, -1 (errno set) on error.
  static int read_next(DIR* d, const fs::path& dir, fs::directory_entry& e) {
    for (;;) {
      errno = 0;
      struct dirent* de = ::readdir(d);
      if (de == nullptr)
        return errno != 0 ? -1 : 0;
      const char* name = de->d_name;
      if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        continue;
      e.path_ = dir;
      e.path_ /= fs::path(name);
      e.cache_ = fs_attr_cache();
      fs::file_type t = fs::file_type::none;
      switch (de->d_type) {
      case DT_REG:
        t = fs::file_type::regular;
        break;
      case DT_DIR:
        t = fs::file_type::directory;
        break;
      case DT_LNK:
        t = fs::file_type::symlink;
        break;
      case DT_BLK:
        t = fs::file_type::block;
        break;
      case DT_CHR:
        t = fs::file_type::character;
        break;
      case DT_FIFO:
        t = fs::file_type::fifo;
        break;
      case DT_SOCK:
        t = fs::file_type::socket;
        break;
      default:
        break;
      }
      if (t != fs::file_type::none) {
        e.cache_.level = 1;
        e.cache_.sym_type = t;
        if (t != fs::file_type::symlink)
          e.cache_.type = t;
      }
      return 1;
    }
  }
};

struct fs_rec_state {
  struct level {
    DIR* dir;
    fs::path path;
  };
  std::vector<level> stack;
  fs::directory_options options = fs::directory_options::none;
  fs::directory_entry entry;

  fs_rec_state() = default;
  fs_rec_state(const fs_rec_state&) = delete;
  fs_rec_state& operator=(const fs_rec_state&) = delete;
  ~fs_rec_state() {
    for (level& l : stack)
      ::closedir(l.dir);
  }

  bool has(fs::directory_options o) const noexcept { return (options & o) != fs::directory_options::none; }

  // Whether the current entry is a directory to descend into ([fs.rec.dir.itr.members]/21.2),
  // and whether it is reached through a symbolic link. When its type cannot be determined
  // (file_type::none), the attempt to open it reports the error (or skips a permission error
  // on request), as the evaluation of status() in /21.2 would.
  bool descend(bool& through_link) const noexcept {
    error_code ec;
    fs::file_type lt = entry.type_of(true, ec);
    through_link = lt == fs::file_type::symlink;
    if (lt == fs::file_type::directory || lt == fs::file_type::none)
      return true;
    if (!through_link || !has(fs::directory_options::follow_directory_symlink))
      return false;
    fs::file_type t = entry.type_of(false, ec);
    return t == fs::file_type::directory || t == fs::file_type::none;
  }
};

} // namespace ycxx::detail

namespace std::filesystem {

void directory_iterator::open(const path& p, directory_options options, error_code& ec) {
  ec.clear();
  state_.reset();
  auto st = make_shared<ycxx::detail::fs_dir_state>();
  st->root = p;
  st->dir = ::opendir(p.c_str());
  if (st->dir == nullptr) {
    int e = errno;
    if (e == EACCES && (options & directory_options::skip_permission_denied) != directory_options::none)
      return;
    ec = errno_code(e);
    return;
  }
  int r = ycxx::detail::fs_dir_state::read_next(st->dir, st->root, st->entry);
  if (r < 0)
    ec = last_error();
  else if (r > 0)
    state_ = static_cast<shared_ptr<ycxx::detail::fs_dir_state>&&>(st);
}

void directory_iterator::advance(error_code& ec, path* where) {
  ec.clear();
  if (state_ == nullptr)
    return;
  int r = ycxx::detail::fs_dir_state::read_next(state_->dir, state_->root, state_->entry);
  if (r < 0) {
    ec = last_error();
    if (where != nullptr)
      *where = state_->root;
  }
  if (r <= 0)
    state_.reset();
}

const directory_entry& directory_iterator::operator*() const {
  ycxx::detail::precondition(state_ != nullptr, "directory_iterator: the end iterator is not dereferenceable");
  return state_->entry;
}

void recursive_directory_iterator::open(const path& p, directory_options options, error_code& ec) {
  ec.clear();
  state_.reset();
  pending_ = true;
  auto st = make_shared<ycxx::detail::fs_rec_state>();
  st->options = options;
  st->stack.reserve(8);
  DIR* d = ::opendir(p.c_str());
  if (d == nullptr) {
    int e = errno;
    if (e == EACCES && st->has(directory_options::skip_permission_denied))
      return;
    ec = errno_code(e);
    return;
  }
  st->stack.push_back({d, p});
  int r = ycxx::detail::fs_dir_state::read_next(d, st->stack.back().path, st->entry);
  if (r < 0)
    ec = last_error();
  else if (r > 0)
    state_ = static_cast<shared_ptr<ycxx::detail::fs_rec_state>&&>(st);
}

void recursive_directory_iterator::advance(error_code& ec, path* where) {
  ec.clear();
  if (state_ == nullptr)
    return;
  ycxx::detail::fs_rec_state& st = *state_;
  if (pending_) {
    bool through_link = false;
    if (st.descend(through_link)) {
      // A directory entry that is not a symbolic link is opened relative to its parent's
      // descriptor without following links, so a directory swapped for a link is never entered.
      // A symbolic link followed on request is opened by its whole path: /21.2 recurses into
      // (*this)->path() once is_directory((*this)->status()) holds, so its resolution, and its
      // limit on symbolic links, is that path's. A loop (d/self -> .) then ends with ELOOP,
      // which status() reports as an error (file_type::none, [fs.op.status]/6.1.3).
      int fd = through_link
                 ? ::open(st.entry.path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)
                 : ::openat(::dirfd(st.stack.back().dir), st.entry.path().filename().c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
      DIR* d = fd < 0 ? nullptr : open_dir_fd(fd);
      if (d != nullptr) {
        try {
          st.stack.push_back({d, st.entry.path()});
        } catch (...) {
          ::closedir(d);
          throw;
        }
      } else {
        int e = errno;
        // A directory that is gone or was replaced by a non-directory since it was listed is
        // simply not entered; a permission error may be skipped on request.
        bool skip = e == ENOENT || e == ENOTDIR || (e == ELOOP && !through_link) ||
                    (e == EACCES && st.has(directory_options::skip_permission_denied));
        if (!skip) {
          ec = errno_code(e);
          if (where != nullptr)
            *where = st.entry.path();
          state_.reset();
          return;
        }
      }
    }
  }
  pending_ = true;
  for (;;) {
    int r = ycxx::detail::fs_dir_state::read_next(st.stack.back().dir, st.stack.back().path, st.entry);
    if (r > 0)
      return;
    if (r < 0) {
      ec = last_error();
      if (where != nullptr)
        *where = st.stack.back().path;
      state_.reset();
      return;
    }
    ::closedir(st.stack.back().dir);
    st.stack.pop_back();
    if (st.stack.empty()) {
      state_.reset();
      return;
    }
  }
}

void recursive_directory_iterator::pop(error_code& ec, path* where) {
  ec.clear();
  ycxx::detail::precondition(state_ != nullptr, "recursive_directory_iterator::pop: not dereferenceable");
  ycxx::detail::fs_rec_state& st = *state_;
  if (st.stack.size() == 1) {
    state_.reset();
    return;
  }
  ::closedir(st.stack.back().dir);
  st.stack.pop_back();
  // Continue in the parent without entering the directory just left. pop() is not an increment:
  // recursion_pending() keeps its value.
  const bool pending = pending_;
  pending_ = false;
  advance(ec, where);
  pending_ = pending;
}

directory_options recursive_directory_iterator::options() const {
  ycxx::detail::precondition(state_ != nullptr, "recursive_directory_iterator::options: not dereferenceable");
  return state_->options;
}
int recursive_directory_iterator::depth() const {
  ycxx::detail::precondition(state_ != nullptr, "recursive_directory_iterator::depth: not dereferenceable");
  return static_cast<int>(state_->stack.size()) - 1;
}
bool recursive_directory_iterator::recursion_pending() const {
  ycxx::detail::precondition(state_ != nullptr, "recursive_directory_iterator::recursion_pending: not dereferenceable");
  return pending_;
}
void recursive_directory_iterator::disable_recursion_pending() {
  ycxx::detail::precondition(state_ != nullptr,
                             "recursive_directory_iterator::disable_recursion_pending: not dereferenceable");
  pending_ = false;
}
const directory_entry& recursive_directory_iterator::operator*() const {
  ycxx::detail::precondition(state_ != nullptr, "recursive_directory_iterator: the end iterator is not dereferenceable");
  return state_->entry;
}

// ---- [fs.class.directory.entry] ----
void directory_entry::refresh(error_code& ec) noexcept {
  cache_ = ycxx::detail::fs_attr_cache();
  cache_.level = 2;
  struct stat st;
  if (::lstat(path_.c_str(), &st) != 0) {
    int e = errno;
    cache_.sym_err = cache_.stat_err = e;
    cache_.sym_type = cache_.type = status_for_error(e).type();
    ec = errno_code(e); // the throwing refresh() ignores a missing file
    return;
  }
  cache_.sym_type = type_of_mode(st.st_mode);
  cache_.sym_perms = perms_of_mode(st.st_mode);
  if (S_ISLNK(st.st_mode) && ::stat(path_.c_str(), &st) != 0) {
    int e = errno;
    cache_.stat_err = e;
    cache_.type = status_for_error(e).type();
    ec.clear();
    return;
  }
  cache_.type = type_of_mode(st.st_mode);
  cache_.perms = perms_of_mode(st.st_mode);
  cache_.size = static_cast<uintmax_t>(st.st_size);
  cache_.nlink = static_cast<uintmax_t>(st.st_nlink);
  if (!mtime_of(st, cache_.mtime))
    cache_.stat_err = EOVERFLOW; // reported by last_write_time; status is still cached
  ec.clear();
}

file_status directory_entry::status(error_code& ec) const noexcept {
  if (cache_.level == 2 && (cache_.stat_err == 0 || cache_.stat_err == EOVERFLOW)) {
    ec.clear();
    return file_status(cache_.type, cache_.perms);
  }
  if (cache_.level == 2) {
    ec = errno_code(cache_.stat_err);
    return file_status(cache_.type);
  }
  return filesystem::status(path_, ec);
}

file_status directory_entry::symlink_status(error_code& ec) const noexcept {
  if (cache_.level == 2) {
    if (cache_.sym_err != 0) {
      ec = errno_code(cache_.sym_err);
      return file_status(cache_.sym_type);
    }
    ec.clear();
    return file_status(cache_.sym_type, cache_.sym_perms);
  }
  return filesystem::symlink_status(path_, ec);
}

file_type directory_entry::type_of(bool link, error_code& ec) const noexcept {
  if (cache_.level == 1 && (link || cache_.sym_type != file_type::symlink)) {
    ec.clear();
    return cache_.sym_type;
  }
  return (link ? symlink_status(ec) : status(ec)).type();
}

uintmax_t directory_entry::file_size(error_code& ec) const noexcept {
  if (cache_.level == 2 && (cache_.stat_err == 0 || cache_.stat_err == EOVERFLOW) &&
      cache_.type == file_type::regular) {
    ec.clear();
    return cache_.size;
  }
  return filesystem::file_size(path_, ec);
}

uintmax_t directory_entry::hard_link_count(error_code& ec) const noexcept {
  if (cache_.level == 2 && (cache_.stat_err == 0 || cache_.stat_err == EOVERFLOW)) {
    ec.clear();
    return cache_.nlink;
  }
  return filesystem::hard_link_count(path_, ec);
}

file_time_type directory_entry::last_write_time(error_code& ec) const noexcept {
  if (cache_.level == 2 && cache_.stat_err == 0) {
    ec.clear();
    return file_time_type(file_time_type::duration(cache_.mtime));
  }
  return filesystem::last_write_time(path_, ec);
}

// ---- [fs.op.funcs] ----

path absolute(const path& p, error_code& ec) {
  ec.clear();
  if (p.is_absolute())
    return p;
  path cwd = current_path(ec);
  if (ec)
    return path();
  return cwd / p;
}

path canonical(const path& p, error_code& ec) {
  ec.clear();
  if (p.empty()) {
    ec = errno_code(ENOENT);
    return path();
  }
  path abs = absolute(p, ec);
  if (ec)
    return path();
  char* r = ::realpath(abs.c_str(), nullptr);
  if (r == nullptr) {
    ec = last_error();
    return path();
  }
  struct guard {
    char* p;
    ~guard() { ::free(p); }
  } g{r};
  return path(r);
}

namespace {
constexpr copy_options in_recursive_copy = static_cast<copy_options>(512);
bool has_opt(copy_options o) noexcept { return o != copy_options::none; }
} // namespace

// [fs.op.copy]
void copy(const path& from, const path& to, copy_options options, error_code& ec) {
  ec.clear();
  const bool create_or_skip_links = has_opt(options & (copy_options::create_symlinks | copy_options::skip_symlinks));
  const bool copy_links = has_opt(options & copy_options::copy_symlinks);
  file_status f = create_or_skip_links || copy_links ? symlink_status(from, ec) : status(from, ec);
  if (!exists(f)) { // (4.5.1)
    if (!ec)
      ec = errno_code(ENOENT);
    return;
  }
  error_code tec;
  file_status t = create_or_skip_links ? symlink_status(to, tec) : status(to, tec);
  if (t.type() == file_type::none) {
    ec = tec;
    return;
  }
  ec.clear();
  if (exists(t)) { // (4.5.2)
    error_code eq;
    if (equivalent(from, to, eq)) {
      ec = errno_code(EEXIST);
      return;
    }
  }
  if (is_other(f) || is_other(t)) { // (4.5.3)
    ec = errno_code(ENOTSUP);
    return;
  }
  if (is_directory(f) && is_regular_file(t)) { // (4.5.4)
    ec = errno_code(EISDIR);
    return;
  }
  if (is_symlink(f)) { // (4.6)
    if (has_opt(options & copy_options::skip_symlinks))
      return;
    if (!exists(t) && copy_links)
      copy_symlink(from, to, ec);
    else // (4.6.3)
      ec = errno_code(exists(t) ? EEXIST : ENOTSUP);
    return;
  }
  if (is_regular_file(f)) { // (4.7)
    if (has_opt(options & copy_options::directories_only))
      return;
    if (has_opt(options & copy_options::create_symlinks))
      create_symlink(from, to, ec);
    else if (has_opt(options & copy_options::create_hard_links))
      create_hard_link(from, to, ec);
    else if (is_directory(t))
      copy_file(from, to / from.filename(), options, ec);
    else
      copy_file(from, to, options, ec);
    return;
  }
  if (is_directory(f) && has_opt(options & copy_options::create_symlinks)) { // (4.8)
    ec = errno_code(EISDIR);
    return;
  }
  if (is_directory(f) && (has_opt(options & copy_options::recursive) || options == copy_options::none)) { // (4.9)
    if (!exists(t)) {
      create_directory(to, from, ec);
      if (ec)
        return;
    }
    directory_iterator it(from, ec);
    for (; !ec && it != directory_iterator(); it.increment(ec)) {
      const path& x = it->path();
      copy(x, to / x.filename(), options | in_recursive_copy, ec);
      if (ec)
        return;
    }
  }
}

// [fs.op.copy.file]
bool copy_file(const path& from, const path& to, copy_options options, error_code& ec) {
  ec.clear();
  struct stat fst;
  if (::stat(from.c_str(), &fst) != 0) {
    ec = last_error();
    return false;
  }
  if (!S_ISREG(fst.st_mode)) { // (4.1.1); checked before opening, which could block on a FIFO
    ec = errno_code(S_ISDIR(fst.st_mode) ? EISDIR : ENOTSUP);
    return false;
  }
  struct stat tst;
  bool to_exists = ::stat(to.c_str(), &tst) == 0;
  if (!to_exists && errno != ENOENT) {
    ec = last_error();
    return false;
  }
  if (to_exists) {
    if (!S_ISREG(tst.st_mode)) { // (4.1.2)
      ec = errno_code(S_ISDIR(tst.st_mode) ? EISDIR : ENOTSUP);
      return false;
    }
    if (fst.st_dev == tst.st_dev && fst.st_ino == tst.st_ino) { // (4.1.3)
      ec = errno_code(EEXIST);
      return false;
    }
    if (!has_opt(options & (copy_options::skip_existing | copy_options::overwrite_existing |
                        copy_options::update_existing))) { // (4.1.4)
      ec = errno_code(EEXIST);
      return false;
    }
    if (!has_opt(options & copy_options::overwrite_existing)) {
      if (has_opt(options & copy_options::skip_existing))
        return false;
      long long ft = 0, tt = 0; // update_existing: copy only if from is more recent
      if (!mtime_of(fst, ft) || !mtime_of(tst, tt)) {
        ec = errno_code(EOVERFLOW);
        return false;
      }
      if (ft <= tt)
        return false;
    }
  }
  int in = ::open(from.c_str(), O_RDONLY | O_CLOEXEC | O_NOCTTY);
  if (in < 0) {
    ec = last_error();
    return false;
  }
  struct stat ist;
  int fe = ::fstat(in, &ist) != 0 ? errno : S_ISREG(ist.st_mode) ? 0 : ENOTSUP; // replaced since the stat?
  if (fe != 0) {
    ec = errno_code(fe);
    ::close(in);
    return false;
  }
  const mode_t mode = ist.st_mode & 07777;
  int out = ::open(to.c_str(), O_WRONLY | O_CLOEXEC | O_NOCTTY | (to_exists ? O_TRUNC : O_CREAT | O_EXCL), mode);
  if (out < 0) {
    ec = last_error();
    ::close(in);
    return false;
  }
  char buf[32768];
  int err = 0;
  for (;;) {
    ssize_t n = ::read(in, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      err = errno;
      break;
    }
    if (n == 0)
      break;
    for (ssize_t done = 0; done < n;) {
      ssize_t w = ::write(out, buf + done, static_cast<size_t>(n - done));
      if (w < 0) {
        if (errno == EINTR)
          continue;
        err = errno;
        break;
      }
      done += w;
    }
    if (err != 0)
      break;
  }
  if (err == 0 && ::fchmod(out, mode) != 0) // the attributes of from (open's mode is umasked)
    err = errno;
  ::close(in);
  if (::close(out) != 0 && err == 0)
    err = errno;
  if (err != 0) {
    ec = errno_code(err);
    return false;
  }
  return true;
}

void copy_symlink(const path& existing_symlink, const path& new_symlink, error_code& ec) noexcept {
  path target = read_symlink(existing_symlink, ec);
  if (ec)
    return;
  create_symlink(target, new_symlink, ec); // POSIX makes no difference for directory links
}

// [fs.op.create.directories]
bool create_directories(const path& p, error_code& ec) {
  ec.clear();
  error_code sec;
  file_status st = status(p, sec);
  if (exists(st)) {
    if (!is_directory(st))
      ec = errno_code(EEXIST);
    return false;
  }
  if (st.type() == file_type::none) {
    ec = sec;
    return false;
  }
  // The elements that do not exist, innermost first. A trailing "." or empty filename names the
  // same directory as its parent, so it gets no mkdir of its own.
  std::vector<path> missing;
  path cur = p;
  for (;;) {
    path f = cur.filename();
    if (!f.empty() && f.native() != ".")
      missing.push_back(cur);
    path parent = cur.parent_path();
    if (parent.empty() || parent == cur)
      break;
    st = status(parent, sec);
    if (exists(st)) {
      if (!is_directory(st)) {
        ec = errno_code(ENOTDIR);
        return false;
      }
      break;
    }
    if (st.type() == file_type::none) {
      ec = sec;
      return false;
    }
    cur = static_cast<path&&>(parent);
  }
  if (missing.empty()) { // p names no directory to create (the empty path)
    ec = errno_code(ENOENT);
    return false;
  }
  bool created = false;
  for (size_t i = missing.size(); i-- > 0;) {
    created = create_directory(missing[i], ec);
    if (ec)
      return false;
  }
  return created;
}

namespace {
// mkdir with [fs.op.create.directory]'s rule: an existing directory is not an error.
bool make_directory(const path& p, mode_t mode, error_code& ec) noexcept {
  if (::mkdir(p.c_str(), mode) == 0) {
    ec.clear();
    return true;
  }
  int e = errno;
  struct stat st;
  if (e == EEXIST && ::stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
    ec.clear();
    return false;
  }
  ec = errno_code(e);
  return false;
}
} // namespace

bool create_directory(const path& p, error_code& ec) noexcept {
  return make_directory(p, static_cast<mode_t>(perms::all), ec);
}

bool create_directory(const path& p, const path& existing_p, error_code& ec) noexcept {
  struct stat st;
  if (::stat(existing_p.c_str(), &st) != 0) {
    ec = errno == ENOENT ? errno_code(ENOTDIR) : last_error(); // existing_p is not a directory
    return false;
  }
  if (!S_ISDIR(st.st_mode)) {
    ec = errno_code(ENOTDIR);
    return false;
  }
  return make_directory(p, st.st_mode & 07777, ec);
}

void create_directory_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept {
  create_symlink(to, new_symlink, ec);
}

void create_hard_link(const path& to, const path& new_hard_link, error_code& ec) noexcept {
  if (::link(to.c_str(), new_hard_link.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

void create_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept {
  if (::symlink(to.c_str(), new_symlink.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

path current_path(error_code& ec) {
  ec.clear();
  std::string buf(256, '\0');
  for (;;) {
    if (::getcwd(buf.data(), buf.size()) != nullptr) {
      buf.resize(std::char_traits<char>::length(buf.data()));
      return path(static_cast<std::string&&>(buf));
    }
    if (errno != ERANGE) {
      ec = last_error();
      return path();
    }
    buf.resize(buf.size() * 2);
  }
}

void current_path(const path& p, error_code& ec) noexcept {
  if (::chdir(p.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

bool equivalent(const path& p1, const path& p2, error_code& ec) noexcept {
  struct stat s1, s2;
  if (::stat(p1.c_str(), &s1) != 0 || ::stat(p2.c_str(), &s2) != 0) {
    ec = last_error();
    return false;
  }
  ec.clear();
  return s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino;
}

uintmax_t file_size(const path& p, error_code& ec) noexcept {
  struct stat st;
  if (::stat(p.c_str(), &st) != 0) {
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  if (!S_ISREG(st.st_mode)) { // implementation-defined ([fs.op.file.size]/2.2): an error
    ec = errno_code(S_ISDIR(st.st_mode) ? EISDIR : ENOTSUP);
    return static_cast<uintmax_t>(-1);
  }
  ec.clear();
  return static_cast<uintmax_t>(st.st_size);
}

uintmax_t hard_link_count(const path& p, error_code& ec) noexcept {
  struct stat st;
  if (::stat(p.c_str(), &st) != 0) {
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  ec.clear();
  return static_cast<uintmax_t>(st.st_nlink);
}

bool is_empty(const path& p, error_code& ec) {
  file_status s = status(p, ec);
  if (ec)
    return false;
  if (is_directory(s)) {
    directory_iterator it(p, ec);
    if (ec)
      return false;
    return it == directory_iterator();
  }
  uintmax_t sz = file_size(p, ec);
  if (ec)
    return false;
  return sz == 0;
}

file_time_type last_write_time(const path& p, error_code& ec) noexcept {
  struct stat st;
  long long ns = 0;
  if (::stat(p.c_str(), &st) != 0) {
    ec = last_error();
    return file_time_type::min();
  }
  if (!mtime_of(st, ns)) {
    ec = errno_code(EOVERFLOW);
    return file_time_type::min();
  }
  ec.clear();
  return file_time_type(file_time_type::duration(ns));
}

void last_write_time(const path& p, file_time_type new_time, error_code& ec) noexcept {
  long long ns = new_time.time_since_epoch().count();
  long long sec = ns / 1'000'000'000, rem = ns % 1'000'000'000;
  if (rem < 0) { // timespec wants 0 <= tv_nsec < 10^9
    rem += 1'000'000'000;
    --sec;
  }
  struct timespec ts[2];
  ts[0].tv_sec = 0;
  ts[0].tv_nsec = UTIME_OMIT; // leave the access time alone
  ts[1].tv_sec = static_cast<time_t>(sec);
  ts[1].tv_nsec = static_cast<long>(rem);
  if (::utimensat(AT_FDCWD, p.c_str(), ts, 0) != 0)
    ec = last_error();
  else
    ec.clear();
}

// [fs.op.permissions]
void permissions(const path& p, perms prms, perm_options opts, error_code& ec) {
  ec.clear();
  const bool add = (opts & perm_options::add) != perm_options{};
  const bool rem = (opts & perm_options::remove) != perm_options{};
  const bool rep = (opts & perm_options::replace) != perm_options{};
  const bool nofollow = (opts & perm_options::nofollow) != perm_options{};
  if (int(add) + int(rem) + int(rep) != 1) {
    ec = errno_code(EINVAL);
    return;
  }
  prms &= perms::mask;
  file_status st;
  if (add || rem || nofollow) {
    st = nofollow ? symlink_status(p, ec) : status(p, ec);
    if (ec)
      return;
    if (add)
      prms = st.permissions() | prms;
    else if (rem)
      prms = st.permissions() & ~prms;
  }
  int flags = nofollow && is_symlink(st) ? AT_SYMLINK_NOFOLLOW : 0;
  if (::fchmodat(AT_FDCWD, p.c_str(), static_cast<mode_t>(prms & perms::mask), flags) != 0)
    ec = last_error();
}

path read_symlink(const path& p, error_code& ec) {
  ec.clear();
  std::string buf(256, '\0');
  for (;;) {
    ssize_t n = ::readlink(p.c_str(), buf.data(), buf.size());
    if (n < 0) {
      ec = last_error();
      return path();
    }
    if (static_cast<size_t>(n) < buf.size()) {
      buf.resize(static_cast<size_t>(n));
      return path(static_cast<std::string&&>(buf));
    }
    buf.resize(buf.size() * 2); // possibly truncated
  }
}

bool remove(const path& p, error_code& ec) noexcept {
  if (::remove(p.c_str()) == 0) {
    ec.clear();
    return true;
  }
  int e = errno;
  if (e == ENOENT || e == ENOTDIR) { // [fs.op.remove]/3 Note 2: absence is not an error
    ec.clear();
    return false;
  }
  ec = errno_code(e);
  return false;
}

namespace {
// Removes everything in the directory open on dfd (which it closes), counting into n; returns 0
// or an error number. Entries are opened relative to their directory and never through a
// symbolic link. Passes are repeated until one finds nothing: some file systems skip entries
// when a directory is modified while it is being read.
int remove_contents(int dfd, uintmax_t& n) {
  DIR* d = open_dir_fd(dfd);
  if (d == nullptr)
    return errno;
  int err = 0;
  for (bool found = true; found && err == 0;) {
    found = false;
    ::rewinddir(d);
    for (;;) {
      errno = 0;
      struct dirent* de = ::readdir(d);
      if (de == nullptr) {
        err = errno;
        break;
      }
      const char* name = de->d_name;
      if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        continue;
      found = true;
      bool dir = de->d_type == DT_DIR;
      if (de->d_type == DT_UNKNOWN) {
        struct stat st;
        if (::fstatat(dfd, name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
          if (errno == ENOENT)
            continue;
          err = errno;
          break;
        }
        dir = S_ISDIR(st.st_mode);
      }
      if (dir) {
        int sub = ::openat(dfd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (sub < 0) {
          if (errno == ENOENT)
            continue;
          if (errno != ENOTDIR && errno != ELOOP) {
            err = errno;
            break;
          }
          dir = false; // replaced by a non-directory since it was listed
        } else {
          err = remove_contents(sub, n);
          if (err != 0)
            break;
        }
      }
      if (::unlinkat(dfd, name, dir ? AT_REMOVEDIR : 0) != 0) {
        if (errno == ENOENT)
          continue;
        err = errno;
        break;
      }
      ++n;
    }
  }
  ::closedir(d);
  return err;
}
} // namespace

// [fs.op.remove.all]
uintmax_t remove_all(const path& p, error_code& ec) {
  ec.clear();
  struct stat st;
  if (::lstat(p.c_str(), &st) != 0) {
    if (errno == ENOENT || errno == ENOTDIR)
      return 0;
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  uintmax_t n = 0;
  if (S_ISDIR(st.st_mode)) {
    int fd = ::open(p.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd >= 0) {
      int err = remove_contents(fd, n);
      if (err != 0) {
        ec = errno_code(err);
        return static_cast<uintmax_t>(-1);
      }
    } else if (errno == ENOENT) {
      return 0;
    } else if (errno != ENOTDIR && errno != ELOOP) {
      ec = last_error();
      return static_cast<uintmax_t>(-1);
    }
  }
  if (::remove(p.c_str()) != 0) {
    if (errno == ENOENT)
      return n;
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  return n + 1;
}

void rename(const path& from, const path& to, error_code& ec) noexcept {
  if (::rename(from.c_str(), to.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

void resize_file(const path& p, uintmax_t size, error_code& ec) noexcept {
  if (size > static_cast<uintmax_t>(numeric_limits<off_t>::max())) {
    ec = errno_code(EFBIG);
    return;
  }
  if (::truncate(p.c_str(), static_cast<off_t>(size)) != 0)
    ec = last_error();
  else
    ec.clear();
}

space_info space(const path& p, error_code& ec) noexcept {
  struct statvfs sv;
  if (::statvfs(p.c_str(), &sv) != 0) {
    ec = last_error();
    const uintmax_t unknown = static_cast<uintmax_t>(-1);
    return space_info{unknown, unknown, unknown};
  }
  ec.clear();
  const uintmax_t unit = sv.f_frsize;
  return space_info{static_cast<uintmax_t>(sv.f_blocks) * unit, static_cast<uintmax_t>(sv.f_bfree) * unit,
                    static_cast<uintmax_t>(sv.f_bavail) * unit};
}

file_status status(const path& p, error_code& ec) noexcept {
  struct stat st;
  int r = ::stat(p.c_str(), &st);
  return status_from(r, st, ec);
}

file_status symlink_status(const path& p, error_code& ec) noexcept {
  struct stat st;
  int r = ::lstat(p.c_str(), &st);
  return status_from(r, st, ec);
}

// [fs.op.temp.dir.path]: TMPDIR, TMP, TEMP, TEMPDIR, else /tmp.
path temp_directory_path(error_code& ec) {
  const char* dir = nullptr;
  for (const char* var : {"TMPDIR", "TMP", "TEMP", "TEMPDIR"}) {
    dir = ::getenv(var);
    if (dir != nullptr && *dir != '\0')
      break;
    dir = nullptr;
  }
  path p(dir != nullptr ? dir : "/tmp");
  file_status s = status(p, ec);
  if (!is_directory(s)) {
    if (!ec)
      ec = errno_code(ENOTDIR);
    return path();
  }
  return p;
}

// [fs.op.weakly.canonical]
// The walk starts from absolute(p) (on POSIX, current_path() / p), so a relative path whose
// first element does not exist still resolves against the canonical working directory.
path weakly_canonical(const path& p_in, error_code& ec) {
  ec.clear();
  const path p = p_in.empty() ? current_path(ec) : absolute(p_in, ec);
  if (ec)
    return path();
  file_status s = status(p, ec);
  if (s.type() == file_type::none)
    return path();
  if (exists(s))
    return canonical(p, ec);
  // the longest leading sequence of elements that exists
  path head;
  path::iterator split = p.begin();
  for (path::iterator it = p.begin(); it != p.end(); ++it) {
    path next = head;
    next /= *it;
    s = status(next, ec);
    if (s.type() == file_type::none)
      return path();
    if (!exists(s))
      break;
    head = static_cast<path&&>(next);
    split = it;
    ++split;
  }
  ec.clear();
  path r;
  if (!head.empty()) {
    r = canonical(head, ec);
    if (ec)
      return path();
  }
  for (; split != p.end(); ++split)
    r /= *split;
  return r.lexically_normal();
}

} // namespace std::filesystem
