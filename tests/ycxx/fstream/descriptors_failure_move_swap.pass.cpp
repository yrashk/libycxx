// File streams release their file descriptors when open fails, when close fails, and across
// move construction, move assignment and swap.
//   [filebuf.members]/3-6 open: "If is_open() != false, returns a null pointer" (the file
//     already open stays open, and nothing else is opened); "If the open operation fails,
//     returns a null pointer" (nothing is left open).
//   /8 close: flushes ("calls overflow(traits::eof())"), then "Finally, regardless of whether any
//     of the preceding calls fails or throws an exception, the function closes the file (as if
//     by calling fclose(file))"; "Returns: this on success, a null pointer otherwise"; /9
//     Postconditions: is_open() == false. Here the flush fails (a write to a pipe without a
//     reader: EPIPE), so close returns a null pointer and still closes the file.
//   [ofstream.members] close: "Calls rdbuf()->close() and, if that function fails (returns a
//     null pointer), calls setstate(failbit)".
//   [filebuf.cons]/5: ~basic_filebuf calls close() (also when the flush fails).
//   [filebuf.cons]/3-4 move construction takes over rhs's file (rhs is no longer open);
//   [filebuf.assign]/1 operator=: "Calls close() then move assigns from rhs"; /3 swap exchanges
//   the state; [fstream.cons], [fstream.assign] (move and swap of the streams' filebufs).
// Each case runs many times; the process's open descriptors are counted.
// COUNTERPART: libcxx:input.output/file.streams/fstreams/filebuf.assign/(member_swap|move_assign|nonmember_swap).pass.cpp
// COUNTERPART: libcxx:input.output/file.streams/fstreams/filebuf.cons/move.pass.cpp
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "check.hpp"
#include "fs_tmpdir.hpp"

static int open_fds() {
  int n = 0;
  for (int fd = 0; fd < 1024; ++fd) n += fcntl(fd, F_GETFD) != -1;
  return n;
}

static std::string read_all(const std::string& p) {
  std::ifstream in(p);
  std::string s, line;
  while (std::getline(in, line)) s += line + "\n";
  return s;
}

int main() {
  signal(SIGPIPE, SIG_IGN);
  TmpDir tmp;
  const std::string dir = tmp / "d";
  CHECK(mkdir(dir.c_str(), 0700) == 0);
  const std::string f1 = tmp / "one", f2 = tmp / "two", f3 = tmp / "three", fifo = tmp / "fifo";
  const std::string big(100000, 'x');  // more than any buffer: the writes reach the file
  const int base = open_fds();

  for (int i = 0; i < 200; ++i) {
    std::filebuf fb;
    CHECK(fb.open(tmp / "missing/file", std::ios::in) == nullptr);
    CHECK(fb.open(dir, std::ios::out) == nullptr);  // a directory
    CHECK(fb.open(f1, std::ios::in | std::ios::app | std::ios::trunc) == nullptr);  // no such mode
    CHECK(!fb.is_open());
    CHECK(fb.open(f1, std::ios::out) == &fb);
    CHECK(fb.open(f2, std::ios::out) == nullptr);  // already open
    CHECK(fb.is_open());
    CHECK(open_fds() == base + 1);
    CHECK(fb.close() == &fb);
    CHECK(open_fds() == base);
    std::ifstream in(tmp / "missing/file");
    CHECK(!in.is_open() && in.fail());
  }
  CHECK(open_fds() == base);

  // close() whose flush fails.
  CHECK(mkfifo(fifo.c_str(), 0600) == 0);
  for (int i = 0; i < 100; ++i) {
    for (int form = 0; form < 3; ++form) {
      const int reader = open(fifo.c_str(), O_RDONLY | O_NONBLOCK);
      CHECK(reader >= 0);
      if (form == 0) {
        std::filebuf fb;
        CHECK(fb.open(fifo, std::ios::out) == &fb);
        close(reader);  // no reader any more: writes fail with EPIPE
        fb.sputn("abc", 3);
        CHECK(fb.close() == nullptr);
        CHECK(!fb.is_open());
      } else if (form == 1) {
        std::ofstream out(fifo);
        CHECK(out.is_open());
        close(reader);
        out << "abc";
        out.close();
        CHECK(!out.is_open());
        CHECK(out.fail());
      } else {
        std::ofstream out(fifo);
        CHECK(out.is_open());
        close(reader);
        out << "abc";  // left for the destructor's close()
      }
      CHECK(open_fds() == base);
    }
  }

  // Move construction, move assignment and swap.
  for (int i = 0; i < 200; ++i) {
    {
      std::filebuf a, b;
      CHECK(a.open(f1, std::ios::out | std::ios::trunc) && b.open(f2, std::ios::out | std::ios::trunc));
      a.sputn("a1\n", 3);
      b.sputn("b1\n", 3);
      CHECK(open_fds() == base + 2);
      a = std::move(b);  // closes f1, takes f2
      CHECK(open_fds() == base + 1);
      CHECK(a.is_open() && !b.is_open());
      a.sputn("a2\n", 3);
      std::filebuf c(std::move(a));
      CHECK(open_fds() == base + 1);
      CHECK(c.is_open() && !a.is_open());
      std::filebuf d;
      CHECK(d.open(f3, std::ios::out | std::ios::trunc));
      c.swap(d);
      d.sputn("d\n", 2);  // f2
      c.sputn("c\n", 2);  // f3
      CHECK(open_fds() == base + 2);
      b = std::move(d);  // b was not open: nothing to close
      CHECK(open_fds() == base + 2);
    }
    CHECK(open_fds() == base);
    CHECK(read_all(f1) == "a1\n");
    CHECK(read_all(f2) == "b1\na2\nd\n");
    CHECK(read_all(f3) == "c\n");
    {
      std::fstream a(f1, std::ios::out | std::ios::trunc), b(f2, std::ios::out | std::ios::trunc);
      std::ofstream o(f3);
      CHECK(open_fds() == base + 3);
      a << "x";
      b << big;
      a = std::move(b);
      CHECK(open_fds() == base + 2);
      std::fstream c(std::move(a));
      swap(c, a);
      a << "y";
      std::ifstream in(f1);
      std::ifstream in2(std::move(in));
      CHECK(open_fds() == base + 3);
    }
    CHECK(open_fds() == base);
    CHECK(read_all(f1) == "x\n");
    CHECK(read_all(f2) == big + "y\n");
  }
  CHECK(open_fds() == base);
  return 0;
}
