// [iostream.objects.overview]/6: "Mixing operations on corresponding wide- and narrow-character
// streams follows the same semantics as mixing such operations on FILEs, as specified in the C
// standard library." [ios.members.static]/3: while a standard iostream object str is
// synchronized with a standard stdio stream f, inserting a character with the C stream's
// function is the same as str.rdbuf()->sputc(c), extracting one is the same as
// str.rdbuf()->sbumpc(), pushing one back (ungetc) the same as str.rdbuf()->sputbackc(c), for
// any sequence of characters (footnote: operations on the object can be mixed arbitrarily with
// operations on the stdio stream). For the wide objects the C stream's functions are the wide
// ones (fputwc, fgetwc, ungetwc): C17 7.21.2/4 gives a stream its orientation with its first
// operation, wide for a wide character function. So:
// - after wcout << L"...", stdout is wide-oriented (fwide(stdout, 0) > 0) and fputws, putwc and
//   wprintf on stdout keep working, interleaved in program order with wcout's output; the same
//   for wcerr, wclog and stderr;
// - wprintf followed by wcout: one wide-oriented stream, all output in order;
// - printf followed by cout: one byte-oriented stream (fwide(stdout, 0) < 0), all in order;
// - wcin: wcin.get() then wcin.unget() gives the character back to fgetwc(stdin); a character
//   pushed back with ungetwc is the next one wcin extracts; peek() leaves the character for
//   fgetwc; rdbuf()->sungetc() after sgetn gives the last character back; stdin becomes
//   wide-oriented.
// COUNTERPART: libstdcxx:27_io/objects/wchar_t/(9662|12048-2|12048-4).cc
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <string>
#include "child_process.hpp"
#include "check.hpp"

static int child_wout() {
  std::wcout << L"Hello, ";
  if (!std::wcout.good()) return 2;
  if (std::fwide(stdout, 0) <= 0) return 3;
  if (std::fputws(L"world!\n", stdout) < 0) return 4;
  std::wcout << 42 << L' ';
  if (std::putwc(L'x', stdout) == WEOF) return 5;
  std::wcout << L'y' << L"z\n" << std::flush;
  if (!std::wcout.good()) return 6;
  return 0;
}

static int child_werr() {
  std::wcerr << L"e1 ";
  if (std::fwide(stderr, 0) <= 0) return 2;
  if (std::fputws(L"c1 ", stderr) < 0) return 3;
  std::wclog << L"l1 " << std::flush;
  std::fwprintf(stderr, L"c%d ", 2);
  std::wcerr << L"e2";
  if (!std::wcerr.good() || !std::wclog.good()) return 4;
  return 0;
}

static int child_wprintf_wcout() {
  std::wprintf(L"a%d", 1);
  std::wcout << L"b";
  std::wprintf(L"c");
  std::wcout << L"d" << std::endl;
  if (!std::wcout.good() || std::fwide(stdout, 0) <= 0) return 2;
  return 0;
}

static int child_printf_cout() {
  std::printf("x%d", 1);
  std::cout << "y";
  std::fputs("z", stdout);
  std::cout << "w" << std::endl;
  if (!std::cout.good() || std::fwide(stdout, 0) >= 0) return 2;
  return 0;
}

static int child_win() {
  char path[] = "/tmp/ycxx_wide_objects_XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) return 2;
  if (write(fd, "abcdef", 6) != 6) return 3;
  close(fd);
  if (!std::freopen(path, "r", stdin)) return 4;
  unlink(path);

  wchar_t c1 = 0;
  std::wcin.get(c1);
  if (c1 != L'a') return 10;
  std::wcin.unget();
  if (!std::wcin.good()) return 11;
  if (std::fwide(stdin, 0) <= 0) return 12;
  if (std::fgetwc(stdin) != L'a') return 13; // the character given back
  if (std::wcin.peek() != L'b') return 14;
  if (std::fgetwc(stdin) != L'b') return 15; // peek left it in the C stream
  if (std::ungetwc(L'Q', stdin) == WEOF) return 16;
  if (std::wcin.get() != L'Q') return 17; // ungetwc is sputbackc
  wchar_t buf[2];
  if (std::wcin.rdbuf()->sgetn(buf, 2) != 2 || buf[0] != L'c' || buf[1] != L'd') return 18;
  if (std::wcin.rdbuf()->sungetc() != L'd') return 19;
  if (std::fgetwc(stdin) != L'd') return 20;
  if (std::wcin.rdbuf()->sbumpc() != L'e') return 21;
  if (std::fgetwc(stdin) != L'f') return 22;
  if (std::wcin.get() != std::char_traits<wchar_t>::eof() || !std::wcin.eof()) return 23;
  return 0;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    (void)argc;
    const std::string m = argv[1];
    return m == "wout"    ? child_wout()
           : m == "werr"  ? child_werr()
           : m == "wprintf" ? child_wprintf_wcout()
           : m == "printf" ? child_printf_cout()
                            : child_win();
  }
  struct Case {
    const char* mode;
    const char* out;
    const char* err;
  };
  for (Case c : {Case{"wout", "Hello, world!\n42 xyz\n", ""}, Case{"werr", "", "e1 c1 l1 c2 e2"},
                 Case{"wprintf", "a1bcd\n", ""}, Case{"printf", "x1yzw\n", ""}, Case{"win", "", ""}}) {
    ChildResult r = run_self(c.mode);
    if (r.status != 0) dprintf(2, "%s: child status %d\n", c.mode, r.status);
    CHECK(r.status == 0);
    CHECK(same_text(r.out, c.out, c.mode));
    CHECK(same_text(r.err, c.err, c.mode));
  }
  return 0;
}
