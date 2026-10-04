// libycxx hosted runtime: the file operations of basic_filebuf ([filebuf.members]), declared in
// ycxx/hosted/fstream.hpp. A file is a C stdio FILE with stdio's own buffering turned off (the
// filebuf buffers); offsets are bytes.
#include <ios>
#include <cstdio>

namespace {

// [filebuf.members] Table 146: the fopen mode for `mode & ~ate`, or null if the combination is
// not in the table.
const char* stdio_mode(std::ios_base::openmode mode) noexcept {
  using std::ios_base;
  const bool binary = (mode & ios_base::binary) != 0;
  switch (mode & ~(ios_base::ate | ios_base::binary)) {
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

std::FILE* file(void* f) noexcept { return static_cast<std::FILE*>(f); }

} // namespace

namespace ycxx::detail {

void* file_open(const char* name, std::ios_base::openmode mode) noexcept {
  const char* m = stdio_mode(mode);
  if (m == nullptr)
    return nullptr;
  std::FILE* f = std::fopen(name, m);
  if (f == nullptr)
    return nullptr;
  std::setvbuf(f, nullptr, _IONBF, 0);
  // [filebuf.members]/4-5: ate positions the file at its end; a failure closes it again.
  if ((mode & std::ios_base::ate) && std::fseek(f, 0, SEEK_END) != 0) {
    std::fclose(f);
    return nullptr;
  }
  return f;
}

bool file_close(void* f) noexcept { return std::fclose(file(f)) == 0; }

// The end-of-file indicator is cleared first, so that a file (or a terminal) that has grown can
// be read further.
std::size_t file_read(void* f, char* buf, std::size_t n) noexcept {
  std::clearerr(file(f));
  return std::fread(buf, 1, n, file(f));
}

bool file_write(void* f, const char* buf, std::size_t n) noexcept {
  return n == 0 || std::fwrite(buf, 1, n, file(f)) == n;
}

long long file_seek(void* f, long long off, int whence) noexcept {
  const int w = whence == 0 ? SEEK_SET : whence == 1 ? SEEK_CUR : SEEK_END;
  if (::fseeko(file(f), static_cast<off_t>(off), w) != 0)
    return -1;
  return static_cast<long long>(::ftello(file(f)));
}

bool file_flush(void* f) noexcept { return std::fflush(file(f)) == 0; }

int file_native(void* f) noexcept { return ::fileno(file(f)); }

} // namespace ycxx::detail
