// [narrow.stream.objects]/2: "After the object cin is initialized, cin.tie() returns &cout";
// /5: "cerr.flags() & unitbuf is nonzero and cerr.tie() returns &cout"; otherwise the state is
// as required for basic_ios<char>::init (Table 142: rdstate() goodbit, exceptions() goodbit,
// flags() skipws | dec, width() 0, precision() 6, fill() ' '); cout and clog are not tied.
// [wide.stream.objects]: the same for wcin, wcout, wcerr, wclog. [iostream.objects.overview]/3:
// the objects are constructed before main begins. [ios.members.static]: sync_with_stdio
// returns true the first time it is called (the previous setting; synchronized by default).
// Output to cout / cerr and stdio interleave in order while synchronized
// ([iostream.objects.overview]/6). Checked by redirecting the standard file descriptors to a
// temporary file.
// COUNTERPART: libstdcxx:27_io/objects/(char|wchar_t)/2523-1_xin.cc
#include <iostream>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include "fs_tmpdir.hpp"
#include "check.hpp"

template <class S>
static bool init_state(const S& s, std::ios_base::fmtflags extra = {}) {
  return s.rdstate() == std::ios_base::goodbit && s.exceptions() == std::ios_base::goodbit &&
         s.flags() == (std::ios_base::skipws | std::ios_base::dec | extra) && s.width() == 0 &&
         s.precision() == 6 && s.fill() == s.widen(' ') && s.rdbuf() != nullptr;
}

int main() {
  CHECK(std::cin.tie() == &std::cout);
  CHECK(std::cerr.tie() == &std::cout);
  CHECK(std::cout.tie() == nullptr);
  CHECK(std::clog.tie() == nullptr);
  CHECK(init_state(std::cin));
  CHECK(init_state(std::cout));
  CHECK(init_state(std::cerr, std::ios_base::unitbuf));
  CHECK(init_state(std::clog));
  CHECK(std::wcin.tie() == &std::wcout);
  CHECK(std::wcerr.tie() == &std::wcout);
  CHECK(std::wcout.tie() == nullptr && std::wclog.tie() == nullptr);
  CHECK(init_state(std::wcin));
  CHECK(init_state(std::wcout));
  CHECK(init_state(std::wcerr, std::ios_base::unitbuf));
  CHECK(init_state(std::wclog));
  CHECK(std::cout.rdbuf() != std::cerr.rdbuf());

  // synchronized with stdio: interleaving with printf keeps the order
  TmpDir dir;
  std::string path = dir / "out";
  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  CHECK(fd >= 0);
  std::fflush(stdout);
  int saved = ::dup(1);
  CHECK(::dup2(fd, 1) == 1);
  std::cout << "a";
  std::printf("b");
  std::cout << "c" << 1;
  std::fputs("d", stdout);
  std::cout << std::endl;
  std::fflush(stdout);
  ::dup2(saved, 1);
  ::close(fd);
  ::close(saved);
  CHECK(read_file(path) == "abc1d\n");
  CHECK(std::cout.good());

  CHECK(std::ios_base::sync_with_stdio(true) == true);  // previous setting: synchronized
  return 0;
}
