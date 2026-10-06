// Positioning a wide file stream whose locale's codecvt is a variable-width encoding (a named
// UTF-8 locale: [locale.codecvt.virtuals]/10, do_encoding() is 0, "not a constant").
// [filebuf.virtuals]/13: with width = a_codecvt.encoding() <= 0, seekoff with off != 0 fails;
// seekoff(0, cur) (tellg, [istream.unformatted]) does not, and /14 returns the resultant stream
// position; /16-18: seekpos to a position obtained from a previous successful positioning call
// on the same file sets the file to that position and returns sp. So a position taken with
// tellg after reading some multibyte characters is where the next character starts: seeking
// back to it reads the same characters again.
// [istream.unformatted] seekg: a failed positioning sets failbit.
#include <fstream>
#include <iterator>
#include <locale>
#include <string>
#include "fs_tmpdir.hpp"
#include "named_locale.hpp"
#include "check.hpp"

int main() {
  const std::locale utf8(require_locale("de_DE.UTF-8"));
  CHECK(std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(utf8).encoding() == 0);
  TmpDir dir;
  const std::string path = dir / "positions.txt";
  {
    std::ofstream out(path, std::ios::binary);
    out << "\xe2\x82\xac\xc3\xa4x\xf0\x9f\x98\x80yz"; // U+20AC U+00E4 'x' U+1F600 'y' 'z'
  }
  std::wifstream in;
  in.imbue(utf8);
  in.open(path);
  CHECK(in.is_open());
  CHECK(in.get() == L'\u20ac');
  CHECK(in.get() == L'\u00e4');
  const auto p1 = in.tellg();
  CHECK(p1 != std::wifstream::pos_type(-1));
  CHECK(in.get() == L'x');
  const wchar_t smile = static_cast<wchar_t>(0x1F600);
  CHECK(in.get() == smile);
  const auto p2 = in.tellg();
  CHECK(p2 != std::wifstream::pos_type(-1));
  std::wstring rest;
  std::getline(in, rest);
  CHECK(rest == L"yz");
  in.clear();

  // back to p1: the same characters again
  in.seekg(p1);
  CHECK(in.good());
  CHECK(in.get() == L'x');
  CHECK(in.get() == smile);
  // to p2
  in.seekg(p2);
  CHECK(in.get() == L'y');
  // to the beginning (offset 0 from beg is allowed whatever the width)
  in.seekg(0, std::ios_base::beg);
  CHECK(in.good() && in.get() == L'\u20ac');

  // an offset other than 0 fails with a variable-width encoding
  in.seekg(1, std::ios_base::cur);
  CHECK(in.fail());
  in.clear();
  in.seekg(-1, std::ios_base::end);
  CHECK(in.fail());
}
