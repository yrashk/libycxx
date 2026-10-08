// libycxx hosted runtime: the file operations of basic_filebuf ([filebuf.members]), declared in
// ycxx/hosted/fstream.hpp. A file is a C stdio FILE with stdio's own buffering turned off (the
// filebuf buffers); offsets are bytes.
#include <ios>
#include <cstdio>

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

// The end-of-file indicator is cleared first, so that a file (or a terminal) that has grown can
// be read further.
std::size_t __file_read(void* __f, char* __buf, std::size_t n) noexcept {
  std::clearerr(__file(__f));
  return std::fread(__buf, 1, n, __file(__f));
}

bool __file_write(void* __f, const char* __buf, std::size_t n) noexcept {
  return n == 0 || std::fwrite(__buf, 1, n, __file(__f)) == n;
}

long long __file_seek(void* __f, long long __off, int __whence) noexcept {
  const int __w = __whence == 0 ? SEEK_SET : __whence == 1 ? SEEK_CUR : SEEK_END;
  if (::fseeko(__file(__f), static_cast<off_t>(__off), __w) != 0)
    return -1;
  return static_cast<long long>(::ftello(__file(__f)));
}

bool __file_flush(void* __f) noexcept { return std::fflush(__file(__f)) == 0; }

int __file_native(void* __f) noexcept { return ::fileno(__file(__f)); }

}} // namespace __ycxx::__detail
