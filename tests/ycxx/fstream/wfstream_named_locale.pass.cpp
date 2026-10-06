// Wide file streams with named locales imbued.
// [filebuf.virtuals]/10-11: a basic_filebuf<wchar_t> converts what it writes with a_codecvt.out
// and fails (returns eof) if the conversion fails; /3: it converts what it reads with
// a_codecvt.in; a_codecvt is the codecvt facet of its locale; /20-21 (imbue, before any I/O):
// the new locale's facet is used from then on.
// [locale.codecvt.byname], [locale.facet]/5: a named locale's codecvt<wchar_t, char, mbstate_t>
// converts as that locale's encoding: ISO-8859-1 maps U+0000-U+00FF to single bytes and cannot
// represent U+20AC; UTF-8 encodes it in three bytes.
// [locale.codecvt.virtuals]/10: do_encoding() is the constant number of external characters per
// internal character (1 for ISO-8859-1); [filebuf.virtuals]/13-17: seekoff seeks width * off
// bytes when width = a_codecvt.encoding() > 0, and seekpos goes back to a position tellg gave.
// [ostream.unformatted] flush: badbit when pubsync() returns -1.
#include <fstream>
#include <iterator>
#include <locale>
#include <string>
#include "fs_tmpdir.hpp"
#include "named_locale.hpp"
#include "check.hpp"

static std::string bytes_of(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main() {
  const std::locale latin1(require_locale("de_DE.ISO8859-1"));
  const std::locale utf8(require_locale("de_DE.UTF-8"));
  TmpDir dir;
  const std::string p1 = dir / "latin1.txt", p2 = dir / "utf8.txt", p3 = dir / "bad.txt";

  CHECK(std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(latin1).encoding() == 1);
  {
    std::wofstream out;
    out.imbue(latin1);
    out.open(p1);
    CHECK(out.is_open());
    out << L"äöü " << 1234.5;
    out.close();
    CHECK(out.good());
  }
  CHECK(bytes_of(p1) == "\xe4\xf6\xfc 1.234,5");
  {
    std::wifstream in;
    in.imbue(latin1);
    in.open(p1);
    std::wstring s;
    double d = 0;
    in >> s >> d;
    CHECK(s == L"äöü");
    CHECK(d == 1234.5);
    // fixed width 1: seek by characters
    in.clear();
    in.seekg(2);
    CHECK(in.good());
    CHECK(in.get() == L'ü');
    CHECK(in.tellg() == std::wifstream::pos_type(3));
  }
  {
    std::wofstream out;
    out.imbue(utf8);
    out.open(p2);
    out << L"€ä";
    out.close();
    CHECK(out.good());
  }
  CHECK(bytes_of(p2) == "\xe2\x82\xac\xc3\xa4");
  {
    std::wifstream in;
    in.imbue(utf8);
    in.open(p2);
    std::wstring s;
    std::getline(in, s);
    CHECK(s == L"€ä");
    CHECK(in.eof() && !in.bad());
  }
  {
    // U+20AC has no ISO-8859-1 byte: the conversion fails and the stream reports it
    std::wofstream out;
    out.imbue(latin1);
    out.open(p3);
    out << L"a€";
    out.flush();
    CHECK(out.bad());
  }
}
