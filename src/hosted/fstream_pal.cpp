// libycxx hosted runtime, YCXX_PAL=none with the 'files' hosted layer (DECISIONS §18): the file
// operations of basic_filebuf ([filebuf.members]), declared in ycxx/hosted/fstream.hpp, over the
// integrator's file primitives (ycxx_pal_file_*), instead of C stdio (src/hosted/fstream.cpp).
// The filebuf does its own buffering; offsets are bytes. A file is the provider's handle, which
// is nonzero (ycxx/pal.h).
#include <ios>
#include <ycxx/pal.h>

namespace {

// [filebuf.members] Table 146: the primitive's flags for `mode & ~ate`, or 0 if the combination
// is not in the table (binary changes nothing: there is no newline translation).
int file_flags(std::ios_base::openmode mode) noexcept {
  using std::ios_base;
  constexpr int r = ycxx_pal_file_read_access, w = ycxx_pal_file_write_access, a = ycxx_pal_file_append,
                c = ycxx_pal_file_create, t = ycxx_pal_file_truncate, x = ycxx_pal_file_exclusive;
  switch (mode & ~(ios_base::ate | ios_base::binary)) {
  case ios_base::out:
  case ios_base::out | ios_base::trunc:
    return w | c | t; // "w"
  case ios_base::out | ios_base::noreplace:
  case ios_base::out | ios_base::trunc | ios_base::noreplace:
    return w | c | t | x; // "wx"
  case ios_base::out | ios_base::app:
  case ios_base::app:
    return w | a | c; // "a"
  case ios_base::in:
    return r; // "r"
  case ios_base::in | ios_base::out:
    return r | w; // "r+"
  case ios_base::in | ios_base::out | ios_base::trunc:
    return r | w | c | t; // "w+"
  case ios_base::in | ios_base::out | ios_base::trunc | ios_base::noreplace:
    return r | w | c | t | x; // "w+x"
  case ios_base::in | ios_base::out | ios_base::app:
  case ios_base::in | ios_base::app:
    return r | w | a | c; // "a+"
  default:
    return 0;
  }
}

ycxx_pal_handle handle(void* f) noexcept { return reinterpret_cast<ycxx_pal_handle>(f); }

} // namespace

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

void* file_open(const char* name, std::ios_base::openmode mode) noexcept {
  const int flags = file_flags(mode);
  if (flags == 0)
    return nullptr;
  ycxx_pal_handle h = 0;
  if (::ycxx_pal_file_open(name, flags, &h) != 0 || h == 0)
    return nullptr;
  // [filebuf.members]/4-5: ate positions the file at its end; a failure closes it again.
  if (mode & std::ios_base::ate) {
    ycxx_pal_i64 pos = 0;
    if (::ycxx_pal_file_seek(h, 0, 2, &pos) != 0) {
      ::ycxx_pal_file_close(h);
      return nullptr;
    }
  }
  return reinterpret_cast<void*>(h);
}

bool file_close(void* f) noexcept { return ::ycxx_pal_file_close(handle(f)) == 0; }

std::size_t file_read(void* f, char* buf, std::size_t n) noexcept {
  std::size_t done = 0;
  while (done < n) {
    ycxx_pal_size got = 0;
    if (::ycxx_pal_file_read(handle(f), buf + done, n - done, &got) != 0 || got == 0)
      break;
    done += got;
  }
  return done;
}

bool file_write(void* f, const char* buf, std::size_t n) noexcept {
  while (n != 0) {
    ycxx_pal_size w = 0;
    if (::ycxx_pal_file_write(handle(f), buf, n, &w) != 0 || w == 0)
      return false;
    buf += w;
    n -= w;
  }
  return true;
}

long long file_seek(void* f, long long off, int whence) noexcept {
  ycxx_pal_i64 pos = 0;
  if (::ycxx_pal_file_seek(handle(f), off, whence, &pos) != 0)
    return -1;
  return static_cast<long long>(pos);
}

bool file_flush(void* f) noexcept { return ::ycxx_pal_file_flush(handle(f)) == 0; }

// basic_filebuf::native_handle(): the provider's handle (an int, as the POSIX descriptor is).
int file_native(void* f) noexcept { return static_cast<int>(handle(f)); }

}} // namespace ycxx::detail
