// [filebuf.virtuals]/12: after setbuf(0, 0) before any I/O the stream is unbuffered. A converting
// (wide) filebuf then reads the file's bytes only as far as the character it delivers, so bytes
// written to the file afterwards by another FILE are seen (libstdc++'s
// 27_io/basic_filebuf/underflow/wchar_t/5.cc does this in se_NO.UTF-8; the classic locale's
// codecvt<wchar_t, char> is UTF-8 here, DECISIONS §7). A character of several bytes is still read
// whole.
#include <cstdio>
#include <fstream>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

int main() {
  TmpDir dir;
  const std::string name = dir / "unbuf_in";
  std::FILE* file = std::fopen(name.c_str(), "w");
  CHECK(file != nullptr);
  std::setvbuf(file, nullptr, _IONBF, 0);
  std::fputs("abc\xc3\xa9" "de", file);

  std::wfilebuf fb;
  fb.pubsetbuf(nullptr, 0);
  CHECK(fb.open(name.c_str(), std::ios_base::in) != nullptr);
  CHECK(fb.sbumpc() == L'a');
  std::fseek(file, 1, SEEK_SET);
  std::fputc('0', file);
  CHECK(fb.sbumpc() == L'0');
  CHECK(fb.sbumpc() == L'c');
  CHECK(fb.sbumpc() == static_cast<std::wint_t>(0xE9));
  std::fseek(file, 5, SEEK_SET); // the 'd'
  std::fputc('1', file);
  CHECK(fb.sbumpc() == L'1');
  CHECK(fb.sbumpc() == L'e');
  CHECK(fb.sbumpc() == WEOF);
  std::fseek(file, 0, SEEK_END);
  std::fputc('2', file);
  CHECK(fb.sbumpc() == L'2');
  fb.close();
  std::fclose(file);
}
