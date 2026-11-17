// [filebuf.virtuals]/14 (underflow: "some initial subsequence of the characters" not yet read),
// [streambuf.virt.get]/16: a filebuf on a FIFO delivers the characters the writer has written
// without waiting for its buffer to fill or for the end of the file. The writer sends one line
// and keeps its end open until the reader has it: getline must return that line (a reader that
// waited for 4096 bytes or the end of the file would never return, and the watchdog ends the
// test). Narrow, wide (converted through codecvt<wchar_t, char>) and unbuffered (setbuf(0, 0))
// filebufs; after the writer closes, the rest is read and the end of the file is seen.
#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fs_tmpdir.hpp"
#include "watchdog.hpp"
#include "check.hpp"

// Writes `line` to the FIFO, then holds it open until `done`.
static void writer(const std::string& fifo, const char* line, std::atomic<bool>& done) {
  const int fd = ::open(fifo.c_str(), O_WRONLY);
  CHECK(fd >= 0);
  const size_t n = strlen(line);
  CHECK(::write(fd, line, n) == static_cast<ssize_t>(n));
  while (!done.load())
    std::this_thread::yield();
  ::close(fd);
}

template <class Stream, class String>
static void check_fifo(const TmpDir& dir, const char* name, const char* line, const String& want,
                       bool unbuffered) {
  const std::string fifo = dir / name;
  CHECK(::mkfifo(fifo.c_str(), 0600) == 0);
  std::atomic<bool> done{false};
  std::thread w(writer, fifo, line, std::ref(done));
  Stream in;
  if (unbuffered)
    in.rdbuf()->pubsetbuf(nullptr, 0);
  in.open(fifo); // returns when the writer has opened its end
  CHECK(in.is_open());
  String got;
  CHECK(std::getline(in, got));
  CHECK(got == want);
  done.store(true); // the writer closes: the end of the file
  w.join();
  CHECK(std::getline(in, got)); // the rest of the line, ended by the end of the file
  CHECK(got.size() == 3 && got[0] == 'w' && got[2] == 'r');
  CHECK(in.eof() && !in.fail());
}

int main() {
  watchdog(20);
  TmpDir dir;
  check_fifo<std::ifstream>(dir, "narrow", "hello\nwor", std::string("hello"), false);
  check_fifo<std::ifstream>(dir, "narrow_unbuf", "hello\nwor", std::string("hello"), true);
  check_fifo<std::wifstream>(dir, "wide", "h\xc3\xa9llo\nwor", std::wstring(L"héllo"), false);
  check_fifo<std::wifstream>(dir, "wide_unbuf", "h\xc3\xa9llo\nwor", std::wstring(L"héllo"), true);
}
