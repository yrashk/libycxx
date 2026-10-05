// Wide file streams convert through the imbued locale's codecvt<wchar_t, char, mbstate_t>
// ([filebuf.general]/7: "the character sequences of a basic_filebuf<charT, traits> are
// converted as if by a.out / a.in of the codecvt facet"); the "C.UTF-8" locale's facet
// converts UTF-8. Multi-byte characters must survive however the file's bytes are split into
// the filebuf's buffer (the file is much longer than any buffer, and 3- and 4-byte sequences
// fall on every residue). [filebuf.virtuals]/14-18: seekoff(0, cur) reports the position
// (allowed for a variable-width encoding, "if ... off != 0 and width <= 0, the positioning
// operation fails"); seekpos(sp) returns to a position obtained from it, restoring the
// conversion state (fpos keeps the state, [fpos.members]), so re-reading from it gives the same
// characters. [istream.unformatted] tellg / seekg; [ostream.seeks].
// REQUIRES: exceptions
// COUNTERPART: libcxx:input.output/file.streams/fstreams/(filebuf.virtuals/xsputn|ifstream.members/buffered_reads|ofstream.members/buffered_writes).pass.cpp
#include <fstream>
#include <locale>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

int main() {
  std::locale utf8;
  try {
    utf8 = std::locale("C.UTF-8");
  } catch (...) {
    return 0;  // the environment has no such locale: nothing to check
  }
  TmpDir dir;
  const std::string p = dir / "w.txt";
  std::wstring text;
  for (int i = 0; i < 7000; ++i) {
    text += L"\x20ac";  // 3 bytes
    if (i % 7 == 0) text += L"a";
    if (i % 11 == 0) text += static_cast<wchar_t>(0x1F600);  // 4 bytes
    if (i % 13 == 0) text += L"\xe9";                       // 2 bytes
  }
  {
    std::wofstream out;
    out.imbue(utf8);
    out.open(p);
    CHECK(out.is_open());
    out << text;
    CHECK(out.good());
  }
  const std::string bytes = read_file(p);
  CHECK(bytes.size() > 20000);
  CHECK(bytes.compare(0, 3, "\xE2\x82\xAC") == 0);
  {
    std::wifstream in;
    in.imbue(utf8);
    in.open(p);
    std::wstring back;
    wchar_t c;
    while (in.get(c)) back.push_back(c);
    CHECK(back == text);
  }
  {
    std::wifstream in;
    in.imbue(utf8);
    in.open(p);
    std::wstring head(5001, L'\0');
    in.read(head.data(), 5001);
    CHECK(head == text.substr(0, 5001));
    const auto pos = in.tellg();
    CHECK(pos != std::wifstream::pos_type(-1));
    std::wstring a(777, L'\0'), b(777, L'\0');
    in.read(a.data(), 777);
    CHECK(a == text.substr(5001, 777));
    in.seekg(pos);
    CHECK(in.good());
    in.read(b.data(), 777);
    CHECK(b == a);
  }
  return 0;
}
