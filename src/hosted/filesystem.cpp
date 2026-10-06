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

namespace __fs = std::filesystem;
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
  size_t __f = s.find_first_not_of('/');
  return __f == npos ? s.size() : __f;
}

// The element after the one at pos (pos < size).
size_t next_elem(sv s, size_t __pos) noexcept {
  const size_t n = s.size();
  if (s[__pos] == '/') // the root-directory (pos 0) or the trailing empty element (the last one)
    return __pos == 0 ? root_end(s) : n;
  size_t e = s.find('/', __pos);
  if (e == npos)
    return n;
  size_t __f = s.find_first_not_of('/', e);
  return __f == npos ? n - 1 : __f; // a trailing separator: the empty element, at the last separator
}

// The element before the one at pos (pos > the first element).
size_t prev_elem(sv s, size_t __pos) noexcept {
  const size_t n = s.size();
  size_t last_char; // the last character of the previous filename
  if (__pos == n) {
    if (s[n - 1] == '/')
      return s.find_first_not_of('/') == npos ? 0 : n - 1;
    last_char = n - 1;
  } else {
    last_char = s.find_last_not_of('/', __pos - 1);
    if (last_char == npos)
      return 0; // only the root-directory precedes
  }
  size_t k = s.rfind('/', last_char);
  return k == npos ? 0 : k + 1;
}

// The text of the element at pos.
sv elem_at(sv s, size_t __pos) noexcept {
  if (__pos >= s.size())
    return sv();
  if (s[__pos] == '/')
    return __pos == 0 ? sv("/") : sv();
  size_t e = s.find('/', __pos);
  return s.substr(__pos, e == npos ? npos : e - __pos);
}

sv filename_of(sv s) noexcept {
  if (root_end(s) == s.size() || s.back() == '/')
    return sv();
  size_t k = s.rfind('/');
  return k == npos ? s : s.substr(k + 1);
}

// The offset where the extension of filename f starts (f.size() without one).
size_t extension_start(sv __f) noexcept {
  if (__f == "." || __f == "..")
    return __f.size();
  size_t dot = __f.rfind('.');
  return dot == npos || dot == 0 ? __f.size() : dot;
}

int compare_paths(sv a, sv b) noexcept {
  bool __ra = has_root(a), __rb = has_root(b);
  if (__ra != __rb)
    return __ra ? 1 : -1;
  size_t i = root_end(a), __j = root_end(b);
  while (i < a.size() && __j < b.size()) {
    int c = elem_at(a, i).compare(elem_at(b, __j));
    if (c != 0)
      return c < 0 ? -1 : 1;
    i = next_elem(a, i);
    __j = next_elem(b, __j);
  }
  if (i < a.size())
    return 1;
  if (__j < b.size())
    return -1;
  return 0;
}

// ---- POSIX helpers ----
error_code errno_code(int e) noexcept { return error_code(e, std::generic_category()); }
error_code last_error() noexcept { return errno_code(errno); }

__fs::file_type type_of_mode(mode_t m) noexcept {
  if (S_ISREG(m))
    return __fs::file_type::regular;
  if (S_ISDIR(m))
    return __fs::file_type::directory;
  if (S_ISLNK(m))
    return __fs::file_type::symlink;
  if (S_ISBLK(m))
    return __fs::file_type::block;
  if (S_ISCHR(m))
    return __fs::file_type::character;
  if (S_ISFIFO(m))
    return __fs::file_type::fifo;
  if (S_ISSOCK(m))
    return __fs::file_type::socket;
  return __fs::file_type::unknown;
}

__fs::perms perms_of_mode(mode_t m) noexcept { return static_cast<__fs::perms>(m & 07777); }

// [fs.op.status]/6.1: the status that an error of stat/lstat stands for.
__fs::file_status status_for_error(int e) noexcept {
  if (e == ENOENT || e == ENOTDIR)
    return __fs::file_status(__fs::file_type::not_found);
  if (e == EOVERFLOW)
    return __fs::file_status(__fs::file_type::unknown);
  return __fs::file_status();
}

__fs::file_status status_from(int r, const struct stat& __st, error_code& ec) noexcept {
  if (r != 0) {
    int e = errno;
    ec = errno_code(e);
    return status_for_error(e);
  }
  ec.clear();
  return __fs::file_status(type_of_mode(__st.st_mode), perms_of_mode(__st.st_mode));
}

// The modification time of a struct stat in nanoseconds since the Unix epoch; false if it does
// not fit file_time_type. (st_mtim is POSIX; Darwin names it st_mtimespec.)
template <class Stat>
bool mtime_of(const Stat& __st, long long& out) noexcept {
  long long __sec, __nsec;
  if constexpr (requires { __st.st_mtim; }) {
    __sec = static_cast<long long>(__st.st_mtim.tv_sec);
    __nsec = static_cast<long long>(__st.st_mtim.tv_nsec);
  } else {
    __sec = static_cast<long long>(__st.st_mtimespec.tv_sec);
    __nsec = static_cast<long long>(__st.st_mtimespec.tv_nsec);
  }
  long long ns;
  if (__builtin_mul_overflow(__sec, 1'000'000'000LL, &ns) || __builtin_add_overflow(ns, __nsec, &ns))
    return false;
  out = ns;
  return true;
}

// Opens a directory stream on a descriptor; closes fd on failure.
DIR* open_dir_fd(int __fd) noexcept {
  DIR* d = ::fdopendir(__fd);
  if (d == nullptr) {
    int e = errno;
    ::close(__fd);
    errno = e;
  }
  return d;
}

} // namespace

// ---- [fs.class.path] ----
namespace [[__gnu__::__visibility__("hidden")]] std { namespace filesystem {

path& path::operator/=(const path& p) {
  if (p.is_absolute()) { // [fs.path.append]/2
    if (this != __builtin_addressof(p))
      __s_ = p.__s_;
    return *this;
  }
  if (this == __builtin_addressof(p)) {
    path copy(p);
    return *this /= copy;
  }
  if (has_filename()) // [fs.path.append]/3.1 (no root-names: the absolute case has a root-directory)
    __s_.push_back('/');
  __s_.append(p.__s_);
  return *this;
}

path& path::remove_filename() {
  __s_.erase(__s_.size() - filename_of(__s_).size());
  return *this;
}

path& path::replace_filename(const path& __replacement) {
  path r(__replacement); // replacement may be *this
  remove_filename();
  return *this /= r;
}

path& path::replace_extension(const path& __replacement) {
  path r(__replacement);
  sv __f = filename_of(__s_);
  __s_.erase(__s_.size() - (__f.size() - extension_start(__f)));
  if (!r.empty() && r.__s_[0] != '.')
    __s_.push_back('.');
  __s_.append(r.__s_);
  return *this;
}

int path::compare(const path& p) const noexcept { return compare_paths(__s_, p.__s_); }
int path::compare(basic_string_view<value_type> s) const { return compare_paths(__s_, s); }

path path::root_name() const { return path(); }
path path::root_directory() const { return has_root(__s_) ? path("/") : path(); }
path path::root_path() const { return root_directory(); }
path path::relative_path() const { return path(string_type(sv(__s_).substr(root_end(__s_)))); }

path path::parent_path() const {
  const size_t n = __s_.size();
  if (root_end(__s_) == n) // !has_relative_path()
    return *this;
  size_t last = prev_elem(__s_, n);
  if (__s_[last] == '/') // the trailing empty element: drop the separators
    return path(__s_.substr(0, __s_.find_last_not_of('/', last) + 1));
  size_t __j = __s_.find_last_not_of('/', last == 0 ? 0 : last - 1);
  if (last == 0 || __j == npos) // the root-directory (or nothing) precedes the last filename
    return path(__s_.substr(0, last));
  return path(__s_.substr(0, __j + 1));
}

path path::filename() const { return path(string_type(filename_of(__s_))); }

path path::stem() const {
  sv __f = filename_of(__s_);
  return path(string_type(__f.substr(0, extension_start(__f))));
}

path path::extension() const {
  sv __f = filename_of(__s_);
  return path(string_type(__f.substr(extension_start(__f))));
}

bool path::has_relative_path() const { return root_end(__s_) < __s_.size(); }
bool path::has_parent_path() const { return !parent_path().empty(); }
bool path::has_filename() const { return !filename_of(__s_).empty(); }
bool path::has_stem() const { return extension_start(filename_of(__s_)) != 0; }
bool path::has_extension() const {
  sv __f = filename_of(__s_);
  return extension_start(__f) != __f.size();
}

// [fs.path.generic]/6: the normal form, as a stack of filenames.
path path::lexically_normal() const {
  if (__s_.empty())
    return path();
  const sv s = __s_;
  const bool __root = has_root(s);
  std::vector<sv> __names;
  bool trailing = false; // the normal form ends in a separator
  for (size_t __pos = root_end(s); __pos < s.size(); __pos = next_elem(s, __pos)) {
    sv e = elem_at(s, __pos);
    if (e.empty() || e == ".") { // steps 3-4
      trailing = true;
    } else if (e == "..") {
      if (!__names.empty() && __names.back() != "..") { // step 5
        __names.pop_back();
        trailing = true;
      } else if (__root) { // step 6
        trailing = true;
      } else {
        __names.push_back(e);
        trailing = false;
      }
    } else {
      __names.push_back(e);
      trailing = false;
    }
  }
  string_type r = __root ? "/" : "";
  if (__names.empty())
    return path(__root ? r : string_type(".")); // step 8
  for (size_t i = 0; i < __names.size(); ++i) {
    if (i != 0)
      r.push_back('/');
    r.append(__names[i]);
  }
  if (trailing && __names.back() != "..") // step 7
    r.push_back('/');
  return path(static_cast<string_type&&>(r));
}

// [fs.path.gen]/3
path path::lexically_relative(const path& base) const {
  if (is_absolute() != base.is_absolute())
    return path();
  const sv a = __s_, b = base.__s_;
  size_t i = 0, __j = 0; // mismatch(begin(), end(), base.begin(), base.end())
  while (i < a.size() && __j < b.size() && elem_at(a, i) == elem_at(b, __j)) {
    i = next_elem(a, i);
    __j = next_elem(b, __j);
  }
  if (i == a.size() && __j == b.size())
    return path(".");
  long n = 0;
  for (; __j < b.size(); __j = next_elem(b, __j)) {
    sv e = elem_at(b, __j);
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
  iterator __it;
  __it.__p_ = this;
  __it.__pos_ = 0;
  __it.load();
  return __it;
}

path::iterator path::end() const {
  iterator __it;
  __it.__p_ = this;
  __it.__pos_ = __s_.size();
  return __it;
}

void path::iterator::load() { __elem_ = path(string_type(elem_at(__p_->__s_, __pos_))); }

path::iterator& path::iterator::operator++() {
  __pos_ = next_elem(__p_->__s_, __pos_);
  load();
  return *this;
}

path::iterator& path::iterator::operator--() {
  __pos_ = prev_elem(__p_->__s_, __pos_);
  load();
  return *this;
}

// Equal paths have equal elements ([fs.path.compare]), so the hash combines the elements.
size_t hash_value(const path& p) noexcept {
  const sv s = p.native();
  std::uint64_t h = has_root(s) ? 0x2f : 0;
  for (size_t __pos = root_end(s); __pos < s.size(); __pos = next_elem(s, __pos)) {
    sv e = elem_at(s, __pos);
    h = __ycxx::__detail::__mum(h ^ __ycxx::__detail::__hash_chars(e.data(), e.size()), __ycxx::__detail::__hash_k1);
  }
  return static_cast<size_t>(h);
}

// ---- [fs.class.filesystem.error] ----
namespace {
shared_ptr<const __ycxx::__detail::__fs_error_data> error_data(const char* base_what, const path* __p1, const path* __p2) {
  auto d = make_shared<__ycxx::__detail::__fs_error_data>();
  d->what = "filesystem error: ";
  d->what += base_what;
  if (__p1 != nullptr) {
    d->__p1 = *__p1;
    d->what += " [";
    d->what += __p1->native();
    d->what += "]";
  }
  if (__p2 != nullptr) {
    d->__p2 = *__p2;
    d->what += " [";
    d->what += __p2->native();
    d->what += "]";
  }
  return d;
}
} // namespace

filesystem_error::filesystem_error(const string& __what_arg, error_code ec)
    : system_error(ec, __what_arg), __data_(error_data(system_error::what(), nullptr, nullptr)) {}
filesystem_error::filesystem_error(const string& __what_arg, const path& __p1, error_code ec)
    : system_error(ec, __what_arg), __data_(error_data(system_error::what(), __builtin_addressof(__p1), nullptr)) {}
filesystem_error::filesystem_error(const string& __what_arg, const path& __p1, const path& __p2, error_code ec)
    : system_error(ec, __what_arg),
      __data_(error_data(system_error::what(), __builtin_addressof(__p1), __builtin_addressof(__p2))) {}
filesystem_error::~filesystem_error() = default;

}} // namespace std::filesystem

// [fs.path.construct]/6
std::string __ycxx::__detail::__fs_native_through_locale(const char* first, const char* last, const std::locale& __loc) {
  using cvt_t = std::codecvt<wchar_t, char, std::mbstate_t>;
  const cvt_t& cvt = std::use_facet<cvt_t>(__loc);
  std::wstring __wide;
  if (cvt.always_noconv()) {
    for (; first != last; ++first)
      __wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*first)));
  } else {
    std::mbstate_t state{};
    wchar_t __buf[64];
    while (first != last) {
      const char* __from_next = first;
      wchar_t* __to_next = __buf;
      auto __res = cvt.in(state, first, last, __from_next, __buf, __buf + 64, __to_next);
      __wide.append(__buf, __to_next);
      if (__res == std::codecvt_base::error || (__res == std::codecvt_base::partial && __from_next == first)) {
        // [fs.path.type.cvt]/3: an unrepresentable character converts to something unspecified
        __wide.push_back(L'�');
        __from_next = first + 1;
        state = std::mbstate_t{};
      } else if (__res == std::codecvt_base::noconv) {
        for (; first != last; ++first)
          __wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*first)));
        break;
      }
      first = __from_next;
    }
  }
  std::string out;
  ::__ycxx::__detail::__utf_append<wchar_t>(out, __wide.data(), __wide.data() + __wide.size());
  return out;
}

// ---- directory iteration ----
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __fs_dir_state {
  DIR* __dir = nullptr;
  __fs::path __root;
  __fs::directory_entry __entry;

  __fs_dir_state() = default;
  __fs_dir_state(const __fs_dir_state&) = delete;
  __fs_dir_state& operator=(const __fs_dir_state&) = delete;
  ~__fs_dir_state() {
    if (__dir != nullptr)
      ::closedir(__dir);
  }

  // Reads the next entry of d (not "." or "..") into e, as dir / name with the file type of the
  // listing cached. Returns 1, 0 at the end, -1 (errno set) on error.
  static int read_next(DIR* d, const __fs::path& __dir, __fs::directory_entry& e) {
    for (;;) {
      errno = 0;
      struct dirent* de = ::readdir(d);
      if (de == nullptr)
        return errno != 0 ? -1 : 0;
      const char* name = de->d_name;
      if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        continue;
      e.__path_ = __dir;
      e.__path_ /= __fs::path(name);
      e.__cache_ = __fs_attr_cache();
      __fs::file_type t = __fs::file_type::none;
      switch (de->d_type) {
      case DT_REG:
        t = __fs::file_type::regular;
        break;
      case DT_DIR:
        t = __fs::file_type::directory;
        break;
      case DT_LNK:
        t = __fs::file_type::symlink;
        break;
      case DT_BLK:
        t = __fs::file_type::block;
        break;
      case DT_CHR:
        t = __fs::file_type::character;
        break;
      case DT_FIFO:
        t = __fs::file_type::fifo;
        break;
      case DT_SOCK:
        t = __fs::file_type::socket;
        break;
      default:
        break;
      }
      if (t != __fs::file_type::none) {
        e.__cache_.__level = 1;
        e.__cache_.__sym_type = t;
        if (t != __fs::file_type::symlink)
          e.__cache_.type = t;
      }
      return 1;
    }
  }
};

struct __fs_rec_state {
  struct __level {
    DIR* __dir;
    __fs::path path;
  };
  std::vector<__level> stack;
  __fs::directory_options options = __fs::directory_options::none;
  __fs::directory_entry __entry;

  __fs_rec_state() = default;
  __fs_rec_state(const __fs_rec_state&) = delete;
  __fs_rec_state& operator=(const __fs_rec_state&) = delete;
  ~__fs_rec_state() {
    for (__level& __l : stack)
      ::closedir(__l.__dir);
  }

  bool has(__fs::directory_options __o) const noexcept { return (options & __o) != __fs::directory_options::none; }

  // Whether the current entry is a directory to descend into ([fs.rec.dir.itr.members]/21.2),
  // and whether it is reached through a symbolic link. When its type cannot be determined
  // (file_type::none), the attempt to open it reports the error (or skips a permission error
  // on request), as the evaluation of status() in /21.2 would.
  bool descend(bool& through_link) const noexcept {
    error_code ec;
    __fs::file_type lt = __entry.type_of(true, ec);
    through_link = lt == __fs::file_type::symlink;
    if (lt == __fs::file_type::directory || lt == __fs::file_type::none)
      return true;
    if (!through_link || !has(__fs::directory_options::follow_directory_symlink))
      return false;
    __fs::file_type t = __entry.type_of(false, ec);
    return t == __fs::file_type::directory || t == __fs::file_type::none;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace filesystem {

void directory_iterator::open(const path& p, directory_options options, error_code& ec) {
  ec.clear();
  __state_.reset();
  auto __st = make_shared<__ycxx::__detail::__fs_dir_state>();
  __st->__root = p;
  __st->__dir = ::opendir(p.c_str());
  if (__st->__dir == nullptr) {
    int e = errno;
    if (e == EACCES && (options & directory_options::skip_permission_denied) != directory_options::none)
      return;
    ec = errno_code(e);
    return;
  }
  int r = __ycxx::__detail::__fs_dir_state::read_next(__st->__dir, __st->__root, __st->__entry);
  if (r < 0)
    ec = last_error();
  else if (r > 0)
    __state_ = static_cast<shared_ptr<__ycxx::__detail::__fs_dir_state>&&>(__st);
}

void directory_iterator::advance(error_code& ec, path* where) {
  ec.clear();
  if (__state_ == nullptr)
    return;
  int r = __ycxx::__detail::__fs_dir_state::read_next(__state_->__dir, __state_->__root, __state_->__entry);
  if (r < 0) {
    ec = last_error();
    if (where != nullptr)
      *where = __state_->__root;
  }
  if (r <= 0)
    __state_.reset();
}

const directory_entry& directory_iterator::operator*() const {
  __ycxx::__detail::__precondition(__state_ != nullptr, "directory_iterator: the end iterator is not dereferenceable");
  return __state_->__entry;
}

void recursive_directory_iterator::open(const path& p, directory_options options, error_code& ec) {
  ec.clear();
  __state_.reset();
  __pending_ = true;
  auto __st = make_shared<__ycxx::__detail::__fs_rec_state>();
  __st->options = options;
  __st->stack.reserve(8);
  DIR* d = ::opendir(p.c_str());
  if (d == nullptr) {
    int e = errno;
    if (e == EACCES && __st->has(directory_options::skip_permission_denied))
      return;
    ec = errno_code(e);
    return;
  }
  try {
    __st->stack.push_back({d, p}); // copies p: the descriptor is the stack's once it is pushed
  } catch (...) {
    ::closedir(d);
    throw;
  }
  int r = __ycxx::__detail::__fs_dir_state::read_next(d, __st->stack.back().path, __st->__entry);
  if (r < 0)
    ec = last_error();
  else if (r > 0)
    __state_ = static_cast<shared_ptr<__ycxx::__detail::__fs_rec_state>&&>(__st);
}

void recursive_directory_iterator::advance(error_code& ec, path* where) {
  ec.clear();
  if (__state_ == nullptr)
    return;
  __ycxx::__detail::__fs_rec_state& __st = *__state_;
  if (__pending_) {
    bool through_link = false;
    if (__st.descend(through_link)) {
      // A directory entry that is not a symbolic link is opened relative to its parent's
      // descriptor without following links, so a directory swapped for a link is never entered.
      // A symbolic link followed on request is opened by its whole path: /21.2 recurses into
      // (*this)->path() once is_directory((*this)->status()) holds, so its resolution, and its
      // limit on symbolic links, is that path's. A loop (d/self -> .) then ends with ELOOP,
      // which status() reports as an error (file_type::none, [fs.op.status]/6.1.3).
      int __fd = through_link
                 ? ::open(__st.__entry.path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)
                 : ::openat(::dirfd(__st.stack.back().__dir), __st.__entry.path().filename().c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
      DIR* d = __fd < 0 ? nullptr : open_dir_fd(__fd);
      if (d != nullptr) {
        try {
          __st.stack.push_back({d, __st.__entry.path()});
        } catch (...) {
          ::closedir(d);
          throw;
        }
      } else {
        int e = errno;
        // A directory that is gone or was replaced by a non-directory since it was listed is
        // simply not entered; a permission error may be skipped on request.
        bool __skip = e == ENOENT || e == ENOTDIR || (e == ELOOP && !through_link) ||
                    (e == EACCES && __st.has(directory_options::skip_permission_denied));
        if (!__skip) {
          ec = errno_code(e);
          if (where != nullptr)
            *where = __st.__entry.path();
          __state_.reset();
          return;
        }
      }
    }
  }
  __pending_ = true;
  for (;;) {
    int r = __ycxx::__detail::__fs_dir_state::read_next(__st.stack.back().__dir, __st.stack.back().path, __st.__entry);
    if (r > 0)
      return;
    if (r < 0) {
      ec = last_error();
      if (where != nullptr)
        *where = __st.stack.back().path;
      __state_.reset();
      return;
    }
    ::closedir(__st.stack.back().__dir);
    __st.stack.pop_back();
    if (__st.stack.empty()) {
      __state_.reset();
      return;
    }
  }
}

void recursive_directory_iterator::pop(error_code& ec, path* where) {
  ec.clear();
  __ycxx::__detail::__precondition(__state_ != nullptr, "recursive_directory_iterator::pop: not dereferenceable");
  __ycxx::__detail::__fs_rec_state& __st = *__state_;
  if (__st.stack.size() == 1) {
    __state_.reset();
    return;
  }
  ::closedir(__st.stack.back().__dir);
  __st.stack.pop_back();
  // Continue in the parent without entering the directory just left. pop() is not an increment:
  // recursion_pending() keeps its value.
  const bool pending = __pending_;
  __pending_ = false;
  advance(ec, where);
  __pending_ = pending;
}

directory_options recursive_directory_iterator::options() const {
  __ycxx::__detail::__precondition(__state_ != nullptr, "recursive_directory_iterator::options: not dereferenceable");
  return __state_->options;
}
int recursive_directory_iterator::depth() const {
  __ycxx::__detail::__precondition(__state_ != nullptr, "recursive_directory_iterator::depth: not dereferenceable");
  return static_cast<int>(__state_->stack.size()) - 1;
}
bool recursive_directory_iterator::recursion_pending() const {
  __ycxx::__detail::__precondition(__state_ != nullptr, "recursive_directory_iterator::recursion_pending: not dereferenceable");
  return __pending_;
}
void recursive_directory_iterator::disable_recursion_pending() {
  __ycxx::__detail::__precondition(__state_ != nullptr,
                             "recursive_directory_iterator::disable_recursion_pending: not dereferenceable");
  __pending_ = false;
}
const directory_entry& recursive_directory_iterator::operator*() const {
  __ycxx::__detail::__precondition(__state_ != nullptr, "recursive_directory_iterator: the end iterator is not dereferenceable");
  return __state_->__entry;
}

// ---- [fs.class.directory.entry] ----
void directory_entry::refresh(error_code& ec) noexcept {
  __cache_ = __ycxx::__detail::__fs_attr_cache();
  __cache_.__level = 2;
  struct stat __st;
  if (::lstat(__path_.c_str(), &__st) != 0) {
    int e = errno;
    __cache_.__sym_err = __cache_.__stat_err = e;
    __cache_.__sym_type = __cache_.type = status_for_error(e).type();
    ec = errno_code(e); // the throwing refresh() ignores a missing file
    return;
  }
  __cache_.__sym_type = type_of_mode(__st.st_mode);
  __cache_.__sym_perms = perms_of_mode(__st.st_mode);
  if (S_ISLNK(__st.st_mode) && ::stat(__path_.c_str(), &__st) != 0) {
    int e = errno;
    __cache_.__stat_err = e;
    __cache_.type = status_for_error(e).type();
    ec.clear();
    return;
  }
  __cache_.type = type_of_mode(__st.st_mode);
  __cache_.perms = perms_of_mode(__st.st_mode);
  __cache_.size = static_cast<uintmax_t>(__st.st_size);
  __cache_.__nlink = static_cast<uintmax_t>(__st.st_nlink);
  if (!mtime_of(__st, __cache_.__mtime))
    __cache_.__stat_err = EOVERFLOW; // reported by last_write_time; status is still cached
  ec.clear();
}

file_status directory_entry::status(error_code& ec) const noexcept {
  if (__cache_.__level == 2 && (__cache_.__stat_err == 0 || __cache_.__stat_err == EOVERFLOW)) {
    ec.clear();
    return file_status(__cache_.type, __cache_.perms);
  }
  if (__cache_.__level == 2) {
    ec = errno_code(__cache_.__stat_err);
    return file_status(__cache_.type);
  }
  return filesystem::status(__path_, ec);
}

file_status directory_entry::symlink_status(error_code& ec) const noexcept {
  if (__cache_.__level == 2) {
    if (__cache_.__sym_err != 0) {
      ec = errno_code(__cache_.__sym_err);
      return file_status(__cache_.__sym_type);
    }
    ec.clear();
    return file_status(__cache_.__sym_type, __cache_.__sym_perms);
  }
  return filesystem::symlink_status(__path_, ec);
}

file_type directory_entry::type_of(bool link, error_code& ec) const noexcept {
  if (__cache_.__level == 1 && (link || __cache_.__sym_type != file_type::symlink)) {
    ec.clear();
    return __cache_.__sym_type;
  }
  return (link ? symlink_status(ec) : status(ec)).type();
}

uintmax_t directory_entry::file_size(error_code& ec) const noexcept {
  if (__cache_.__level == 2 && (__cache_.__stat_err == 0 || __cache_.__stat_err == EOVERFLOW) &&
      __cache_.type == file_type::regular) {
    ec.clear();
    return __cache_.size;
  }
  return filesystem::file_size(__path_, ec);
}

uintmax_t directory_entry::hard_link_count(error_code& ec) const noexcept {
  if (__cache_.__level == 2 && (__cache_.__stat_err == 0 || __cache_.__stat_err == EOVERFLOW)) {
    ec.clear();
    return __cache_.__nlink;
  }
  return filesystem::hard_link_count(__path_, ec);
}

file_time_type directory_entry::last_write_time(error_code& ec) const noexcept {
  if (__cache_.__level == 2 && __cache_.__stat_err == 0) {
    ec.clear();
    return file_time_type(file_time_type::duration(__cache_.__mtime));
  }
  return filesystem::last_write_time(__path_, ec);
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
  struct __guard {
    char* p;
    ~__guard() { ::free(p); }
  } __g{r};
  return path(r);
}

namespace {
constexpr copy_options in_recursive_copy = static_cast<copy_options>(512);
bool has_opt(copy_options __o) noexcept { return __o != copy_options::none; }
} // namespace

// [fs.op.copy]
void copy(const path& from, const path& to, copy_options options, error_code& ec) {
  ec.clear();
  const bool create_or_skip_links = has_opt(options & (copy_options::create_symlinks | copy_options::skip_symlinks));
  const bool copy_links = has_opt(options & copy_options::copy_symlinks);
  file_status __f = create_or_skip_links || copy_links ? symlink_status(from, ec) : status(from, ec);
  if (!exists(__f)) { // (4.5.1)
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
  if (is_other(__f) || is_other(t)) { // (4.5.3)
    ec = errno_code(ENOTSUP);
    return;
  }
  if (is_directory(__f) && is_regular_file(t)) { // (4.5.4)
    ec = errno_code(EISDIR);
    return;
  }
  if (is_symlink(__f)) { // (4.6)
    if (has_opt(options & copy_options::skip_symlinks))
      return;
    if (!exists(t) && copy_links)
      copy_symlink(from, to, ec);
    else // (4.6.3)
      ec = errno_code(exists(t) ? EEXIST : ENOTSUP);
    return;
  }
  if (is_regular_file(__f)) { // (4.7)
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
  if (is_directory(__f) && has_opt(options & copy_options::create_symlinks)) { // (4.8)
    ec = errno_code(EISDIR);
    return;
  }
  if (is_directory(__f) && (has_opt(options & copy_options::recursive) || options == copy_options::none)) { // (4.9)
    if (!exists(t)) {
      create_directory(to, from, ec);
      if (ec)
        return;
    }
    directory_iterator __it(from, ec);
    for (; !ec && __it != directory_iterator(); __it.increment(ec)) {
      const path& __x = __it->path();
      copy(__x, to / __x.filename(), options | in_recursive_copy, ec);
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
      long long __ft = 0, tt = 0; // update_existing: copy only if from is more recent
      if (!mtime_of(fst, __ft) || !mtime_of(tst, tt)) {
        ec = errno_code(EOVERFLOW);
        return false;
      }
      if (__ft <= tt)
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
  const mode_t __mode = ist.st_mode & 07777;
  int out = ::open(to.c_str(), O_WRONLY | O_CLOEXEC | O_NOCTTY | (to_exists ? O_TRUNC : O_CREAT | O_EXCL), __mode);
  if (out < 0) {
    ec = last_error();
    ::close(in);
    return false;
  }
  char __buf[32768];
  int __err = 0;
  for (;;) {
    ssize_t n = ::read(in, __buf, sizeof __buf);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      __err = errno;
      break;
    }
    if (n == 0)
      break;
    for (ssize_t done = 0; done < n;) {
      ssize_t __w = ::write(out, __buf + done, static_cast<size_t>(n - done));
      if (__w < 0) {
        if (errno == EINTR)
          continue;
        __err = errno;
        break;
      }
      done += __w;
    }
    if (__err != 0)
      break;
  }
  if (__err == 0 && ::fchmod(out, __mode) != 0) // the attributes of from (open's mode is umasked)
    __err = errno;
  ::close(in);
  if (::close(out) != 0 && __err == 0)
    __err = errno;
  if (__err != 0) {
    ec = errno_code(__err);
    return false;
  }
  return true;
}

void copy_symlink(const path& __existing_symlink, const path& __new_symlink, error_code& ec) noexcept {
  // The link's target is read into a path, which allocates: this overload is noexcept, so an
  // allocation failure is reported through ec ([fs.err.report]/3).
  try {
    path target = read_symlink(__existing_symlink, ec);
    if (ec)
      return;
    create_symlink(target, __new_symlink, ec); // POSIX makes no difference for directory links
  } catch (const std::bad_alloc&) {
    ec = errno_code(ENOMEM);
  }
}

// [fs.op.create.directories]
bool create_directories(const path& p, error_code& ec) {
  ec.clear();
  error_code __sec;
  file_status __st = status(p, __sec);
  if (exists(__st)) {
    if (!is_directory(__st))
      ec = errno_code(EEXIST);
    return false;
  }
  if (__st.type() == file_type::none) {
    ec = __sec;
    return false;
  }
  // The elements that do not exist, innermost first. A trailing "." or empty filename names the
  // same directory as its parent, so it gets no mkdir of its own.
  std::vector<path> __missing;
  path cur = p;
  for (;;) {
    path __f = cur.filename();
    if (!__f.empty() && __f.native() != ".")
      __missing.push_back(cur);
    path __parent = cur.parent_path();
    if (__parent.empty() || __parent == cur)
      break;
    __st = status(__parent, __sec);
    if (exists(__st)) {
      if (!is_directory(__st)) {
        ec = errno_code(ENOTDIR);
        return false;
      }
      break;
    }
    if (__st.type() == file_type::none) {
      ec = __sec;
      return false;
    }
    cur = static_cast<path&&>(__parent);
  }
  if (__missing.empty()) { // p names no directory to create (the empty path)
    ec = errno_code(ENOENT);
    return false;
  }
  bool created = false;
  for (size_t i = __missing.size(); i-- > 0;) {
    created = create_directory(__missing[i], ec);
    if (ec)
      return false;
  }
  return created;
}

namespace {
// mkdir with [fs.op.create.directory]'s rule: an existing directory is not an error.
bool make_directory(const path& p, mode_t __mode, error_code& ec) noexcept {
  if (::mkdir(p.c_str(), __mode) == 0) {
    ec.clear();
    return true;
  }
  int e = errno;
  struct stat __st;
  if (e == EEXIST && ::stat(p.c_str(), &__st) == 0 && S_ISDIR(__st.st_mode)) {
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
  struct stat __st;
  if (::stat(existing_p.c_str(), &__st) != 0) {
    ec = errno == ENOENT ? errno_code(ENOTDIR) : last_error(); // existing_p is not a directory
    return false;
  }
  if (!S_ISDIR(__st.st_mode)) {
    ec = errno_code(ENOTDIR);
    return false;
  }
  return make_directory(p, __st.st_mode & 07777, ec);
}

void create_directory_symlink(const path& to, const path& __new_symlink, error_code& ec) noexcept {
  create_symlink(to, __new_symlink, ec);
}

void create_hard_link(const path& to, const path& __new_hard_link, error_code& ec) noexcept {
  if (::link(to.c_str(), __new_hard_link.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

void create_symlink(const path& to, const path& __new_symlink, error_code& ec) noexcept {
  if (::symlink(to.c_str(), __new_symlink.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

path current_path(error_code& ec) {
  ec.clear();
  std::string __buf(256, '\0');
  for (;;) {
    if (::getcwd(__buf.data(), __buf.size()) != nullptr) {
      __buf.resize(std::char_traits<char>::length(__buf.data()));
      return path(static_cast<std::string&&>(__buf));
    }
    if (errno != ERANGE) {
      ec = last_error();
      return path();
    }
    __buf.resize(__buf.size() * 2);
  }
}

void current_path(const path& p, error_code& ec) noexcept {
  if (::chdir(p.c_str()) != 0)
    ec = last_error();
  else
    ec.clear();
}

bool equivalent(const path& __p1, const path& __p2, error_code& ec) noexcept {
  struct stat __s1, __s2;
  if (::stat(__p1.c_str(), &__s1) != 0 || ::stat(__p2.c_str(), &__s2) != 0) {
    ec = last_error();
    return false;
  }
  ec.clear();
  return __s1.st_dev == __s2.st_dev && __s1.st_ino == __s2.st_ino;
}

uintmax_t file_size(const path& p, error_code& ec) noexcept {
  struct stat __st;
  if (::stat(p.c_str(), &__st) != 0) {
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  if (!S_ISREG(__st.st_mode)) { // implementation-defined ([fs.op.file.size]/2.2): an error
    ec = errno_code(S_ISDIR(__st.st_mode) ? EISDIR : ENOTSUP);
    return static_cast<uintmax_t>(-1);
  }
  ec.clear();
  return static_cast<uintmax_t>(__st.st_size);
}

uintmax_t hard_link_count(const path& p, error_code& ec) noexcept {
  struct stat __st;
  if (::stat(p.c_str(), &__st) != 0) {
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  ec.clear();
  return static_cast<uintmax_t>(__st.st_nlink);
}

bool is_empty(const path& p, error_code& ec) {
  file_status s = status(p, ec);
  if (ec)
    return false;
  if (is_directory(s)) {
    directory_iterator __it(p, ec);
    if (ec)
      return false;
    return __it == directory_iterator();
  }
  uintmax_t __sz = file_size(p, ec);
  if (ec)
    return false;
  return __sz == 0;
}

file_time_type last_write_time(const path& p, error_code& ec) noexcept {
  struct stat __st;
  long long ns = 0;
  if (::stat(p.c_str(), &__st) != 0) {
    ec = last_error();
    return file_time_type::min();
  }
  if (!mtime_of(__st, ns)) {
    ec = errno_code(EOVERFLOW);
    return file_time_type::min();
  }
  ec.clear();
  return file_time_type(file_time_type::duration(ns));
}

void last_write_time(const path& p, file_time_type __new_time, error_code& ec) noexcept {
  long long ns = __new_time.time_since_epoch().count();
  long long __sec = ns / 1'000'000'000, rem = ns % 1'000'000'000;
  if (rem < 0) { // timespec wants 0 <= tv_nsec < 10^9
    rem += 1'000'000'000;
    --__sec;
  }
  struct timespec __ts[2];
  __ts[0].tv_sec = 0;
  __ts[0].tv_nsec = UTIME_OMIT; // leave the access time alone
  __ts[1].tv_sec = static_cast<time_t>(__sec);
  __ts[1].tv_nsec = static_cast<long>(rem);
  if (::utimensat(AT_FDCWD, p.c_str(), __ts, 0) != 0)
    ec = last_error();
  else
    ec.clear();
}

// [fs.op.permissions]
void permissions(const path& p, perms __prms, perm_options __opts, error_code& ec) {
  ec.clear();
  const bool add = (__opts & perm_options::add) != perm_options{};
  const bool rem = (__opts & perm_options::remove) != perm_options{};
  const bool rep = (__opts & perm_options::replace) != perm_options{};
  const bool nofollow = (__opts & perm_options::nofollow) != perm_options{};
  if (int(add) + int(rem) + int(rep) != 1) {
    ec = errno_code(EINVAL);
    return;
  }
  __prms &= perms::mask;
  file_status __st;
  if (add || rem || nofollow) {
    __st = nofollow ? symlink_status(p, ec) : status(p, ec);
    if (ec)
      return;
    if (add)
      __prms = __st.permissions() | __prms;
    else if (rem)
      __prms = __st.permissions() & ~__prms;
  }
  int flags = nofollow && is_symlink(__st) ? AT_SYMLINK_NOFOLLOW : 0;
  if (::fchmodat(AT_FDCWD, p.c_str(), static_cast<mode_t>(__prms & perms::mask), flags) != 0)
    ec = last_error();
}

path read_symlink(const path& p, error_code& ec) {
  ec.clear();
  std::string __buf(256, '\0');
  for (;;) {
    ssize_t n = ::readlink(p.c_str(), __buf.data(), __buf.size());
    if (n < 0) {
      ec = last_error();
      return path();
    }
    if (static_cast<size_t>(n) < __buf.size()) {
      __buf.resize(static_cast<size_t>(n));
      return path(static_cast<std::string&&>(__buf));
    }
    __buf.resize(__buf.size() * 2); // possibly truncated
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
  int __err = 0;
  for (bool found = true; found && __err == 0;) {
    found = false;
    ::rewinddir(d);
    for (;;) {
      errno = 0;
      struct dirent* de = ::readdir(d);
      if (de == nullptr) {
        __err = errno;
        break;
      }
      const char* name = de->d_name;
      if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        continue;
      found = true;
      bool __dir = de->d_type == DT_DIR;
      if (de->d_type == DT_UNKNOWN) {
        struct stat __st;
        if (::fstatat(dfd, name, &__st, AT_SYMLINK_NOFOLLOW) != 0) {
          if (errno == ENOENT)
            continue;
          __err = errno;
          break;
        }
        __dir = S_ISDIR(__st.st_mode);
      }
      if (__dir) {
        int __sub = ::openat(dfd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (__sub < 0) {
          if (errno == ENOENT)
            continue;
          if (errno != ENOTDIR && errno != ELOOP) {
            __err = errno;
            break;
          }
          __dir = false; // replaced by a non-directory since it was listed
        } else {
          __err = remove_contents(__sub, n);
          if (__err != 0)
            break;
        }
      }
      if (::unlinkat(dfd, name, __dir ? AT_REMOVEDIR : 0) != 0) {
        if (errno == ENOENT)
          continue;
        __err = errno;
        break;
      }
      ++n;
    }
  }
  ::closedir(d);
  return __err;
}
} // namespace

// [fs.op.remove.all]
uintmax_t remove_all(const path& p, error_code& ec) {
  ec.clear();
  struct stat __st;
  if (::lstat(p.c_str(), &__st) != 0) {
    if (errno == ENOENT || errno == ENOTDIR)
      return 0;
    ec = last_error();
    return static_cast<uintmax_t>(-1);
  }
  uintmax_t n = 0;
  if (S_ISDIR(__st.st_mode)) {
    int __fd = ::open(p.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (__fd >= 0) {
      int __err = remove_contents(__fd, n);
      if (__err != 0) {
        ec = errno_code(__err);
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
  const uintmax_t __unit = sv.f_frsize;
  return space_info{static_cast<uintmax_t>(sv.f_blocks) * __unit, static_cast<uintmax_t>(sv.f_bfree) * __unit,
                    static_cast<uintmax_t>(sv.f_bavail) * __unit};
}

file_status status(const path& p, error_code& ec) noexcept {
  struct stat __st;
  int r = ::stat(p.c_str(), &__st);
  return status_from(r, __st, ec);
}

file_status symlink_status(const path& p, error_code& ec) noexcept {
  struct stat __st;
  int r = ::lstat(p.c_str(), &__st);
  return status_from(r, __st, ec);
}

// [fs.op.temp.dir.path]: TMPDIR, TMP, TEMP, TEMPDIR, else /tmp.
path temp_directory_path(error_code& ec) {
  const char* __dir = nullptr;
  for (const char* var : {"TMPDIR", "TMP", "TEMP", "TEMPDIR"}) {
    __dir = ::getenv(var);
    if (__dir != nullptr && *__dir != '\0')
      break;
    __dir = nullptr;
  }
  path p(__dir != nullptr ? __dir : "/tmp");
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
  path __head;
  path::iterator split = p.begin();
  for (path::iterator __it = p.begin(); __it != p.end(); ++__it) {
    path next = __head;
    next /= *__it;
    s = status(next, ec);
    if (s.type() == file_type::none)
      return path();
    if (!exists(s))
      break;
    __head = static_cast<path&&>(next);
    split = __it;
    ++split;
  }
  ec.clear();
  path r;
  if (!__head.empty()) {
    r = canonical(__head, ec);
    if (ec)
      return path();
  }
  for (; split != p.end(); ++split)
    r /= *split;
  return r.lexically_normal();
}

}} // namespace std::filesystem
