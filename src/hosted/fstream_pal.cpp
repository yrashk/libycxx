// libycxx hosted runtime, YCXX_PAL=none with the 'files' hosted layer (DECISIONS §18): the file
// operations of basic_filebuf ([filebuf.members]), declared in ycxx/hosted/fstream.hpp, over the
// integrator's file primitives (ycxx_pal_file_*), instead of C stdio (src/hosted/fstream.cpp).
// The filebuf does its own buffering; offsets are bytes. A file is the provider's handle, which
// is nonzero (ycxx/pal.h).
#include <ios>
#include <ycxx/pal.h>

namespace {

// [filebuf.members] Table 146: the primitive's flags for `__mode & ~ate`, or 0 if the combination
// is not in the table (binary changes nothing: there is no newline translation).
int file_flags(std::ios_base::openmode __mode) noexcept {
  using std::ios_base;
  constexpr int r = ycxx_pal_file_read_access, __w = ycxx_pal_file_write_access, a = ycxx_pal_file_append,
                c = ycxx_pal_file_create, t = ycxx_pal_file_truncate, __x = ycxx_pal_file_exclusive;
  switch (__mode & ~(ios_base::ate | ios_base::binary)) {
  case ios_base::out:
  case ios_base::out | ios_base::trunc:
    return __w | c | t; // "w"
  case ios_base::out | ios_base::noreplace:
  case ios_base::out | ios_base::trunc | ios_base::noreplace:
    return __w | c | t | __x; // "wx"
  case ios_base::out | ios_base::app:
  case ios_base::app:
    return __w | a | c; // "a"
  case ios_base::in:
    return r; // "r"
  case ios_base::in | ios_base::out:
    return r | __w; // "r+"
  case ios_base::in | ios_base::out | ios_base::trunc:
    return r | __w | c | t; // "w+"
  case ios_base::in | ios_base::out | ios_base::trunc | ios_base::noreplace:
    return r | __w | c | t | __x; // "w+x"
  case ios_base::in | ios_base::out | ios_base::app:
  case ios_base::in | ios_base::app:
    return r | __w | a | c; // "a+"
  default:
    return 0;
  }
}

ycxx_pal_handle handle(void* __f) noexcept { return reinterpret_cast<ycxx_pal_handle>(__f); }

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

void* __file_open(const char* name, std::ios_base::openmode __mode) noexcept {
  const int flags = file_flags(__mode);
  if (flags == 0)
    return nullptr;
  ycxx_pal_handle h = 0;
  if (::ycxx_pal_file_open(name, flags, &h) != 0 || h == 0)
    return nullptr;
  // [filebuf.members]/4-5: ate positions the file at its end; a failure closes it again.
  if (__mode & std::ios_base::ate) {
    ycxx_pal_i64 __pos = 0;
    if (::ycxx_pal_file_seek(h, 0, 2, &__pos) != 0) {
      ::ycxx_pal_file_close(h);
      return nullptr;
    }
  }
  return reinterpret_cast<void*>(h);
}

bool __file_close(void* __f) noexcept { return ::ycxx_pal_file_close(handle(__f)) == 0; }

std::size_t __file_read(void* __f, char* __buf, std::size_t n) noexcept {
  std::size_t done = 0;
  while (done < n) {
    ycxx_pal_size __got = 0;
    if (::ycxx_pal_file_read(handle(__f), __buf + done, n - done, &__got) != 0 || __got == 0)
      break;
    done += __got;
  }
  return done;
}

bool __file_write(void* __f, const char* __buf, std::size_t n) noexcept {
  while (n != 0) {
    ycxx_pal_size __w = 0;
    if (::ycxx_pal_file_write(handle(__f), __buf, n, &__w) != 0 || __w == 0)
      return false;
    __buf += __w;
    n -= __w;
  }
  return true;
}

long long __file_seek(void* __f, long long __off, int __whence) noexcept {
  ycxx_pal_i64 __pos = 0;
  if (::ycxx_pal_file_seek(handle(__f), __off, __whence, &__pos) != 0)
    return -1;
  return static_cast<long long>(__pos);
}

bool __file_flush(void* __f) noexcept { return ::ycxx_pal_file_flush(handle(__f)) == 0; }

// basic_filebuf::native_handle(): the provider's handle (an int, as the POSIX descriptor is).
int __file_native(void* __f) noexcept { return static_cast<int>(handle(__f)); }

}} // namespace __ycxx::__detail
