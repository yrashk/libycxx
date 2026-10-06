// The wide standard objects with a named locale imbued.
// [iostream.objects.overview]/2: their stream buffers "have the behavior specified for
// std::basic_filebuf<wchar_t>"; [filebuf.virtuals]/10 (overflow) and /3-4 (underflow): a
// basic_filebuf<wchar_t> converts characters with a_codecvt.out / a_codecvt.in, a_codecvt being
// the codecvt<wchar_t, char, mbstate_t> of getloc(); [streambuf.virt.locales]: imbue changes the
// translations. [basic.ios.members]: wcout.imbue(loc) imbues loc in the stream and its buffer.
// [locale.codecvt.byname], [locale.facet]/5: the named locale's codecvt converts as that
// locale's encoding does, whatever the C library's current (global) locale is.
// [facet.num.put.virtuals] (stage 2): num_put uses the stream locale's numpunct (decimal_point,
// thousands_sep, grouping), so a named locale's punctuation appears in the converted output.
// So, with the global C locale left at "C":
// - wcout imbued with an ISO-8859-1 locale writes U+00E9 U+00DF as the bytes E9 DF;
// - wcout imbued with a UTF-8 locale writes them as their UTF-8 sequences;
// - wcin imbued with an ISO-8859-1 locale reads the bytes E9 DF as U+00E9 U+00DF.
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <string>
#include "child_process.hpp"
#include "check.hpp"
#include "named_locale.hpp"

static const char* latin1;
static const char* utf8;

static int child_latin1_out() {
  std::wcout.imbue(std::locale(latin1));
  std::wcout << L"\u00e9\u00df " << 1234.5 << std::flush;
  return std::wcout.good() ? 0 : 2;
}

static int child_utf8_out() {
  std::wcout.imbue(std::locale(utf8));
  std::wcout << L"\u00e9\u20ac" << std::flush;
  return std::wcout.good() ? 0 : 2;
}

static int child_latin1_in() {
  char path[] = "/tmp/ycxx_wide_named_XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) return 2;
  if (write(fd, "\xe9\xdf x", 4) != 4) return 3;
  close(fd);
  if (!std::freopen(path, "r", stdin)) return 4;
  unlink(path);
  std::wcin.imbue(std::locale(latin1));
  std::wstring s;
  std::wcin >> s;
  if (s != L"\u00e9\u00df") return 5;
  wchar_t c = 0;
  std::wcin >> c;
  if (c != L'x') return 6;
  return 0;
}

int main(int argc, char** argv) {
  latin1 = require_locale("de_DE.ISO8859-1");
  utf8 = require_locale("de_DE.UTF-8");
  if (child_mode()) {
    (void)argc;
    const std::string m = argv[1];
    return m == "latin1_out" ? child_latin1_out() : m == "utf8_out" ? child_utf8_out() : child_latin1_in();
  }
  ChildResult r = run_self("latin1_out");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "\xe9\xdf 1.234,5", "latin1_out"));
  r = run_self("utf8_out");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "\xc3\xa9\xe2\x82\xac", "utf8_out"));
  r = run_self("latin1_in");
  CHECK(r.status == 0);
  return 0;
}
