// [ios.members.static]/1-2: sync_with_stdio(false), called before any input or output on the
// standard streams, "allows the standard streams to operate independently of the standard C
// streams"; it returns true if the objects were synchronized before (the first call returns
// true), false otherwise. [iostream.objects.overview]/3: the objects are not destroyed, and
// [ios.init]: the destruction of the last ios_base::Init flushes cout, cerr, clog, wcout, wcerr
// and wclog ([ios.init]/4: "Effects: Calls cout.flush(), cerr.flush(), clog.flush(),
// wcout.flush(), wcerr.flush(), wclog.flush()"), so output that is still buffered appears at
// normal termination. flush() writes it at once ([ostream.unformatted]).
// Checked for the wide objects: their output arrives completely and in order, with explicit
// flushes and at exit, with the standard facets and with an imbued codecvt; wcerr (unitbuf)
// writes at each insertion.
#include <cstdio>
#include <iostream>
#include <locale>
#include <string>
#include "child_process.hpp"
#include "check.hpp"

struct upper_codecvt : std::codecvt<wchar_t, char, std::mbstate_t> {
protected:
  result do_out(std::mbstate_t&, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                char* to, char* to_end, char*& to_next) const override {
    for (; from != from_end && to != to_end; ++from, ++to)
      *to = static_cast<char>(*from >= L'a' && *from <= L'z' ? *from - L'a' + L'A' : *from);
    from_next = from;
    to_next = to;
    return ok;
  }
  bool do_always_noconv() const noexcept override { return false; }
};

static int child_plain() {
  if (!std::ios_base::sync_with_stdio(false)) return 2;
  if (std::ios_base::sync_with_stdio(false)) return 3; // now unsynchronized
  std::wcout << L"first ";
  std::wcout.flush();
  for (int i = 0; i < 3000; ++i) // more than any buffer holds
    std::wcout << (i % 10);
  std::wcout << L'\n';
  std::wcerr << L"err";
  std::wcout << L"at exit"; // flushed by ios_base::Init's destruction
  return std::wcout.good() && std::wcerr.good() ? 0 : 4;
}

static int child_imbued() {
  std::ios_base::sync_with_stdio(false);
  std::wcout.imbue(std::locale(std::locale::classic(), new upper_codecvt));
  std::wcout << L"abc" << std::flush << L"def";
  return std::wcout.good() ? 0 : 2;
}

int main(int argc, char** argv) {
  if (child_mode()) {
    (void)argc;
    return std::string(argv[1]) == "plain" ? child_plain() : child_imbued();
  }
  std::string digits;
  for (int i = 0; i < 3000; ++i)
    digits += static_cast<char>('0' + i % 10);
  ChildResult r = run_self("plain");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "first " + digits + "\nat exit", "plain"));
  CHECK(same_text(r.err, "err", "plain stderr"));
  r = run_self("imbued");
  CHECK(r.status == 0);
  CHECK(same_text(r.out, "ABCDEF", "imbued"));
  return 0;
}
