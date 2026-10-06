// [iostream.objects.overview]/2: the stream buffers of the standard objects "have the behavior
// specified for std::basic_filebuf<char> or std::basic_filebuf<wchar_t>". [filebuf.virtuals]:
// a basic_filebuf<wchar_t> converts the characters it writes with a_codecvt.out and those it
// reads with a_codecvt.in, where a_codecvt is the codecvt<wchar_t, char, mbstate_t> of its locale
// (getloc()), and imbue(loc) changes that locale ([streambuf.virt.locales]: pubimbue calls
// imbue, which "changes any translations based on locale"). [ios.base.locales],
// [basic.ios.members]: wcout.imbue(loc) calls rdbuf()->pubimbue(loc).
// So a program's own codecvt facet imbued in wcout converts what wcout writes, and one imbued in
// wcin converts what it reads, the converted bytes going to and coming from the C stream.
// [locale.codecvt.virtuals]/2-3: do_out/do_in convert from [from, from_end) into [to, to_end).
// Imbuing a locale with the standard codecvt facets (classic()) keeps the wide objects
// synchronized with the C stream's wide functions ([iostream.objects.overview]/6, see
// iostreams/wide_objects_c_orientation): stdout stays usable with fputws.
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <locale>
#include <string>
#include "child_process.hpp"
#include "check.hpp"

// Writes every character as 'z'; reads every byte as L'z'.
struct z_codecvt : std::codecvt<wchar_t, char, std::mbstate_t> {
protected:
  result do_out(std::mbstate_t&, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                char* to, char* to_end, char*& to_next) const override {
    while (from != from_end && to != to_end) {
      ++from;
      *to++ = 'z';
    }
    from_next = from;
    to_next = to;
    return ok;
  }
  result do_in(std::mbstate_t&, const char* from, const char* from_end, const char*& from_next, wchar_t* to,
               wchar_t* to_end, wchar_t*& to_next) const override {
    while (from != from_end && to != to_end) {
      ++from;
      *to++ = L'z';
    }
    from_next = from;
    to_next = to;
    return ok;
  }
  bool do_always_noconv() const noexcept override { return false; }
  int do_encoding() const noexcept override { return 1; }
};

static int child_out() {
  std::wcout.imbue(std::locale(std::locale::classic(), new z_codecvt));
  std::wcout << L"1234" << 56;
  std::wcout.flush();
  if (!std::wcout.good()) return 2;
  return 0;
}

static int child_in() {
  char path[] = "/tmp/ycxx_wide_imbue_XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) return 2;
  if (write(fd, "1234 ab", 7) != 7) return 3;
  close(fd);
  if (!std::freopen(path, "r", stdin)) return 4;
  unlink(path);
  std::wcin.imbue(std::locale(std::locale::classic(), new z_codecvt));
  std::wstring s;
  std::wcin >> s;
  if (s != L"zzzzzzz") return 5; // the space, too, is read as 'z'
  return 0;
}

// The classic locale's facets: the C stream's wide functions, as before the imbue.
static int child_classic() {
  std::wcout.imbue(std::locale::classic());
  std::wcout << L"ab";
  if (std::fwide(stdout, 0) <= 0) return 2;
  if (std::fputws(L"cd", stdout) < 0) return 3;
  std::wcout << L"ef" << std::flush;
  return std::wcout.good() ? 0 : 4;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    (void)argc;
    const std::string m = argv[1];
    return m == "out" ? child_out() : m == "in" ? child_in() : child_classic();
  }
  ChildResult r = run_self("out");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "zzzzzz", "out"));
  r = run_self("in");
  CHECK(r.status == 0);
  r = run_self("classic");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "abcdef", "classic"));
  return 0;
}
