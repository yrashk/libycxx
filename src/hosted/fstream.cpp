// libycxx hosted runtime: the file operations of basic_filebuf ([filebuf.members]), declared in
// ycxx/hosted/fstream.hpp. A file is a C stdio FILE with stdio's own buffering turned off (the
// filebuf buffers); offsets are bytes. fopen maps the open modes (Table 146) and fclose closes;
// reads and seeks go to the FILE's descriptor, since fread on an unbuffered FILE waits until the
// whole request is filled or the file ends, and a read from a pipe, terminal or socket must return
// what is available ([filebuf.virtuals]/14: underflow gets "some" characters). stdio holds no data
// of its own for the FILE, so the descriptor's position is the file's.
#include <ios>
#include <cstdio>
#include <errno.h>
#include <unistd.h>

namespace {

// [filebuf.members] Table 146: the fopen mode for `__mode & ~ate`, or null if the combination is
// not in the table.
const char* stdio_mode(std::ios_base::openmode __mode) noexcept {
  using std::ios_base;
  const bool binary = (__mode & ios_base::binary) != 0;
  switch (__mode & ~(ios_base::ate | ios_base::binary)) {
  case ios_base::out:
  case ios_base::out | ios_base::trunc:
    return binary ? "wb" : "w";
  case ios_base::out | ios_base::noreplace:
  case ios_base::out | ios_base::trunc | ios_base::noreplace:
    return binary ? "wbx" : "wx";
  case ios_base::out | ios_base::app:
  case ios_base::app:
    return binary ? "ab" : "a";
  case ios_base::in:
    return binary ? "rb" : "r";
  case ios_base::in | ios_base::out:
    return binary ? "r+b" : "r+";
  case ios_base::in | ios_base::out | ios_base::trunc:
    return binary ? "w+b" : "w+";
  case ios_base::in | ios_base::out | ios_base::trunc | ios_base::noreplace:
    return binary ? "w+bx" : "w+x";
  case ios_base::in | ios_base::out | ios_base::app:
  case ios_base::in | ios_base::app:
    return binary ? "a+b" : "a+";
  default:
    return nullptr;
  }
}

std::FILE* __file(void* __f) noexcept { return static_cast<std::FILE*>(__f); }

} // namespace

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

void* __file_open(const char* name, std::ios_base::openmode __mode) noexcept {
  const char* m = stdio_mode(__mode);
  if (m == nullptr)
    return nullptr;
  std::FILE* __f = std::fopen(name, m);
  if (__f == nullptr)
    return nullptr;
  std::setvbuf(__f, nullptr, _IONBF, 0);
  // [filebuf.members]/4-5: ate positions the file at its end; a failure closes it again.
  if ((__mode & std::ios_base::ate) && std::fseek(__f, 0, SEEK_END) != 0) {
    std::fclose(__f);
    return nullptr;
  }
  return __f;
}

bool __file_close(void* __f) noexcept { return std::fclose(__file(__f)) == 0; }

// One read(2): up to n bytes, fewer when fewer are available; 0 at end of file (a file or a
// terminal that has grown can be read further) or on an error. An interrupted read is retried.
std::size_t __file_read(void* __f, char* __buf, std::size_t n) noexcept {
  const int fd = ::fileno(__file(__f));
  for (;;) {
    const ssize_t __got = ::read(fd, __buf, n);
    if (__got >= 0)
      return static_cast<std::size_t>(__got);
    if (errno != EINTR)
      return 0;
  }
}

bool __file_write(void* __f, const char* __buf, std::size_t n) noexcept {
  return n == 0 || std::fwrite(__buf, 1, n, __file(__f)) == n;
}

long long __file_seek(void* __f, long long __off, int __whence) noexcept {
  // lseek, not fseeko/ftello: stdio may cache the offset, and reads bypass it
  const int __w = __whence == 0 ? SEEK_SET : __whence == 1 ? SEEK_CUR : SEEK_END;
  const off_t r = ::lseek(::fileno(__file(__f)), static_cast<off_t>(__off), __w);
  return r < 0 ? -1 : static_cast<long long>(r);
}

bool __file_flush(void* __f) noexcept { return std::fflush(__file(__f)) == 0; }

int __file_native(void* __f) noexcept { return ::fileno(__file(__f)); }

}} // namespace __ycxx::__detail
