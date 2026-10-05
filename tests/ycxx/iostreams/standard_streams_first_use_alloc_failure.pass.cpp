// The first output of a process to the standard streams (cout, cerr, wcout) happens while
// operator new fails: every call from the k-th on fails, for k = 1, 2, ... until the output
// makes no allocation that fails. Each run is a fresh child process whose standard output and
// standard error are captured.
//   [iostream.objects.overview]: the objects are constructed before main and usable at any
//     time; [ostream.formatted.reqmts]/1: "If an exception is thrown during output, then
//     ios_base::badbit is set in *this's error state. If (exceptions() & badbit) != 0 then the
//     exception is rethrown" (exceptions() is goodbit here, [ios.basic.cons]/[basic.ios.cons]
//     Table: exceptions() goodbit), so nothing escapes the inserter; [ostream.unformatted]/1
//     likewise for write(); [ostream.unformatted] flush(): badbit if pubsync fails.
//   After the failure the stream is usable again once memory is available: clear() resets
//     the state ([iostate.flags]) and the next output appears.
// Checked: the child exits normally; when no badbit was set the output is exactly what was
// written; in every case the output after clear() appears last; the stream's state is good
// after the second output.
// REQUIRES: exceptions
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static long fail_from = 0, calls = 0;
static bool failed = false;
static void* allocate(std::size_t n, std::size_t align, bool nothrow) {
  if (fail_from && ++calls >= fail_from) {
    failed = true;
    if (nothrow) return nullptr;
    throw std::bad_alloc();
  }
  if (n == 0) n = 1;
  void* p = align <= alignof(std::max_align_t) ? std::malloc(n) : std::aligned_alloc(align, (n + align - 1) / align * align);
  if (!p && !nothrow) throw std::bad_alloc();
  return p;
}
void* operator new(std::size_t n) { return allocate(n, 0, false); }
void* operator new[](std::size_t n) { return allocate(n, 0, false); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

enum Which { to_cout, to_cerr, to_wcout, to_cout_unsynced };

// In the child: the output under failure, then a status line on fd 2 ("bad" or "good"), then
// the output after clear().
static void child(Which w, long k) {
  if (w == to_cout_unsynced) std::ios_base::sync_with_stdio(false);  // (before any output)
  bool bad = false, good_after = false;
  fail_from = k;
  if (w == to_wcout) {
    std::wcout << L"wide " << 42 << L' ' << 3.5 << L'\n';
    std::wcout.flush();
    bad = std::wcout.bad();
    fail_from = 0;
    std::wcout.clear();
    std::wcout << L"after\n" << std::flush;
    good_after = std::wcout.good();
  } else {
    std::ostream& os = w == to_cerr ? std::cerr : std::cout;
    os << "narrow " << 42 << ' ' << 3.5 << '\n';
    os.write("written\n", 8);
    os.flush();
    bad = os.bad();
    fail_from = 0;
    os.clear();
    os << "after\n" << std::flush;
    good_after = os.good();
  }
  std::fflush(stdout);
  std::fflush(stderr);
  const char* status = !good_after ? "|BAD-AFTER" : bad ? "|bad" : "|good";
  if (write(2, status, std::char_traits<char>::length(status)) < 0) _exit(3);
  _exit(failed ? 0 : 10);
}

struct Run {
  int status;
  std::string out, err;
};

static Run run_child(Which w, long k) {
  int o[2], e[2];
  CHECK(pipe(o) == 0 && pipe(e) == 0);
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    dup2(o[1], 1);
    dup2(e[1], 2);
    close(o[0]), close(o[1]), close(e[0]), close(e[1]);
    child(w, k);
  }
  close(o[1]);
  close(e[1]);
  Run r{-1, {}, {}};
  pollfd fds[2] = {{o[0], POLLIN, 0}, {e[0], POLLIN, 0}};
  int open_fds = 2;
  char buf[4096];
  while (open_fds > 0 && poll(fds, 2, 60000) > 0) {
    for (int i = 0; i < 2; ++i) {
      if (fds[i].fd < 0 || !(fds[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
      const ssize_t n = read(fds[i].fd, buf, sizeof buf);
      if (n <= 0) {
        close(fds[i].fd);
        fds[i].fd = -1;
        --open_fds;
      } else {
        (i == 0 ? r.out : r.err).append(buf, std::size_t(n));
      }
    }
  }
  int st = 0;
  CHECK(waitpid(pid, &st, 0) == pid);
  r.status = WIFEXITED(st) ? WEXITSTATUS(st) : 1000 + (WIFSIGNALED(st) ? WTERMSIG(st) : 0);
  return r;
}

int main() {
  const char* names[] = {"cout", "cerr", "wcout", "cout without stdio synchronization"};
  for (Which w : {to_cout, to_cerr, to_wcout, to_cout_unsynced}) {
    const std::string written = w == to_wcout ? "wide 42 3.5\n" : "narrow 42 3.5\nwritten\n";
    for (long k = 1;; ++k) {
      CHECK(k < 5000);
      const Run r = run_child(w, k);
      if (r.status != 0 && r.status != 10) {
        dprintf(2, "%s k=%ld: child status %d\nstderr: %s\n", names[w], k, r.status, r.err.c_str());
        CHECK(false);
      }
      // The text of the stream itself (stdout, or stderr before the status line).
      std::string text = w == to_cerr ? r.err : r.out;
      const std::string status = r.err.substr(r.err.rfind('|'));
      if (w == to_cerr) text.resize(text.size() - status.size());
      if (status == "|BAD-AFTER" || !text.ends_with("after\n") ||
          (status == "|good" && text != written + "after\n")) {
        dprintf(2, "%s k=%ld: %s, output [%s]\n", names[w], k, status.c_str(), text.c_str());
        CHECK(false);
      }
      if (r.status == 10) break;
    }
  }
  return 0;
}
